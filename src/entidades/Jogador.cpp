#include "andvarion/entidades/Jogador.hpp"
#include "andvarion/cenas/Mundo.hpp"
#include "andvarion/cenas/Sessao.hpp"
#include "andvarion/combate/EstadosJogador.hpp"
#include "andvarion/core/Assets.hpp"
#include "andvarion/core/EventBus.hpp"
#include "andvarion/entidades/AreaDano.hpp"
#include "andvarion/entidades/Familiar.hpp"
#include "andvarion/entidades/Projetil.hpp"
#include "andvarion/gfx/CameraJogo.hpp"
#include "andvarion/gfx/Iluminacao.hpp"
#include "andvarion/gfx/VFXManager.hpp"

#include <raymath.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace andv {

namespace {
constexpr float VIDA_BASE = 120.f;
constexpr float MANA_BASE = 60.f;
constexpr float VIGOR_BASE = 100.f;
constexpr float VELOCIDADE_BASE = 78.f;   // px/s
constexpr float ALTURA_MAO = 13.f;        // projéteis saem da mão, não dos pés (sprite ~27 unidades)

Vector2 limitarAlcance(Vector2 origem, Vector2 alvo, float maximo) {
    const Vector2 d = Vector2Subtract(alvo, origem);
    const float l = Vector2Length(d);
    return l <= maximo ? alvo : Vector2Add(origem, Vector2Scale(d, maximo / l));
}
} // namespace

Jogador::Jogador() {
    raio = 5.f;
    faccao = Faccao::Jogador;
    armaCorpo_ = armaCorpoPorId("espada");
    armaDistancia_ = armaDistanciaPorId("arco");
}

void Jogador::iniciar(Mundo& m) {
    sessao_ = &m.sessao();
    // Lanterna presa à cintura: luz quente, levemente trêmula.
    Luz lanterna;
    lanterna.pos = pos;
    lanterna.raio = 92.f;
    lanterna.cor = {255, 170, 80, 255};
    lanterna.intensidade = 0.95f;
    lanterna.oscilacao = 0.12f;
    lanterna.brilho = 0.22f;
    luzLanterna_ = m.luz().adicionar(lanterna);
    vfxSede_ = vfxEscudo_ = vfxEncantamento_ = 0;   // os VFX da área anterior foram parados
    escudoTempo = escudoPontos = encantamentoTempo = 0.f;
    afterimages.clear();
    bumerangueEmVoo = false;
    estados.iniciar(*this, m, std::make_unique<EstadoOcioso>());
    aplicarEquipamento(m);
}

void Jogador::aplicarEquipamento(Mundo& m) {
    Sessao& s = m.sessao();
    s.equipamento.validar(s.inventario);
    auto def = [&](SlotEquipamento slot) { return s.inventario.definicao(s.equipamento.item(slot)); };
    auto atlas = [&](SlotEquipamento slot) -> std::shared_ptr<const ConjuntoSprites> {
        const DefinicaoItem* d = def(slot);
        return d && !d->visual.empty() ? m.assets().personagem(d->visual) : nullptr;
    };
    if (const DefinicaoItem* d = def(SlotEquipamento::Arma))
        if (const auto* a = armaCorpoPorId(d->arma)) armaCorpo_ = a;
    if (const DefinicaoItem* d = def(SlotEquipamento::Distancia))
        if (const auto* a = armaDistanciaPorId(d->arma)) armaDistancia_ = a;
    // Paper-doll: cada slot vira uma camada sincronizada com o corpo.
    visual_.definir(CamadaVisual::Armadura, atlas(SlotEquipamento::Armadura));
    visual_.definir(CamadaVisual::Capa, atlas(SlotEquipamento::Capa));
    visual_.definir(CamadaVisual::Escudo, atlas(SlotEquipamento::Escudo));
    visualArma_ = atlas(SlotEquipamento::Arma);
    visualDistancia_ = atlas(SlotEquipamento::Distancia);
    visual_.definir(CamadaVisual::Arma, mostrandoDistancia_ ? visualDistancia_ : visualArma_);
    recalcularAtributos(s, false);
}

float Jogador::defesa() const {
    if (!sessao_) return 0.f;
    return sessao_->equipamento.bonus("defesa", sessao_->inventario) + sessao_->arvore.bonus("defesa");
}

void Jogador::recalcularAtributos(const Sessao& s, bool encher) {
    sessao_ = &s;
    const auto& a = s.arvore;
    auto equip = [&](const char* atributo) { return s.equipamento.bonus(atributo, s.inventario); };
    recursos.vida.definirMaximo(VIDA_BASE + a.bonus("vidaMax") + equip("vidaMax") + (s.progressao.nivel() - 1) * 6.f, encher);
    recursos.mana.definirMaximo(MANA_BASE + a.bonus("manaMax") + equip("manaMax"), encher);
    recursos.vigor.definirMaximo(VIGOR_BASE + a.bonus("vigorMax") + equip("vigorMax"), encher);
    recursos.mana.regenPorSeg = 1.5f + a.bonus("regenMana");
    vidaMax = recursos.vida.maximo;
    vida = recursos.vida.atual;
    sede.multDano = 1.6f + a.bonus("sedeDano");
}

float Jogador::velocidadeMovimento() const {
    const float bonus = sessao_ ? sessao_->arvore.bonus("velocidade") + sessao_->equipamento.bonus("velocidade", sessao_->inventario) : 0.f;
    return VELOCIDADE_BASE * (1.f + bonus) * sede.multiplicadorVelocidade();
}

float Jogador::multiplicadorLamina() const {
    if (!sessao_) return 1.f;
    return 1.f + sessao_->arvore.bonus("danoLamina") + sessao_->bonusDanoForja + sessao_->proficiencia.bonusDano(armaCorpo_->id);
}
float Jogador::multiplicadorDistancia() const {
    return sessao_ ? 1.f + sessao_->arvore.bonus("danoDistancia") + sessao_->proficiencia.bonusDano(armaDistancia_->id) : 1.f;
}
float Jogador::multiplicadorMagia() const {
    return sessao_ ? 1.f + sessao_->arvore.bonus("danoMagia") + sessao_->equipamento.bonus("danoMagia", sessao_->inventario) +
                         0.03f * (sessao_->progressao.nivel() - 1)
                   : 1.f;
}

// Troca rápida: só entre as armas que o jogador POSSUI (inventário).
void Jogador::ciclarCorpo(Mundo& m, int passo) {
    Sessao& s = m.sessao();
    s.equipamento.equipar(s.equipamento.proximo(SlotEquipamento::Arma, s.inventario, passo), s.inventario);
    aplicarEquipamento(m);
    avisar(armaCorpo_->nome);
}

void Jogador::ciclarDistancia(Mundo& m) {
    Sessao& s = m.sessao();
    s.equipamento.equipar(s.equipamento.proximo(SlotEquipamento::Distancia, s.inventario, 1), s.inventario);
    aplicarEquipamento(m);
    avisar(armaDistancia_->nome);
}

void Jogador::olharPara(Vector2 alvoMundo) { direcao = direcaoDeVetor(Vector2Subtract(alvoMundo, pos)); }

void Jogador::mover(Mundo& m, Vector2 dir, float velocidade, float dt) {
    const Vector2 antes = pos;
    pos = m.mapa().moverCirculo(pos, raio, Vector2Scale(dir, velocidade * dt));
    // A caminhada avança pela distância REAL (contra a parede, o pé não desliza).
    anim.avancarDistancia(Vector2Distance(antes, pos), 26.f);
}

void Jogador::atualizar(Mundo&, float) {}

void Jogador::atualizarJogador(Mundo& m, const ComandosJogador& cmd, float dt) {
    mira = cmd.miraMundo;
    tempoDesdeDano += dt;
    if (vivo()) recursos.atualizar(dt);
    // vida é a fonte da verdade para dano; o Recurso espelha para o HUD.
    recursos.vida.atual = vida;
    sede.atualizar(dt);
    for (auto& [id, r] : recargas_) r = std::max(0.f, r - dt);
    // Escudo Arcano e Encantamento de Lâmina expiram sozinhos.
    if (escudoTempo > 0.f && (escudoTempo -= dt) <= 0.f) escudoPontos = 0.f;
    if (escudoPontos <= 0.f && vfxEscudo_) {
        m.vfx().parar(vfxEscudo_);
        vfxEscudo_ = 0;
    }
    if (encantamentoTempo > 0.f) encantamentoTempo -= dt;
    if (encantamentoTempo <= 0.f && vfxEncantamento_) {
        m.vfx().parar(vfxEncantamento_);
        vfxEncantamento_ = 0;
    }
    recargaDisparo = std::max(0.f, recargaDisparo - dt);
    recargaEsquiva = std::max(0.f, recargaEsquiva - dt);
    janelaCombo = std::max(0.f, janelaCombo - dt);
    if (janelaCombo <= 0.f) proximoCombo = 0;
    avisoTempo = std::max(0.f, avisoTempo - dt);

    // Troca de arma: só fora de ataque, para não trocar o golpe pela metade.
    const bool livre = estados.em<EstadoOcioso>() || estados.em<EstadoMovendo>();
    if (livre && cmd.trocarCorpo != 0) ciclarCorpo(m, cmd.trocarCorpo);
    if (livre && cmd.trocarDistancia) ciclarDistancia(m);

    // A mão mostra a arma à distância enquanto mira e logo após o disparo.
    const bool distancia = estados.em<EstadoMirando>() || recargaDisparo > 0.f;
    if (distancia != mostrandoDistancia_) {
        mostrandoDistancia_ = distancia;
        visual_.definir(CamadaVisual::Arma, distancia ? visualDistancia_ : visualArma_);
    }
    if (cmd.frasco && vivo()) usarFrasco(m);

    atualizarComum(m, dt);
    estados.atualizar(*this, m, cmd, dt);
    vida = std::min(vida, vidaMax);
    recursos.vida.atual = vida;

    m.luz().mover(luzLanterna_, {pos.x + 3.f, pos.y - 9.f});

    // Aura da Sede de Sangue (pixelfx status/rage em vermelho) seguindo o corpo.
    if (sede.ativa() && !vfxSede_) {
        ParametrosVfx p;
        p.escala = 0.32f;
        p.seguir = [this] { return Vector2{pos.x, pos.y + 1.f}; };
        p.camada = CamadaVfx::Chao;          // aura por baixo: não lava o sprite
        p.tinta = Color{255, 255, 255, 210};
        vfxSede_ = m.vfx().tocar("sede_de_sangue", pos, p);
    } else if (!sede.ativa() && vfxSede_) {
        m.vfx().parar(vfxSede_);
        vfxSede_ = 0;
    }

    for (auto& a : afterimages) a.vida -= dt;
    while (!afterimages.empty() && afterimages.front().vida <= 0.f) afterimages.pop_front();
}

bool Jogador::receberDano(Mundo& m, const Dano& bruto) {
    // Defesa do equipamento: redução com retorno decrescente (100 de defesa = metade).
    Dano d = bruto;
    d.valor = bruto.valor * 100.f / (100.f + std::max(0.f, defesa()));
    // Escudo Arcano absorve antes da vida (e não interrompe o jogador).
    if (escudoPontos > 0.f && invulneravel <= 0.f && vivo()) {
        const float absorvido = std::min(escudoPontos, d.valor);
        escudoPontos -= absorvido;
        d.valor -= absorvido;
        m.luz().flash(pos, 40.f, Color{150, 120, 255, 255}, 0.15f);
        if (d.valor <= 0.f) {
            invulneravel = 0.2f;
            return false;
        }
    }
    if (!Combatente::receberDano(m, d)) return false;
    tempoDesdeDano = 0.f;
    invulneravel = 0.6f;  // janela de graça após levar dano
    m.camera().tremer(0.45f);
    m.hitStop(0.06f);
    m.bus().publicar(TipoEvento::JogadorFerido, {}, static_cast<i32>(d.valor));
    if (!vivo()) {
        estados.trocar(std::make_unique<EstadoMorto>());
        estados.aplicarPendente(*this, m);
    } else if (estados.atual() && estados.atual()->interrompivelPorDano()) {
        estados.trocar(std::make_unique<EstadoAtordoado>(0.22f));
        estados.aplicarPendente(*this, m);
    }
    return true;
}

void Jogador::executarGolpe(Mundo& m, int indice, Vector2 dir, float multDano) {
    const ArmaCorpoACorpo& arma = *armaCorpo_;
    const float metadeArco = arma.arcoGraus * 0.5f * DEG2RAD;
    const float angulo = std::atan2(dir.y, dir.x);

    // VFX do golpe (pixelfx melee/*), girado na direção da mira.
    ParametrosVfx pv;
    pv.anguloGraus = angulo * RAD2DEG;
    pv.orientar = true;
    pv.escala = arma.escalaVfx * (indice == 2 ? 1.2f : 1.f);
    if (multDano > 1.f) pv.tinta = Color{255, 150, 150, 255};
    m.vfx().tocar(arma.vfx[static_cast<std::size_t>(indice)], Vector2Add(pos, Vector2Scale(dir, 4.f)), pv);

    int acertos = 0;
    const bool voraz = sessao_ && sessao_->arvore.temPassiva("drenagem_voraz");
    for (Combatente* alvo : m.alvosNoRaio(pos, arma.alcance + 6.f, Faccao::Jogador)) {
        const Vector2 para = Vector2Subtract(alvo->pos, pos);
        const float dist = Vector2Length(para);
        float diferenca = std::fabs(std::atan2(para.y, para.x) - angulo);
        if (diferenca > PI) diferenca = 2.f * PI - diferenca;
        if (dist > 10.f && diferenca > metadeArco) continue;

        Dano d;
        const float fogo = encantamentoTempo > 0.f ? 1.5f + (sessao_ ? sessao_->arvore.bonus("encantamento_lamina.dano") : 0.f) : 1.f;
        d.valor = arma.dano[static_cast<std::size_t>(indice)] * multiplicadorLamina() * multDano * fogo;
        d.origem = pos;
        d.empurrao = arma.empurrao * (indice == 2 ? 1.6f : 1.f);
        d.fonte = Faccao::Jogador;
        d.lamina = true;
        d.critico = multDano > 1.f || indice == 2;
        if (!alvo->receberDano(m, d)) continue;
        ++acertos;
        // Ciclo de Sinergia: cada acerto da Lâmina drena essência vital -> Mana.
        recursos.mana.ganhar(arma.manaPorAcerto * (voraz ? 1.5f : 1.f));
        ParametrosVfx pi;
        pi.escala = d.critico ? 0.22f : 0.18f;
        pi.anguloGraus = angulo * RAD2DEG;
        pi.orientar = true;
        m.vfx().tocar(d.critico ? "impacto_critico" : "impacto_sangue", alvo->pos, pi);
        if (encantamentoTempo > 0.f) m.vfx().tocar("impacto_fogo", alvo->pos, pi);
    }
    if (acertos > 0) ganharProficiencia(m, arma.id, acertos);
    if (acertos > 0) {
        // Hit-stop: o act do pixelfx dita o congelamento; o golpe final pesa mais.
        const float congelar = std::max(m.vfx().congelamentoDoGolpe(arma.vfx[static_cast<std::size_t>(indice)]), 0.04f);
        m.hitStop(std::min(0.12f, congelar * (indice == 2 ? 1.8f : 1.f)));
        m.camera().tremer(indice == 2 ? 0.32f : 0.16f);
        m.luz().flash(Vector2Add(pos, Vector2Scale(dir, 14.f)), 40.f, Color{255, 60, 60, 255}, 0.15f);
    }
}

bool Jogador::disparar(Mundo& m, Vector2 alvo, float carga01, float multDano) {
    const ArmaDistancia& arma = *armaDistancia_;
    if (recargaDisparo > 0.f) return false;
    if (arma.projetil == TipoProjetil::Bumerangue && bumerangueEmVoo) {
        avisar("O bumerangue ainda não voltou.");
        return false;
    }
    const bool carregado = carga01 >= 1.f;
    const float custo = arma.custoVigor * (carregado ? 1.5f : 1.f);
    if (!recursos.vigor.gastar(custo)) {
        avisar("Vigor insuficiente.");
        return false;
    }
    const Vector2 origem{pos.x, pos.y - ALTURA_MAO};
    const Vector2 dir = Vector2Normalize(Vector2Subtract(alvo, origem));
    const float dano = arma.dano * multiplicadorDistancia() * multDano * (carregado ? arma.multCarga : 1.f + 0.5f * carga01);

    auto criar = [&](Vector2 d) {
        auto p = std::make_unique<Projetil>(arma.projetil, origem, d, arma.velocidade * (carregado ? 1.25f : 1.f), dano,
                                            arma.alcance, Faccao::Jogador);
        p->vfx = arma.vfx;
        p->escalaVfx = arma.escalaVfx;
        p->proficiencia = arma.id;
        p->perfurante = carregado && arma.projetil != TipoProjetil::Bumerangue;
        if (carregado) {
            p->tinta = Color{255, 140, 140, 255};
            p->corLuz = Color{255, 60, 60, 255};
            p->empurrao = 120.f;
        }
        if (multDano > 1.f) p->tinta = Color{255, 110, 110, 255};
        m.adicionar(std::move(p));
    };
    criar(dir);
    if (carregado && arma.projetil != TipoProjetil::Bumerangue && sessao_ && sessao_->arvore.temPassiva("disparo_triplo")) {
        criar(Vector2Rotate(dir, 12.f * DEG2RAD));
        criar(Vector2Rotate(dir, -12.f * DEG2RAD));
    }
    if (arma.projetil == TipoProjetil::Bumerangue) bumerangueEmVoo = true;
    recargaDisparo = arma.cadencia;
    m.camera().tremer(carregado ? 0.2f : 0.05f);
    return true;
}

const std::string& Jogador::feiticoNoSlot(int slot) const {
    static const std::string vazio;
    if (!sessao_ || slot < 0 || slot >= SLOTS_GRIMORIO) return vazio;
    return sessao_->grimorio[static_cast<std::size_t>(slot)];
}

float Jogador::recarga(const std::string& feitico) const {
    auto it = recargas_.find(feitico);
    return it == recargas_.end() ? 0.f : it->second;
}

float Jogador::custoMana(const DefinicaoFeitico& f) const {
    const float reducao = sessao_ ? sessao_->proficiencia.reducaoCusto(f.escola) : 0.f;
    return f.custoMana * (1.f - reducao);
}

float Jogador::bonusFeitico(const std::string& feitico, const char* chave) const {
    return sessao_ ? sessao_->arvore.bonus(feitico + "." + chave) : 0.f;
}

void Jogador::ganharProficiencia(Mundo& m, const std::string& chave, i32 xp) {
    if (const i32 nivel = m.sessao().proficiencia.ganhar(chave, xp))
        m.bus().publicar(TipoEvento::ProficienciaSubiu, chave, nivel);
}

bool Jogador::podeConjurar(const Mundo&, int slot, std::string* motivo) const {
    auto falhar = [&](const char* t) {
        if (motivo) *motivo = t;
        return false;
    };
    const std::string& id = feiticoNoSlot(slot);
    if (id.empty()) return falhar("Slot vazio: coloque uma magia nele na Árvore de Habilidades (K).");
    const DefinicaoFeitico* f = feiticoPorId(id);
    if (!f || !sessao_->arvore.feiticoLiberado(id)) return falhar("Feitiço ainda não aprendido (K).");
    if (recarga(id) > 0.f) return falhar("O feitiço ainda está recarregando.");
    if (!recursos.mana.tem(custoMana(*f))) return falhar("Mana insuficiente. Corte para drenar essência!");
    return true;
}

// Alvo de magias de toque: o inimigo mais perto da mira (até 60 do cursor e
// 160 do jogador) ou, sem nenhum, o mais perto do próprio jogador.
Combatente* Jogador::alvoMaisProximo(Mundo& m, Vector2 mira, float alcance) {
    Combatente* melhor = nullptr;
    float menor = 60.f;
    for (Combatente* c : m.alvosNoRaio(pos, alcance + 50.f, Faccao::Jogador)) {
        const float d = Vector2Distance(c->pos, mira);
        if (d < menor) { menor = d; melhor = c; }
    }
    if (melhor) return melhor;
    menor = alcance;
    for (Combatente* c : m.alvosNoRaio(pos, alcance, Faccao::Jogador)) {
        const float d = Vector2Distance(c->pos, pos);
        if (d < menor) { menor = d; melhor = c; }
    }
    return melhor;
}

void Jogador::lancarFeitico(Mundo& m, int slot, Vector2 alvo) {
    const std::string id = feiticoNoSlot(slot);
    const DefinicaoFeitico* f = feiticoPorId(id);
    if (!f || !recursos.mana.gastar(custoMana(*f))) return;
    recargas_[id] = f->recarga * std::max(0.2f, 1.f - bonusFeitico(id, "recarga"));
    ganharProficiencia(m, f->escola, 3);

    // Ciclo de Sinergia: gastar mana arma a Sede de Sangue no próximo ataque.
    const bool prolongada = sessao_ && sessao_->arvore.temPassiva("sede_prolongada");
    sede.ativar(prolongada ? 8.f : 5.f, prolongada ? 2 : 1);

    // Dano mágico: base + proficiência da escola + melhorias do galho da magia.
    const float magia = (multiplicadorMagia() + m.sessao().proficiencia.bonusDano(f->escola)) * (1.f + bonusFeitico(id, "dano"));
    const Vector2 mao{pos.x, pos.y - ALTURA_MAO};
    m.luz().flash(mao, 50.f, Color{220, 40, 60, 255}, 0.25f);

    auto projetil = [&](TipoProjetil tipo, float velocidade, float dano, float alcance) {
        auto p = std::make_unique<Projetil>(tipo, mao, Vector2Subtract(alvo, mao), velocidade, dano, alcance, Faccao::Jogador);
        p->proficiencia = f->escola;
        return p;
    };

    if (id == "orbe_sangue") {
        auto p = projetil(TipoProjetil::OrbeSangue, 150.f, 24.f * magia, 210.f);
        p->vfx = "orbe_sangue";
        p->escalaVfx = 0.16f;
        p->corLuz = Color{255, 40, 50, 255};
        p->raioExplosao = 26.f;
        p->vfxExplosao = "orbe_sangue_explosao";
        p->empurrao = 60.f;
        m.adicionar(std::move(p));
    } else if (id == "nevoa_cortante") {
        auto a = std::make_unique<AreaDano>(limitarAlcance(pos, alvo, 110.f), 30.f, 3.5f, 0.4f, 6.f * magia, Faccao::Jogador);
        a->vfx = "nevoa_cortante";
        a->escalaVfx = 0.36f;
        a->corLuz = Color{150, 80, 220, 255};
        m.adicionar(std::move(a));
    } else if (id == "correntes_sombrias") {
        auto a = std::make_unique<AreaDano>(limitarAlcance(pos, alvo, 110.f), 28.f, 1.7f, 0.8f, 10.f * magia, Faccao::Jogador);
        a->vfx = "correntes_sombrias";
        a->escalaVfx = 0.3f;
        a->atordoar = 2.2f;
        a->corLuz = Color{110, 60, 200, 255};
        m.adicionar(std::move(a));
    } else if (id == "sangria_vital") {
        const auto alvos = m.alvosNoRaio(pos, 75.f, Faccao::Jogador);
        float drenado = 0.f;
        for (Combatente* c : alvos) {
            Dano d;
            d.valor = 14.f * magia;
            d.origem = pos;
            d.empurrao = 20.f;
            d.fonte = Faccao::Jogador;
            if (c->receberDano(m, d)) {
                drenado += d.valor;
                ParametrosVfx pi;
                pi.escala = 0.18f;
                m.vfx().tocar("impacto_sangue", c->pos, pi);
            }
        }
        vida = std::min(vidaMax, vida + drenado * 0.5f);
        ParametrosVfx p;
        p.escala = 0.35f;
        p.orientar = true;
        p.anguloGraus = alvos.empty() ? 0.f : std::atan2(alvos[0]->pos.y - pos.y, alvos[0]->pos.x - pos.x) * RAD2DEG;
        m.vfx().tocar("sangria_vital", pos, p);
        if (alvos.empty()) avisar("Nenhuma vítima por perto...");
    } else if (id == "bola_fogo") {
        auto p = projetil(TipoProjetil::BolaFogo, 175.f, 30.f * magia, 220.f);
        p->vfx = "bola_fogo";
        p->escalaVfx = 0.2f;
        p->corLuz = Color{255, 140, 40, 255};
        p->raioExplosao = 30.f + bonusFeitico(id, "raio");
        p->vfxExplosao = "bola_fogo_explosao";
        p->escalaExplosao = 0.3f + bonusFeitico(id, "raio") * 0.008f;
        p->vfxImpacto = "impacto_fogo";
        p->empurrao = 80.f;
        m.adicionar(std::move(p));
    } else if (id == "lanca_gelo") {
        auto p = projetil(TipoProjetil::LancaGelo, 230.f, 16.f * magia, 200.f);
        p->vfx = "lanca_gelo";
        p->escalaVfx = 0.18f;
        p->perfurante = true;
        p->lentidao = 2.5f + bonusFeitico(id, "lentidao");
        p->vfxImpacto = "gelo_impacto";
        p->corLuz = Color{120, 200, 255, 255};
        m.adicionar(std::move(p));
    } else if (id == "relampago") {
        // Salta do inimigo mais perto da mira para os vizinhos, perdendo força.
        const int saltos = 4 + static_cast<int>(bonusFeitico(id, "alvos"));
        std::vector<Combatente*> atingidos;
        Combatente* atual = alvoMaisProximo(m, alvo, 120.f);
        Vector2 de = mao;
        float dano = 20.f * magia;
        for (int k = 0; atual && k < saltos; ++k) {
            const Vector2 para{atual->pos.x, atual->pos.y - 8.f};
            ParametrosVfx p;
            p.orientar = true;
            p.anguloGraus = std::atan2(para.y - de.y, para.x - de.x) * RAD2DEG;
            p.escala = std::clamp(Vector2Distance(de, para) / 220.f, 0.12f, 0.5f);
            p.aditivo = true;
            m.vfx().tocar("relampago", de, p);
            ParametrosVfx pz;
            pz.escala = 0.18f;
            m.vfx().tocar("relampago_impacto", atual->pos, pz);
            m.luz().flash(para, 45.f, Color{170, 200, 255, 255}, 0.2f);
            Dano d;
            d.valor = dano;
            d.origem = de;
            d.empurrao = 30.f;
            d.fonte = Faccao::Jogador;
            d.atordoar = 0.25f;
            atual->receberDano(m, d);
            atingidos.push_back(atual);
            dano *= 0.85f;
            de = para;
            Combatente* proximo = nullptr;
            float menor = 70.f;
            for (Combatente* c : m.alvosNoRaio(atual->pos, 70.f, Faccao::Jogador)) {
                if (std::find(atingidos.begin(), atingidos.end(), c) != atingidos.end()) continue;
                const float dd = Vector2Distance(c->pos, atual->pos);
                if (dd < menor) { menor = dd; proximo = c; }
            }
            atual = proximo;
        }
        if (atingidos.empty()) avisar("O raio não encontrou alvo.");
        m.camera().tremer(0.2f);
    } else if (id == "escudo_arcano") {
        escudoPontos = 40.f + bonusFeitico(id, "pontos");
        escudoTempo = 8.f;
        if (!vfxEscudo_) {
            ParametrosVfx p;
            p.escala = 0.36f;
            p.aditivo = true;
            p.seguir = [this] { return Vector2{pos.x, pos.y + 2.f}; };
            vfxEscudo_ = m.vfx().tocar("escudo_arcano", pos, p);
        }
    } else if (id == "drenar_vida") {
        Combatente* c = alvoMaisProximo(m, alvo, 110.f);
        if (!c) {
            avisar("Nenhuma vida ao alcance.");
        } else {
            Dano d;
            d.valor = 22.f * magia;
            d.origem = pos;
            d.fonte = Faccao::Jogador;
            if (c->receberDano(m, d)) vida = std::min(vidaMax, vida + d.valor * (0.6f + bonusFeitico(id, "cura")));
            ParametrosVfx p;
            p.escala = std::clamp(Vector2Distance(pos, c->pos) / 160.f, 0.2f, 0.5f);
            p.orientar = true;
            p.anguloGraus = std::atan2(c->pos.y - pos.y, c->pos.x - pos.x) * RAD2DEG;
            p.duracaoMax = 1.0f;
            m.vfx().tocar("drenar_vida", pos, p);
            ParametrosVfx pi;
            pi.escala = 0.18f;
            m.vfx().tocar("impacto_sangue", c->pos, pi);
        }
    } else if (id == "familiar") {
        const int quantos = 1 + static_cast<int>(bonusFeitico(id, "extra"));
        for (int k = 0; k < quantos; ++k)
            m.adicionar(std::make_unique<Familiar>(20.f + bonusFeitico(id, "duracao"), k * PI, 7.f * magia));
        ParametrosVfx p;
        p.escala = 0.25f;
        m.vfx().tocar("teleporte_entrada", pos, p);
    } else if (id == "teleporte") {
        // Avança em passos de 4 unidades até o alcance ou a primeira parede.
        const float alcance = 90.f + bonusFeitico(id, "alcance");
        Vector2 d = Vector2Subtract(alvo, pos);
        const float dist = std::min(alcance, Vector2Length(d));
        d = Vector2Length(d) > 0.01f ? Vector2Normalize(d) : vetorDeDirecao(direcao);
        Vector2 destino = pos;
        for (float t = 4.f; t <= dist; t += 4.f) {
            const Vector2 p = Vector2Add(pos, Vector2Scale(d, t));
            if (m.mapa().colideCirculo(p, raio)) break;
            destino = p;
        }
        ParametrosVfx ps;
        ps.escala = 0.28f;
        m.vfx().tocar("teleporte_saida", pos, ps);
        criarAfterimage();
        pos = destino;
        invulneravel = std::max(invulneravel, 0.25f);
        m.vfx().tocar("teleporte_entrada", pos, ps);
        m.luz().flash(pos, 60.f, Color{160, 120, 255, 255}, 0.3f);
    } else if (id == "encantamento_lamina") {
        encantamentoTempo = 10.f + bonusFeitico(id, "duracao");
        if (!vfxEncantamento_) {
            ParametrosVfx p;
            p.escala = 0.14f;
            p.aditivo = true;
            p.seguir = [this] {
                const Vector2 f = vetorDeDirecao(direcao);
                return Vector2{pos.x + f.x * 8.f, pos.y - 6.f + f.y * 3.f};   // na lâmina, à frente
            };
            vfxEncantamento_ = m.vfx().tocar("encantamento", pos, p);
        }
    }
}

void Jogador::iniciarEsquiva(Mundo& m) {
    invulneravel = 0.3f;  // i-frames cobrem o dash inteiro
    recargaEsquiva = 0.35f;
    ParametrosVfx p;
    p.escala = 0.3f;
    p.camada = CamadaVfx::Chao;
    m.vfx().tocar("esquiva_sombria", pos, p);
}

void Jogador::criarAfterimage() {
    auto camadas = visual_.quadros(anim);   // todas as camadas: o rastro mostra o equipamento
    if (camadas.empty()) return;
    afterimages.push_back({std::move(camadas), pos, 0.28f, 0.28f});
    while (afterimages.size() > 10) afterimages.pop_front();
}

bool Jogador::usarFrasco(Mundo& m) {
    Sessao& s = m.sessao();
    if (s.frascos <= 0) {
        avisar("Sem frascos de sangue. Visite a Curandeira.");
        return false;
    }
    if (vida >= vidaMax) {
        avisar("Sua vida já está cheia.");
        return false;
    }
    --s.frascos;
    vida = std::min(vidaMax, vida + vidaMax * 0.45f);
    ParametrosVfx p;
    p.escala = 0.35f;
    p.seguir = [this] { return pos; };
    m.vfx().tocar("cura", pos, p);
    m.luz().flash(pos, 50.f, Color{120, 255, 140, 255}, 0.5f);
    return true;
}

void Jogador::desenharAfterimages() const {
    for (const auto& a : afterimages) {
        const float k = a.vida / a.total;
        // Silhuetas violeta, cada vez mais transparentes (rastro da Esquiva Sombria).
        for (const QuadroCongelado& q : a.camadas)
            if (q.sheet)
                q.sheet->desenhar(q.quadro, {snapTela(a.pos.x), snapTela(a.pos.y)}, ESCALA_DESENHO, q.espelhar, 0.f,
                                  Color{120, 70, 255, static_cast<unsigned char>(110 * k)});
    }
}

void Jogador::desenhar(const Mundo&) const {
    desenharSombra(11.f);
    Color t = tinta();
    if (sede.ativa()) {
        const float p = 0.5f + 0.5f * std::sin(static_cast<float>(GetTime()) * 10.f);
        t = ColorTint(t, Color{255, static_cast<unsigned char>(235 - 30 * p), static_cast<unsigned char>(235 - 30 * p), 255});
    }
    if (anim.temSprites()) {
        visual_.desenhar(anim, pos, t);   // corpo + armadura + capa + escudo + arma, sincronizados
    } else {
        DrawRectangle(static_cast<int>(pos.x) - 5, static_cast<int>(pos.y) - 22, 10, 22, ColorTint(Color{40, 50, 90, 255}, t));
        DrawRectangle(static_cast<int>(pos.x) - 5, static_cast<int>(pos.y) - 26, 10, 6, ColorTint(Color{220, 225, 235, 255}, t));
    }

    // Mira à distância: linha pontilhada + anel de carga.
    if (cargaVisual >= 0.f) {
        const Vector2 mao{pos.x, pos.y - ALTURA_MAO};
        const Vector2 dir = Vector2Normalize(Vector2Subtract(mira, mao));
        const float alcance = std::min(armaDistancia_->alcance, Vector2Distance(mao, mira));
        const Color c = cargaVisual >= 1.f ? Color{255, 70, 70, 200} : Color{230, 220, 200, 120};
        for (float d = 8.f; d < alcance; d += 6.f) {
            const Vector2 p = Vector2Add(mao, Vector2Scale(dir, d));
            DrawPixelV({std::round(p.x), std::round(p.y)}, c);
        }
        DrawRing(mao, 7.f, 8.5f, -90.f, -90.f + 360.f * std::min(1.f, cargaVisual), 24, c);
    }
}

} // namespace andv

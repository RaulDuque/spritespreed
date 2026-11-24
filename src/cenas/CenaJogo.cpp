#include "andvarion/cenas/CenaJogo.hpp"
#include "andvarion/cenas/CenaMenuPrincipal.hpp"
#include "andvarion/core/Assets.hpp"
#include "andvarion/core/Escala.hpp"
#include "andvarion/entidades/NPC.hpp"
#include "andvarion/gfx/VFXManager.hpp"
#include "andvarion/save/SaveSystem.hpp"
#include "andvarion/ui/Fonte.hpp"
#include "andvarion/ui/Pergaminho.hpp"

#include <raymath.h>

#include <algorithm>
#include <cmath>

namespace andv {

namespace {
constexpr const char* AREA_INICIAL = "vila_cinzavale";
constexpr float DURACAO_FADE = 0.35f;
constexpr float CENTRO_Y = ALTURA_VIRTUAL * 0.5f;   // menus centralizados na vertical
const char* OPCOES_PAUSA[4] = {"Continuar", "Salvar Jogo", "Árvore de Habilidades", "Salvar e Sair para o Menu"};
} // namespace

CenaJogo::CenaJogo(Jogo& jogo, std::optional<SaveData> save) : Cena(jogo) {
    sessao_ = std::make_unique<Sessao>(jogo.bus(), jogo.assets());
    luz_.iniciar(LARGURA_VIRTUAL, ALTURA_VIRTUAL);
    camera_.configurar(LARGURA_VIRTUAL, ALTURA_VIRTUAL, ZOOM_MUNDO);
    jogador_.anim.definirConjunto(jogo.assets().personagem("sprites/protagonista"));

    std::string area = AREA_INICIAL;
    std::optional<Vector2> posicao;
    if (save) {
        Sessao& s = *sessao_;
        s.progressao.restaurar(save->nivel, save->xp, save->pontos);
        s.arvore.restaurar(save->habilidades);
        s.inventario.restaurar(save->inventario);
        s.missoes->restaurar(save->missoes);
        s.coletados = {save->coletados.begin(), save->coletados.end()};
        s.frascos = save->frascos;
        s.frascosMax = save->frascosMax;
        s.bonusDanoForja = save->bonusDanoForja;
        s.equipamento.restaurar(save->equipamento, s.inventario);
        s.proficiencia.restaurar(save->proficiencias);
        for (std::size_t k = 0; k < s.grimorio.size(); ++k) s.grimorio[k] = k < save->grimorio.size() ? save->grimorio[k] : "";
        s.tempoJogo = save->tempoJogoSeg;
        area = save->area;
        if (save->temPosicao) posicao = Vector2{save->posX, save->posY};
    }
    sessao_->organizarGrimorio();
    // Novo jogo, ou save anterior ao sistema de equipamento: entrega o kit inicial.
    if (sessao_->equipamento.item(SlotEquipamento::Arma).empty()) sessao_->aplicarKitInicial();
    jogador_.recalcularAtributos(*sessao_, true);
    if (save && save->vida > 0.f) jogador_.vida = std::min(jogador_.vidaMax, save->vida);
    if (save && save->mana >= 0.f) jogador_.recursos.mana.atual = std::min(jogador_.recursos.mana.maximo, save->mana);

    // Observers: recompensas, morte, notificações.
    EventBus& bus = jogo.bus();
    inscricoes_.push_back(bus.inscrever(TipoEvento::MissaoConcluida, [this](const Evento& e) { entregarRecompensa(e.id); }));
    inscricoes_.push_back(bus.inscrever(TipoEvento::JogadorMorreu, [this](const Evento&) {
        modo_ = Modo::Morto;
        tempoMorte_ = 0.f;
    }));
    notificacoes_ = std::make_unique<Notificacoes>(bus, [this](const std::string& id) {
        const Missao* m = sessao_->missoes->missao(id);
        return m ? m->titulo : id;
    });

    dialogo_ = std::make_unique<DialogueSystem>(sessao_->dialogos, *sessao_->missoes, sessao_->inventario, jogo.assets(), bus);
    dialogo_->definirExecutor([this](const std::string& a) { executarAcaoDialogo(a); });

    arvore_.definirCallback([this](const NoHabilidade& n) {
        sessao_->organizarGrimorio();   // magia nova entra num slot livre
        jogador_.recalcularAtributos(*sessao_, false);
        jogo_.bus().publicar(TipoEvento::HabilidadeDesbloqueada, n.id);
        notificacoes_->mostrar("Aprendido: " + n.nome, ui::DOURADO);
    });

    inventario_.definirCallback([this] { jogador_.aplicarEquipamento(*mundo_); });

    mundo_ = std::make_unique<Mundo>(jogo, *sessao_, jogador_, luz_, camera_);
    if (!mundo_->carregarArea(area, "inicio", posicao)) mundo_->carregarArea(AREA_INICIAL, "inicio", std::nullopt);
    anunciarArea();
    if (!save) notificacoes_->mostrar("Fale com o Vigia no portão leste da vila.", ui::OSSO, 6.f);
}

CenaJogo::~CenaJogo() {
    inscricoes_.clear();
    jogo_.vfx().pararTodos();
}

void CenaJogo::anunciarArea() {
    areaAnunciada_ = mundo_->mapa().nome();
    tempoAnuncio_ = 3.f;
}

void CenaJogo::lerEntrada() { cmd_.acumular(lerComandos(jogo_.mouseVirtual(), camera_.camera())); }

void CenaJogo::entregarRecompensa(const std::string& id) {
    const Missao* m = sessao_->missoes->missao(id);
    if (!m) return;
    Sessao& s = *sessao_;
    if (m->consumirItens)
        for (const Objetivo& o : m->objetivos)
            if (o.tipo == TipoObjetivo::ColetarItem) s.inventario.remover(o.alvo, o.quantidade);
    for (const auto& [item, qtd] : m->recompensa.itens) s.inventario.adicionar(item, qtd);

    const std::string& bonus = m->recompensa.bonus;
    if (bonus == "lamina_reforjada") {
        s.bonusDanoForja += 0.2f;
        notificacoes_->mostrar("Sua lâmina foi reforjada: +20% de dano!", ui::DOURADO, 4.f);
    } else if (bonus == "frasco_extra") {
        s.frascosMax += 2;
        s.frascos = s.frascosMax;
        notificacoes_->mostrar("Frascos de Sangue: capacidade +2!", Color{220, 120, 120, 255}, 4.f);
    } else if (bonus == "fim_capitulo") {
        notificacoes_->mostrar("Capítulo I concluído. A catedral ainda sussurra...", ui::SANGUE, 6.f);
    }
    if (m->recompensa.xp > 0) mundo_->recompensarXP(m->recompensa.xp);
    ParametrosVfx p;
    p.escala = 0.4f;
    p.seguir = [this] { return jogador_.pos; };
    jogo_.vfx().tocar("habilidade_desbloqueada", jogador_.pos, p);
    salvar(true);  // checkpoint automático
}

void CenaJogo::executarAcaoDialogo(const std::string& acao) {
    if (acao == "curar") {
        jogador_.vida = jogador_.vidaMax;
        sessao_->frascos = sessao_->frascosMax;
        ParametrosVfx p;
        p.escala = 0.4f;
        p.seguir = [this] { return jogador_.pos; };
        jogo_.vfx().tocar("cura", jogador_.pos, p);
        notificacoes_->mostrar("Vida e frascos restaurados.", Color{150, 220, 140, 255});
    } else if (acao == "salvar") {
        salvar();
    }
}

void CenaJogo::salvar(bool silencioso) {
    const Sessao& s = *sessao_;
    SaveData d;
    d.area = mundo_->mapa().id();
    d.posX = jogador_.pos.x;
    d.posY = jogador_.pos.y;
    d.temPosicao = true;
    d.nivel = s.progressao.nivel();
    d.xp = s.progressao.xp();
    d.pontos = s.progressao.pontos();
    d.vida = jogador_.vida;
    d.mana = jogador_.recursos.mana.atual;
    d.frascos = s.frascos;
    d.frascosMax = s.frascosMax;
    d.equipamento = s.equipamento.salvar();
    d.proficiencias = s.proficiencia.todas();
    d.grimorio = {s.grimorio.begin(), s.grimorio.end()};
    d.bonusDanoForja = s.bonusDanoForja;
    d.habilidades = {s.arvore.desbloqueados().begin(), s.arvore.desbloqueados().end()};
    d.inventario = s.inventario.itens();
    d.missoes = s.missoes->salvar();
    d.coletados = {s.coletados.begin(), s.coletados.end()};
    d.tempoJogoSeg = s.tempoJogo;
    std::string erro;
    if (jogo_.save().salvar(d, &erro)) {
        if (!silencioso) notificacoes_->mostrar("Jogo salvo.", ui::OSSO);
    } else {
        notificacoes_->mostrar("Falha ao salvar: " + erro, ui::SANGUE, 4.f);
    }
}

void CenaJogo::despertar() {
    // Como nos sonhos de caçador: desperta na vila, perdendo os ecos do nível atual.
    const i32 perdido = sessao_->progressao.xp();
    sessao_->progressao.restaurar(sessao_->progressao.nivel(), 0, sessao_->progressao.pontos());
    sessao_->frascos = sessao_->frascosMax;
    mundo_->carregarArea(AREA_INICIAL, "inicio", std::nullopt);
    jogador_.recalcularAtributos(*sessao_, true);
    jogador_.invulneravel = 1.5f;
    anunciarArea();
    if (perdido > 0) notificacoes_->mostrar("Você despertou na vila. Ecos perdidos: " + std::to_string(perdido), ui::SANGUE, 5.f);
    modo_ = Modo::Jogando;
    fade_ = 1.f;
}

void CenaJogo::atualizarPausa(const ComandosJogador& c) {
    if (c.pausar && !c.cancelar) {
        modo_ = Modo::Jogando;
        return;
    }
    if (c.cima) opcaoPausa_ = (opcaoPausa_ + 3) % 4;
    if (c.baixo) opcaoPausa_ = (opcaoPausa_ + 1) % 4;
    int escolhida = c.confirmar ? opcaoPausa_ : -1;
    for (int i = 0; i < 4; ++i) {
        const Rectangle r{LARGURA_VIRTUAL * 0.5f - 80.f, CENTRO_Y - 25.f + i * 18.f, 160.f, 15.f};
        if (CheckCollisionPointRec(c.mouseVirtual, r)) {
            opcaoPausa_ = i;
            if (c.clique) escolhida = i;
        }
    }
    if (c.cancelar) {
        modo_ = Modo::Jogando;
        return;
    }
    switch (escolhida) {
        case 0: modo_ = Modo::Jogando; break;
        case 1: salvar(); modo_ = Modo::Jogando; break;
        case 2: arvore_.abrir(); modo_ = Modo::Arvore; break;
        case 3:
            salvar(true);
            jogo_.trocarCena(std::make_unique<CenaMenuPrincipal>(jogo_));
            break;
        default: break;
    }
}

void CenaJogo::atualizarMorte(const ComandosJogador& c) {
    if (tempoMorte_ > 1.5f && (c.confirmar || c.clique)) despertar();
}

void CenaJogo::atualizarTransicao(float dt) {
    if (!carregou_) {
        fade_ = std::min(1.f, fade_ + dt / DURACAO_FADE);
        if (fade_ >= 1.f) {
            const PedidoTransicao p = *transicao_;
            if (!mundo_->carregarArea(p.destino, p.spawn, std::nullopt))
                notificacoes_->mostrar("Área indisponível: " + p.destino, ui::SANGUE);
            carregou_ = true;
            anunciarArea();
            salvar(true);  // checkpoint ao trocar de área
        }
    } else {
        fade_ = std::max(0.f, fade_ - dt / DURACAO_FADE);
        if (fade_ <= 0.f) {
            transicao_.reset();
            modo_ = Modo::Jogando;
        }
    }
}

void CenaJogo::atualizar(float dt) {
    const ComandosJogador c = cmd_;
    cmd_.consumirBordas();  // bordas valem só para o primeiro passo fixo do quadro

    sessao_->tempoJogo += dt;
    notificacoes_->atualizar(dt);
    hud_.atualizar(jogador_, dt);
    tempoAnuncio_ = std::max(0.f, tempoAnuncio_ - dt);
    if (modo_ != Modo::Transicao) fade_ = std::max(0.f, fade_ - dt / DURACAO_FADE);

    switch (modo_) {
        case Modo::Jogando: {
            if (c.pausar) {
                modo_ = Modo::Pausa;
                opcaoPausa_ = 0;
                return;
            }
            if (c.arvore) {
                arvore_.abrir();
                modo_ = Modo::Arvore;
                return;
            }
            if (c.inventario) {
                inventario_.abrir();
                modo_ = Modo::Inventario;
                return;
            }
            if (c.interagir && jogador_.vivo()) {
                if (NPC* npc = mundo_->npcProximo(jogador_.pos, 24.f)) {
                    npc->virarPara(jogador_.pos);
                    jogador_.olharPara(npc->pos);
                    if (dialogo_->iniciar(npc->id())) {
                        modo_ = Modo::Dialogo;
                        return;
                    }
                }
            }
            mundo_->atualizar(dt, c);
            if (mundo_->pedidoTransicao) {
                transicao_ = mundo_->pedidoTransicao;
                mundo_->pedidoTransicao.reset();
                carregou_ = false;
                modo_ = Modo::Transicao;
            }
            break;
        }
        case Modo::Dialogo: {
            mundo_->atualizarApresentacao(dt);
            dialogo_->atualizar(dt, c);
            if (!dialogo_->ativo()) modo_ = Modo::Jogando;
            break;
        }
        case Modo::Arvore:
            arvore_.atualizar(dt, c, *sessao_);
            if (!arvore_.aberto()) modo_ = Modo::Jogando;
            break;
        case Modo::Inventario:
            mundo_->atualizarApresentacao(dt);
            jogador_.anim.tocar("ocioso", Direcao::Baixo);   // prévia de frente
            jogador_.anim.atualizar(dt);
            inventario_.atualizar(dt, c, *sessao_);
            if (!inventario_.aberto()) modo_ = Modo::Jogando;
            break;
        case Modo::Pausa:
            atualizarPausa(c);
            break;
        case Modo::Morto: {
            tempoMorte_ += dt;
            ComandosJogador nada;
            nada.miraMundo = jogador_.mira;
            mundo_->atualizar(dt, nada);  // o mundo segue vivo atrás da tela de morte
            atualizarMorte(c);
            break;
        }
        case Modo::Transicao:
            atualizarTransicao(dt);
            break;
    }
}

void CenaJogo::desenharMira(Vector2 m) const {
    const Color c = jogador_.sede.ativa() ? Color{255, 80, 80, 255} : ui::OSSO;
    const int x = static_cast<int>(m.x), y = static_cast<int>(m.y);
    DrawRectangle(x - 4, y, 3, 1, c);
    DrawRectangle(x + 2, y, 3, 1, c);
    DrawRectangle(x, y - 4, 1, 3, c);
    DrawRectangle(x, y + 2, 1, 3, c);
    DrawPixel(x, y, Color{255, 255, 255, 120});
}

void CenaJogo::desenharPausa() const {
    DrawRectangle(0, 0, LARGURA_VIRTUAL, ALTURA_VIRTUAL, Color{0, 0, 0, 150});
    ui::desenharPergaminho({LARGURA_VIRTUAL * 0.5f - 90.f, CENTRO_Y - 51.f, 180.f, 104.f});
    const Fonte& f = jogo_.fonte();
    f.desenharCentro("Pausa", {LARGURA_VIRTUAL * 0.5f, CENTRO_Y - 39.f}, 10.f, ui::DOURADO);
    for (int i = 0; i < 4; ++i) {
        const Rectangle r{LARGURA_VIRTUAL * 0.5f - 80.f, CENTRO_Y - 25.f + i * 18.f, 160.f, 15.f};
        if (i == opcaoPausa_) DrawRectangleRec(r, Color{90, 30, 36, 200});
        f.desenharCentro(OPCOES_PAUSA[i], {r.x + r.width * 0.5f, r.y + 8.f}, 10.f, i == opcaoPausa_ ? ui::DOURADO : ui::OSSO);
    }
    const int minutos = static_cast<int>(sessao_->tempoJogo / 60.0);
    f.desenharCentro("Tempo de jogo: " + std::to_string(minutos / 60) + "h " + std::to_string(minutos % 60) + "min",
                     {LARGURA_VIRTUAL * 0.5f, CENTRO_Y + 65.f}, 10.f, ui::CINZA_TEXTO);
}

void CenaJogo::desenharMorte() const {
    const float a = std::min(1.f, tempoMorte_ / 1.2f);
    DrawRectangle(0, 0, LARGURA_VIRTUAL, ALTURA_VIRTUAL, Color{20, 0, 4, static_cast<unsigned char>(170 * a)});
    const Fonte& f = jogo_.fonte();
    f.desenharCentro("VOCÊ PERECEU", {LARGURA_VIRTUAL * 0.5f, CENTRO_Y - 15.f}, 20.f, Fade(ui::SANGUE, a));
    if (tempoMorte_ > 1.5f)
        f.desenharCentro("Pressione Enter para despertar na Vila de Cinzavale", {LARGURA_VIRTUAL * 0.5f, CENTRO_Y + 15.f}, 10.f,
                         Fade(ui::OSSO, 0.6f + 0.4f * std::sin(tempoMorte_ * 3.f)));
}

void CenaJogo::prepararQuadro() {
    // 1) Luzes -> lightmap (render target próprio, fora da tela virtual).
    luz_.renderizar(camera_.camera());
}

void CenaJogo::desenhar() {
    // 2) Mundo com Y-sort.  3) Escuridão + brilho por cima.  4) UI.
    BeginMode2D(camera_.camera());
    mundo_->desenhar();
    EndMode2D();
    luz_.aplicar();

    const Fonte& f = jogo_.fonte();
    if (const ObjetoMapa* t = mundo_->transicaoProxima(); t && modo_ == Modo::Jogando) {
        const Vector2 tela = GetWorldToScreen2D(t->pos, camera_.camera());
        f.desenharCentro(t->rotulo, {tela.x, tela.y - 12.f}, 10.f, Fade(ui::DOURADO, 0.85f));
    }

    // "[E] Conversar" em coordenadas de tela (texto no mundo dobraria com o zoom).
    if (modo_ == Modo::Jogando && jogador_.vivo())
        if (const NPC* npc = mundo_->npcProximo(jogador_.pos, 24.f)) {
            const Vector2 tela = GetWorldToScreen2D({npc->pos.x, npc->pos.y + 4.f}, camera_.camera());
            f.desenharCentro("[E] Conversar", {tela.x, tela.y + 6.f}, 10.f, ui::OSSO);
        }

    hud_.desenhar(f, jogador_, *sessao_, jogo_.vfx());
    notificacoes_->desenhar(f);

    if (tempoAnuncio_ > 0.f) {
        const float a = std::min(1.f, std::min(tempoAnuncio_, 3.f - tempoAnuncio_) * 2.f);
        f.desenharCentro(areaAnunciada_, {LARGURA_VIRTUAL * 0.5f, 74.f}, 20.f, Fade(ui::OSSO, a));
        DrawRectangle(LARGURA_VIRTUAL / 2 - 60, 88, 120, 1, Fade(ui::DOURADO, a));
    }

    if (modo_ == Modo::Dialogo) dialogo_->desenhar(f);
    if (modo_ == Modo::Arvore) arvore_.desenhar(f, *sessao_, jogo_.vfx());
    if (modo_ == Modo::Inventario) inventario_.desenhar(f, *sessao_, jogador_);
    if (modo_ == Modo::Pausa) desenharPausa();
    if (modo_ == Modo::Morto) desenharMorte();
    if (fade_ > 0.f) DrawRectangle(0, 0, LARGURA_VIRTUAL, ALTURA_VIRTUAL, Color{0, 0, 0, static_cast<unsigned char>(255 * fade_)});
    if (modo_ == Modo::Jogando) desenharMira(jogo_.mouseVirtual());
}

} // namespace andv

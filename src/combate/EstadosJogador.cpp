// Estados do protagonista — o coração do "Ciclo de Sinergia Sangue-Arcano".
#include "andvarion/combate/EstadosJogador.hpp"
#include "andvarion/cenas/Mundo.hpp"
#include "andvarion/cenas/Sessao.hpp"
#include "andvarion/core/EventBus.hpp"
#include "andvarion/entidades/Jogador.hpp"
#include "andvarion/gfx/VFXManager.hpp"

#include <raymath.h>

#include <memory>

namespace andv {

namespace {
constexpr float CUSTO_ESQUIVA = 20.f;
constexpr float DURACAO_ESQUIVA = 0.22f;
constexpr float VELOCIDADE_ESQUIVA = 330.f;

Vector2 direcaoMira(const Jogador& j) {
    const Vector2 d = Vector2Subtract(j.mira, j.pos);
    return Vector2Length(d) > 0.5f ? Vector2Normalize(d) : vetorDeDirecao(j.direcao);
}

// Tenta esquivar: vale a partir de quase qualquer estado.
bool tentarEsquiva(Jogador& j, const ComandosJogador& cmd) {
    if (!cmd.esquiva || j.recargaEsquiva > 0.f) return false;
    if (!j.recursos.vigor.gastar(CUSTO_ESQUIVA)) {
        j.avisar("Vigor insuficiente para esquivar.");
        return false;
    }
    const Vector2 dir = Vector2Length(cmd.mover) > 0.f ? cmd.mover : direcaoMira(j);
    j.estados.trocar(std::make_unique<EstadoEsquiva>(dir));
    return true;
}

bool tentarFeitico(Jogador& j, Mundo& m, const ComandosJogador& cmd) {
    for (int slot = 0; slot < 4; ++slot) {
        if (!cmd.feitico[slot]) continue;
        std::string motivo;
        if (j.podeConjurar(m, slot, &motivo)) {
            j.estados.trocar(std::make_unique<EstadoConjurando>(slot, cmd.miraMundo));
            return true;
        }
        j.avisar(motivo);
    }
    return false;
}
} // namespace

// ---------------------------------------------------------------------------
// Locomoção
// ---------------------------------------------------------------------------
bool EstadoLocomocao::transicoesDeCombate(Jogador& j, Mundo& m, const ComandosJogador& cmd) {
    if (tentarEsquiva(j, cmd)) return true;
    if (tentarFeitico(j, m, cmd)) return true;
    if (cmd.laminaPressionado) {
        // Dentro da janela de combo, emenda no golpe seguinte.
        j.estados.trocar(std::make_unique<EstadoAtaqueLamina>(j.janelaCombo > 0.f ? j.proximoCombo : 0));
        return true;
    }
    if (cmd.distanciaPressionado || cmd.distanciaSegurando) {
        j.estados.trocar(std::make_unique<EstadoMirando>());
        return true;
    }
    return false;
}

void EstadoOcioso::entrar(Jogador& j, Mundo&) { j.anim.tocar("ocioso", j.direcao); }

void EstadoOcioso::atualizar(Jogador& j, Mundo& m, const ComandosJogador& cmd, float dt) {
    if (transicoesDeCombate(j, m, cmd)) return;
    if (Vector2Length(cmd.mover) > 0.f) {
        j.estados.trocar(std::make_unique<EstadoMovendo>());
        return;
    }
    j.anim.tocar("ocioso", j.direcao);
    j.anim.atualizar(dt);
}

void EstadoMovendo::entrar(Jogador& j, Mundo&) { j.anim.tocar("andar", j.direcao); }

void EstadoMovendo::atualizar(Jogador& j, Mundo& m, const ComandosJogador& cmd, float dt) {
    if (transicoesDeCombate(j, m, cmd)) return;
    if (Vector2Length(cmd.mover) <= 0.f) {
        j.estados.trocar(std::make_unique<EstadoOcioso>());
        return;
    }
    j.direcao = direcaoDeVetor(cmd.mover);
    j.anim.tocar("andar", j.direcao);   // mesma ação: a fase do passo é preservada ao virar
    j.mover(m, cmd.mover, j.velocidadeMovimento(), dt);
}

// ---------------------------------------------------------------------------
// Lâmina: combo de 3
// ---------------------------------------------------------------------------
void EstadoAtaqueLamina::entrar(Jogador& j, Mundo&) {
    const ArmaCorpoACorpo& arma = j.armaCorpo();
    // Lê a aceleração ANTES de consumir a carga da Sede de Sangue.
    multVel_ = j.sede.multiplicadorVelocidade();
    multDano_ = j.sede.consumir();
    duracao_ = arma.duracao[static_cast<std::size_t>(indice_)] / multVel_;
    dirGolpe_ = direcaoMira(j);
    j.direcao = direcaoDeVetor(dirGolpe_);
    j.comboVisual = indice_;
    static const char* nomes[3] = {"ataque1", "ataque2", "ataque3"};
    j.anim.tocar(nomes[indice_], j.direcao, true);
}

void EstadoAtaqueLamina::atualizar(Jogador& j, Mundo& m, const ComandosJogador& cmd, float dt) {
    t_ += dt;
    const ArmaCorpoACorpo& arma = j.armaCorpo();
    if (cmd.laminaPressionado) proximo_ = true;  // buffer: o clique não se perde

    // Passo à frente durante a preparação (peso no golpe).
    if (!acertou_) j.mover(m, dirGolpe_, arma.avanco * multVel_, dt);
    // O clipe da forge é esticado/comprimido para caber na duração do golpe da arma.
    const float clipe = j.anim.duracaoSeg();
    j.anim.atualizar(dt, clipe > 0.f ? clipe / duracao_ : 1.f);

    // O acerto acontece no evento "hit" da forge OU na fração ativa da arma.
    const bool momento = j.anim.disparou("hit") || t_ >= duracao_ * arma.fracaoAtivo;
    if (!acertou_ && momento) {
        acertou_ = true;
        j.executarGolpe(m, indice_, dirGolpe_, multDano_);
    }

    // Recuperação cancelável por esquiva ou feitiço (estilos combinados sem menu).
    if (acertou_) {
        if (tentarEsquiva(j, cmd)) return;
        if (tentarFeitico(j, m, cmd)) return;
        if (cmd.distanciaPressionado) {
            j.estados.trocar(std::make_unique<EstadoMirando>());
            return;
        }
    }

    if (t_ >= duracao_) {
        j.comboVisual = -1;
        if (proximo_ && indice_ < 2) {
            j.estados.trocar(std::make_unique<EstadoAtaqueLamina>(indice_ + 1));
        } else {
            j.proximoCombo = indice_ < 2 ? indice_ + 1 : 0;
            j.janelaCombo = indice_ < 2 ? arma.janelaCombo : 0.f;
            j.estados.trocar(std::make_unique<EstadoOcioso>());
        }
    }
}

// ---------------------------------------------------------------------------
// Distância: mira + carga
// ---------------------------------------------------------------------------
void EstadoMirando::entrar(Jogador& j, Mundo&) {
    j.olharPara(j.mira);
    j.anim.tocar("mirar", j.direcao, true);
    j.cargaVisual = 0.f;
}

void EstadoMirando::sair(Jogador& j, Mundo& m) {
    j.cargaVisual = -1.f;
    if (vfxCarga_) m.vfx().parar(vfxCarga_);
}

void EstadoMirando::atualizar(Jogador& j, Mundo& m, const ComandosJogador& cmd, float dt) {
    if (tentarEsquiva(j, cmd)) return;
    const ArmaDistancia& arma = j.armaDistancia();
    carga_ += dt;
    const float carga01 = std::min(1.f, carga_ / arma.tempoCarga);
    j.cargaVisual = carga01;
    j.olharPara(j.mira);
    j.anim.tocar("mirar", j.direcao);
    j.anim.atualizar(dt);
    // Anda devagar enquanto mira.
    if (Vector2Length(cmd.mover) > 0.f) j.mover(m, cmd.mover, j.velocidadeMovimento() * 0.45f, dt);

    if (carga01 >= 1.f && !avisouCarga_) {
        avisouCarga_ = true;
        ParametrosVfx p;
        p.escala = 0.15f;
        p.seguir = [&j] {
            const Vector2 d = Vector2Normalize(Vector2Subtract(j.mira, j.pos));
            return Vector2{j.pos.x + d.x * 8.f, j.pos.y - 6.f + d.y * 4.f};  // na mão, para o lado da mira
        };
        vfxCarga_ = m.vfx().tocar("carga_disparo", j.pos, p);
    }

    if (!cmd.distanciaSegurando || cmd.distanciaSoltou) {
        // Disparo rápido (toque) ou carregado (segurou até encher).
        // A Sede de Sangue só é gasta se o disparo realmente sair.
        const float mult = j.sede.ativa() ? j.sede.multDano : 1.f;
        if (j.disparar(m, j.mira, carga01, mult)) {
            j.sede.consumir();
            j.anim.tocar("disparo", j.direcao, true);
        }
        j.estados.trocar(std::make_unique<EstadoOcioso>());
    }
}

// ---------------------------------------------------------------------------
// Magia
// ---------------------------------------------------------------------------
void EstadoConjurando::entrar(Jogador& j, Mundo&) {
    j.olharPara(alvo_);
    j.anim.tocar("conjurar", j.direcao, true);
}

void EstadoConjurando::atualizar(Jogador& j, Mundo& m, const ComandosJogador& cmd, float dt) {
    t_ += dt;
    j.anim.atualizar(dt, 1.5f);
    if (!lancou_ && (t_ >= 0.12f || j.anim.disparou("release"))) {
        lancou_ = true;
        j.lancarFeitico(m, slot_, alvo_);
    }
    if (lancou_ && tentarEsquiva(j, cmd)) return;
    if (t_ >= 0.26f) j.estados.trocar(std::make_unique<EstadoOcioso>());
}

// ---------------------------------------------------------------------------
// Esquiva Sombria
// ---------------------------------------------------------------------------
void EstadoEsquiva::entrar(Jogador& j, Mundo& m) {
    j.direcao = direcaoDeVetor(dir_);
    j.anim.tocar("esquiva", j.direcao, true);
    j.iniciarEsquiva(m);
    j.criarAfterimage();
}

void EstadoEsquiva::atualizar(Jogador& j, Mundo& m, const ComandosJogador&, float dt) {
    t_ += dt;
    proximaImagem_ -= dt;
    // Desacelera no fim (ease-out): sai explosivo, termina controlado.
    const float k = 1.f - (t_ / DURACAO_ESQUIVA) * 0.6f;
    const Vector2 antes = j.pos;
    j.pos = m.mapa().moverCirculo(j.pos, j.raio, Vector2Scale(dir_, VELOCIDADE_ESQUIVA * k * dt));
    j.anim.atualizar(dt);
    if (proximaImagem_ <= 0.f) {
        proximaImagem_ = 0.03f;
        j.criarAfterimage();
    }
    // Passiva "Esquiva Cortante": atravessar inimigos os fere.
    if (j.recursos.vida.maximo > 0.f && m.sessao().arvore.temPassiva("esquiva_cortante")) {
        for (Combatente* c : m.alvosNoRaio(Vector2Lerp(antes, j.pos, 0.5f), 10.f, Faccao::Jogador)) {
            Dano d;
            d.valor = 8.f * j.multiplicadorLamina();
            d.origem = antes;
            d.empurrao = 30.f;
            d.fonte = Faccao::Jogador;
            d.lamina = true;
            if (c->invulneravel <= 0.f && c->receberDano(m, d)) {
                c->invulneravel = 0.25f;  // um corte por esquiva
                j.recursos.mana.ganhar(3.f);
            }
        }
    }
    if (t_ >= DURACAO_ESQUIVA) j.estados.trocar(std::make_unique<EstadoOcioso>());
}

// ---------------------------------------------------------------------------
// Atordoado / Morto
// ---------------------------------------------------------------------------
void EstadoAtordoado::entrar(Jogador& j, Mundo&) { j.anim.tocar("dano", j.direcao, true); }

void EstadoAtordoado::atualizar(Jogador& j, Mundo&, const ComandosJogador& cmd, float dt) {
    t_ += dt;
    j.anim.atualizar(dt);
    // Esquiva de recuperação nos últimos quadros: o jogo pune, mas não prende.
    if (t_ > duracao_ * 0.6f && tentarEsquiva(j, cmd)) return;
    if (t_ >= duracao_) j.estados.trocar(std::make_unique<EstadoOcioso>());
}

void EstadoMorto::entrar(Jogador& j, Mundo&) {
    j.anim.tocar("morte", j.direcao, true);
    j.cargaVisual = -1.f;
}

void EstadoMorto::atualizar(Jogador& j, Mundo& m, const ComandosJogador&, float dt) {
    t_ += dt;
    j.anim.atualizar(dt);
    if (t_ > 1.2f && !avisou_) {
        avisou_ = true;
        m.bus().publicar(TipoEvento::JogadorMorreu);
    }
}

} // namespace andv

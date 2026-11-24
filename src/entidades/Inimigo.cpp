#include "andvarion/entidades/Inimigo.hpp"
#include "andvarion/cenas/Mundo.hpp"
#include "andvarion/core/EventBus.hpp"
#include "andvarion/entidades/Jogador.hpp"
#include "andvarion/entidades/Projetil.hpp"
#include "andvarion/gfx/CameraJogo.hpp"
#include "andvarion/gfx/VFXManager.hpp"

#include <raymath.h>

#include <cmath>

namespace andv {

const DefinicaoInimigo* definicaoInimigo(const std::string& classe) {
    static const DefinicaoInimigo cultista = [] {
        DefinicaoInimigo d;
        d.classe = "cultista";
        d.nome = "Cultista da Catedral";
        d.sprite = "sprites/cultista";
        d.vida = 45.f;
        d.velocidade = 34.f;
        d.raioVisao = 135.f;
        d.alcance = 115.f;
        d.distanciaPreferida = 80.f;
        d.dano = 12.f;
        d.preparo = 0.7f;
        d.duracaoAtaque = 0.35f;
        d.recarga = 1.6f;
        d.distancia = true;
        d.xp = 22;
        return d;
    }();
    static const DefinicaoInimigo carnical = [] {
        DefinicaoInimigo d;
        d.classe = "carnical";
        d.nome = "Carniçal";
        d.sprite = "sprites/carnical";
        d.vida = 55.f;
        d.velocidade = 52.f;
        d.velocidadeAtaque = 190.f;
        d.raioVisao = 120.f;
        d.alcance = 34.f;
        d.dano = 14.f;
        d.preparo = 0.45f;
        d.duracaoAtaque = 0.28f;
        d.recarga = 1.0f;
        d.xp = 18;
        d.dropItem = "fragmento_ferro";
        d.dropChance = 0.65f;
        return d;
    }();
    if (classe == "cultista") return &cultista;
    if (classe == "carnical") return &carnical;
    return nullptr;
}

Inimigo::Inimigo(const DefinicaoInimigo& def, Vector2 p, std::vector<Vector2> patrulha)
    : def_(def), patrulha_(std::move(patrulha)) {
    pos = p;
    raio = 6.f;
    faccao = Faccao::Inimigo;
    vida = vidaMax = def.vida;
    if (patrulha_.empty()) patrulha_.push_back(p);
}

void Inimigo::trocar(Estado e) {
    estado_ = e;
    tempoEstado_ = 0.f;
}

void Inimigo::andarPara(Mundo& m, Vector2 alvo, float vel, float dt) {
    Vector2 d = Vector2Subtract(alvo, pos);
    const float dist = Vector2Length(d);
    if (dist < 1.f) return;
    d = Vector2Scale(d, 1.f / dist);
    const Vector2 antes = pos;
    if (lentidao > 0.f) vel *= 0.45f;
    pos = m.mapa().moverCirculo(pos, raio, Vector2Scale(d, std::min(vel * dt, dist)));
    direcao = direcaoDeVetor(d);
    anim.tocar("andar", direcao);
    anim.avancarDistancia(Vector2Distance(antes, pos), 22.f);
}

void Inimigo::executarAtaque(Mundo& m) {
    Jogador& j = m.jogador();
    if (def_.distancia) {
        // Cultista: raio sombrio mirado onde o jogador ESTAVA no início do preparo.
        const Vector2 origem{pos.x, pos.y - 13.f};
        auto p = std::make_unique<Projetil>(TipoProjetil::RaioSombrio, origem, Vector2Subtract(alvoAtaque_, origem),
                                            125.f, def_.dano, 220.f, Faccao::Inimigo);
        p->vfx = "orbe_sangue";
        p->escalaVfx = 0.12f;
        p->tinta = Color{170, 110, 255, 255};
        p->corLuz = Color{140, 80, 255, 255};
        m.adicionar(std::move(p));
    } else {
        dirBote_ = Vector2Normalize(Vector2Subtract(j.pos, pos));
        acertouBote_ = false;
    }
}

void Inimigo::morrer(Mundo& m) {
    trocar(Estado::Morrendo);
    anim.tocar("morte", direcao, true);
    ParametrosVfx p;
    p.escala = 0.35f;
    m.vfx().tocar("morte_inimigo", pos, p);
    m.recompensarXP(def_.xp);
    if (!def_.dropItem.empty() && m.aleatorio(0.f, 1.f) < def_.dropChance) m.soltarItem(def_.dropItem, pos);
    TraceLog(LOG_INFO, "ANDVARION: %s derrotado (+%d XP)", def_.nome.c_str(), def_.xp);
    m.bus().publicar(TipoEvento::InimigoMorto, def_.classe, 1);
}

bool Inimigo::receberDano(Mundo& m, const Dano& d) {
    if (estado_ == Estado::Morrendo) return false;
    if (!Combatente::receberDano(m, d)) return false;
    barraVisivel_ = 3.f;
    if (!vivo()) {
        morrer(m);
        return true;
    }
    if (d.atordoar > 0.f) {
        trocar(Estado::Atordoado);
    } else if (estado_ == Estado::Patrulha || estado_ == Estado::Recuperar) {
        trocar(Estado::Perseguir);
    } else if (estado_ == Estado::Preparar && d.valor >= 15.f) {
        atordoado = 0.3f;           // golpe forte interrompe a telegrafia
        trocar(Estado::Atordoado);
    }
    return true;
}

void Inimigo::atualizar(Mundo& m, float dt) {
    atualizarComum(m, dt);
    tempoEstado_ += dt;
    barraVisivel_ = std::max(0.f, barraVisivel_ - dt);
    Jogador& j = m.jogador();
    const float dist = Vector2Distance(pos, j.pos);
    const bool ve = j.vivo() && dist < def_.raioVisao && m.mapa().linhaDeVisao(pos, j.pos);

    switch (estado_) {
        case Estado::Patrulha: {
            if (ve) {
                trocar(Estado::Perseguir);
                break;
            }
            if (esperaPatrulha_ > 0.f) {
                esperaPatrulha_ -= dt;
                anim.tocar("ocioso", direcao);
                anim.atualizar(dt);
                break;
            }
            const Vector2 alvo = patrulha_[pontoPatrulha_];
            if (Vector2Distance(pos, alvo) < 3.f) {
                pontoPatrulha_ = (pontoPatrulha_ + 1) % patrulha_.size();
                esperaPatrulha_ = m.aleatorio(0.8f, 2.2f);
            } else {
                andarPara(m, alvo, def_.velocidade * 0.55f, dt);
            }
            break;
        }
        case Estado::Perseguir: {
            semVer_ = ve ? 0.f : semVer_ + dt;
            if (!j.vivo() || semVer_ > 3.f || dist > def_.raioVisao * 1.8f) {
                trocar(Estado::Patrulha);
                break;
            }
            if (def_.distancia) {
                if (dist < def_.distanciaPreferida * 0.7f) {
                    // Recua mantendo distância (kiting).
                    andarPara(m, Vector2Add(pos, Vector2Subtract(pos, j.pos)), def_.velocidade * 0.9f, dt);
                } else if (dist <= def_.alcance && ve && tempoEstado_ > 0.25f) {
                    trocar(Estado::Preparar);
                    alvoAtaque_ = j.pos;
                } else {
                    andarPara(m, j.pos, def_.velocidade, dt);
                }
            } else if (dist <= def_.alcance && ve) {
                trocar(Estado::Preparar);
            } else {
                andarPara(m, j.pos, def_.velocidade, dt);
            }
            break;
        }
        case Estado::Preparar: {
            direcao = direcaoDeVetor(Vector2Subtract(j.pos, pos));
            anim.tocar(def_.distancia ? "conjurar" : "ataque1", direcao, tempoEstado_ <= dt);
            anim.atualizar(dt, 0.5f);  // pose de preparação mais lenta = telegrafia legível
            if (def_.distancia) alvoAtaque_ = Vector2Lerp(alvoAtaque_, j.pos, 0.5f * dt);
            if (tempoEstado_ >= def_.preparo) {
                executarAtaque(m);
                trocar(Estado::Atacar);
            }
            break;
        }
        case Estado::Atacar: {
            anim.atualizar(dt);
            if (!def_.distancia) {
                pos = m.mapa().moverCirculo(pos, raio, Vector2Scale(dirBote_, def_.velocidadeAtaque * (lentidao > 0.f ? 0.45f : 1.f) * dt));
                if (!acertouBote_ && Vector2Distance(pos, j.pos) < raio + j.raio + 4.f) {
                    Dano d;
                    d.valor = def_.dano;
                    d.origem = pos;
                    d.empurrao = 140.f;
                    d.fonte = Faccao::Inimigo;
                    if (j.receberDano(m, d)) {
                        acertouBote_ = true;
                        ParametrosVfx p;
                        p.escala = 0.2f;
                        m.vfx().tocar("impacto_sangue", j.pos, p);
                    }
                }
            }
            if (tempoEstado_ >= def_.duracaoAtaque) trocar(Estado::Recuperar);
            break;
        }
        case Estado::Recuperar:
            anim.tocar("ocioso", direcao);
            anim.atualizar(dt);
            if (tempoEstado_ >= def_.recarga) trocar(Estado::Perseguir);
            break;
        case Estado::Atordoado:
            anim.tocar("dano", direcao);
            anim.atualizar(dt);
            if (atordoado <= 0.f && tempoEstado_ > 0.2f) trocar(Estado::Perseguir);
            break;
        case Estado::Morrendo:
            anim.atualizar(dt);
            if (tempoEstado_ > 0.7f) remover = true;
            break;
    }
}

void Inimigo::desenhar(const Mundo&) const {
    if (estado_ == Estado::Morrendo) {
        // Some enquanto o VFX de desintegração do pixelfx toma o lugar.
        const float a = std::max(0.f, 1.f - tempoEstado_ / 0.5f);
        if (anim.temSprites()) anim.desenhar(pos, Color{255, 255, 255, static_cast<unsigned char>(255 * a)});
        return;
    }
    desenharSombra(12.f);
    Color t = tinta();
    if (estado_ == Estado::Preparar) {
        // Telegrafia: pulsa em vermelho antes do golpe.
        const float pulso = 0.5f + 0.5f * std::sin(tempoEstado_ * 30.f);
        t = Color{255, static_cast<unsigned char>(255 - 140 * pulso), static_cast<unsigned char>(255 - 140 * pulso), 255};
    }
    if (anim.temSprites()) {
        anim.desenhar(pos, t);
    } else {
        const Color base = def_.distancia ? Color{120, 30, 40, 255} : Color{150, 160, 130, 255};
        DrawRectangle(static_cast<int>(pos.x) - 5, static_cast<int>(pos.y) - 18, 10, 18, ColorTint(base, t));
    }
    if (estado_ == Estado::Preparar) {
        DrawText("!", static_cast<int>(pos.x) - 1, static_cast<int>(pos.y) - 42, 10, Color{255, 60, 60, 255});
    }
    if (barraVisivel_ > 0.f && vivo()) {
        const float w = 18.f;
        DrawRectangle(static_cast<int>(pos.x - w / 2), static_cast<int>(pos.y) + 4, static_cast<int>(w), 3, Color{20, 10, 10, 200});
        DrawRectangle(static_cast<int>(pos.x - w / 2), static_cast<int>(pos.y) + 4, static_cast<int>(w * vida / vidaMax), 3,
                      Color{190, 30, 40, 255});
    }
}

} // namespace andv

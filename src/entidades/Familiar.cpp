#include "andvarion/entidades/Familiar.hpp"
#include "andvarion/cenas/Mundo.hpp"
#include "andvarion/entidades/Jogador.hpp"
#include "andvarion/entidades/Projetil.hpp"
#include "andvarion/gfx/Iluminacao.hpp"
#include "andvarion/gfx/VFXManager.hpp"

#include <raymath.h>

#include <cmath>

namespace andv {

Familiar::Familiar(float duracao, float faseOrbita, float danoTiro) : restante_(duracao), fase_(faseOrbita), dano_(danoTiro) {
    faccao = Faccao::Neutra;   // não é alvo de ninguém
    raio = 3.f;
}

void Familiar::atualizar(Mundo& m, float dt) {
    tempo_ += dt;
    restante_ -= dt;
    const Jogador& j = m.jogador();
    // Órbita elíptica sobre o ombro do jogador.
    const float a = fase_ + tempo_ * 2.2f;
    pos = {j.pos.x + std::cos(a) * 18.f, j.pos.y - 6.f + std::sin(a) * 8.f};
    if (!luz_) {
        Luz l;
        l.pos = pos;
        l.raio = 34.f;
        l.cor = {140, 110, 255, 255};
        l.intensidade = 0.5f;
        l.brilho = 0.35f;
        l.oscilacao = 0.3f;
        luz_ = m.luz().adicionar(l);
    }
    m.luz().mover(luz_, {pos.x, pos.y - 14.f});

    proximoTiro_ -= dt;
    if (proximoTiro_ <= 0.f) {
        // Alvo: inimigo mais próximo do jogador, até 110 unidades e com linha de visão.
        Combatente* alvo = nullptr;
        float menor = 110.f;
        for (Combatente* c : m.alvosNoRaio(j.pos, 110.f, Faccao::Jogador)) {
            const float d = Vector2Distance(c->pos, j.pos);
            if (d < menor && m.mapa().linhaDeVisao(pos, c->pos)) {
                menor = d;
                alvo = c;
            }
        }
        if (alvo) {
            proximoTiro_ = 1.1f;
            const Vector2 origem{pos.x, pos.y - 14.f};
            auto p = std::make_unique<Projetil>(TipoProjetil::Familiar, origem,
                                                Vector2Subtract({alvo->pos.x, alvo->pos.y - 8.f}, origem), 190.f, dano_, 160.f, Faccao::Jogador);
            p->vfx = "familiar_tiro";
            p->escalaVfx = 0.12f;
            p->vfxImpacto = "relampago_impacto";
            p->proficiencia = "arcano";
            m.adicionar(std::move(p));
        } else {
            proximoTiro_ = 0.3f;
        }
    }
    if (restante_ <= 0.f) {
        ParametrosVfx p;
        p.escala = 0.2f;
        m.vfx().tocar("teleporte_saida", pos, p);
        m.luz().remover(luz_);
        remover = true;
    }
}

void Familiar::desenhar(const Mundo& m) const {
    ParametrosVfx p;
    p.escala = 0.16f;
    // Pisca nos últimos 2 s para avisar que vai sumir.
    if (restante_ < 2.f && std::fmod(restante_, 0.25f) < 0.12f) p.tinta = Color{255, 255, 255, 90};
    m.vfx().desenharQuadro("familiar", tempo_, {pos.x, pos.y}, p);
}

} // namespace andv

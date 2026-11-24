#include "andvarion/entidades/Projetil.hpp"
#include "andvarion/cenas/Mundo.hpp"
#include "andvarion/entidades/Jogador.hpp"
#include "andvarion/gfx/CameraJogo.hpp"
#include "andvarion/gfx/Iluminacao.hpp"
#include "andvarion/gfx/VFXManager.hpp"

#include <raymath.h>

#include <algorithm>
#include <cmath>

namespace andv {

Projetil::Projetil(TipoProjetil tipo, Vector2 p, Vector2 dir, float velocidade, float dano, float alcance, Faccao f)
    : tipo_(tipo), dir_(Vector2Normalize(dir)), velocidade_(velocidade), dano_(dano), alcance_(alcance) {
    pos = p;
    faccao = f;
    raio = (tipo == TipoProjetil::OrbeSangue || tipo == TipoProjetil::BolaFogo) ? 5.f : 3.f;
}

void Projetil::destruir(Mundo& m) {
    if (luz_) m.luz().remover(luz_);
    luz_ = 0;
    remover = true;
    if (tipo_ == TipoProjetil::Bumerangue) m.jogador().bumerangueEmVoo = false;
}

void Projetil::explodir(Mundo& m) {
    if (raioExplosao > 0.f) {
        for (Combatente* alvo : m.alvosNoRaio(pos, raioExplosao, faccao)) {
            Dano d;
            d.valor = dano_;
            d.origem = pos;
            d.empurrao = empurrao * 2.f;
            d.fonte = faccao;
            if (alvo->receberDano(m, d) && !proficiencia.empty() && faccao == Faccao::Jogador)
                m.jogador().ganharProficiencia(m, proficiencia, 1);
        }
        m.camera().tremer(0.35f);
        m.hitStop(0.05f);
    }
    if (!vfxExplosao.empty()) {
        ParametrosVfx p;
        p.escala = escalaExplosao;
        m.vfx().tocar(vfxExplosao, pos, p);
    }
    if (corLuz.a) m.luz().flash(pos, 70.f, corLuz, 0.35f);
}

void Projetil::atualizar(Mundo& m, float dt) {
    tempo_ += dt;
    if (corLuz.a && !luz_) {
        Luz l;
        l.pos = pos;
        l.raio = 42.f;
        l.cor = corLuz;
        l.intensidade = 0.7f;
        l.brilho = 0.45f;
        luz_ = m.luz().adicionar(l);
    }

    // --- movimento ---
    Vector2 velocidade = Vector2Scale(dir_, velocidade_);
    if (tipo_ == TipoProjetil::Bumerangue) {
        giro_ += dt * 1080.f;
        if (voltando_) {
            const Vector2 paraJogador = Vector2Subtract(m.jogador().pos, pos);
            if (Vector2Length(paraJogador) < 8.f) {
                destruir(m);
                return;
            }
            dir_ = Vector2Normalize(paraJogador);
            velocidade = Vector2Scale(dir_, velocidade_ * 1.15f);
        } else {
            // Desacelera perto do ponto de retorno (arco natural).
            const float resta = std::max(0.f, 1.f - percorrido_ / alcance_);
            velocidade = Vector2Scale(dir_, velocidade_ * (0.35f + 0.65f * resta));
        }
    }
    const Vector2 passo = Vector2Scale(velocidade, dt);
    const Vector2 novo = Vector2Add(pos, passo);
    percorrido_ += Vector2Length(passo);

    // --- parede ---
    if (m.mapa().solidoEm(novo)) {
        if (tipo_ == TipoProjetil::Bumerangue && !voltando_) {
            voltando_ = true;
            atingidos_.clear();
        } else if (tipo_ != TipoProjetil::Bumerangue) {
            explodir(m);
            ParametrosVfx p;
            p.escala = 0.18f;
            p.anguloGraus = std::atan2(dir_.y, dir_.x) * RAD2DEG + 180.f;
            p.orientar = true;
            if (raioExplosao <= 0.f) m.vfx().tocar(vfxImpacto, pos, p);
            destruir(m);
            return;
        }
    } else {
        pos = novo;
    }
    if (luz_) m.luz().mover(luz_, pos);

    // --- alcance ---
    if (percorrido_ >= alcance_) {
        if (tipo_ == TipoProjetil::Bumerangue) {
            if (!voltando_) {
                voltando_ = true;
                atingidos_.clear();
            } else if (percorrido_ > alcance_ * 4.f) {
                destruir(m);  // segurança: nunca fica preso para sempre
                return;
            }
        } else {
            if (raioExplosao > 0.f) explodir(m);
            destruir(m);
            return;
        }
    }

    // --- acerto ---
    for (Combatente* alvo : m.alvosNoRaio(pos, raio + 4.f, faccao)) {
        if (std::find(atingidos_.begin(), atingidos_.end(), alvo) != atingidos_.end()) continue;
        atingidos_.push_back(alvo);
        Dano d;
        d.valor = dano_;
        d.origem = Vector2Subtract(pos, dir_);
        d.empurrao = empurrao;
        d.fonte = faccao;
        d.lentidao = lentidao;
        if (alvo->receberDano(m, d)) {
            ParametrosVfx p;
            p.escala = 0.2f;
            p.anguloGraus = std::atan2(dir_.y, dir_.x) * RAD2DEG;
            p.orientar = true;
            m.vfx().tocar(vfxImpacto, alvo->pos, p);
            m.camera().tremer(0.12f);
            if (!proficiencia.empty() && faccao == Faccao::Jogador) m.jogador().ganharProficiencia(m, proficiencia, 1);
        }
        if (raioExplosao > 0.f) {
            explodir(m);
            destruir(m);
            return;
        }
        if (!perfurante && tipo_ != TipoProjetil::Bumerangue) {
            destruir(m);
            return;
        }
    }
}

void Projetil::desenhar(const Mundo& m) const {
    ParametrosVfx p;
    p.escala = escalaVfx;
    p.tinta = tinta;
    if (tipo_ == TipoProjetil::Bumerangue) {
        p.anguloGraus = giro_;
    } else {
        p.anguloGraus = std::atan2(dir_.y, dir_.x) * RAD2DEG;
        p.orientar = true;
    }
    if (m.vfx().existe(vfx)) {
        // Sombra no chão dá a leitura de altura.
        DrawEllipse(static_cast<int>(pos.x), static_cast<int>(pos.y + 6), 3.f, 1.2f, Color{0, 0, 0, 70});
        m.vfx().desenharQuadro(vfx, tempo_, pos, p);
    } else {
        DrawCircleV(pos, raio, tinta);  // sem assets: ainda jogável
    }
}

} // namespace andv

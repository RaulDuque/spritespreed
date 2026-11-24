#include "andvarion/entidades/AreaDano.hpp"
#include "andvarion/cenas/Mundo.hpp"
#include "andvarion/gfx/Iluminacao.hpp"
#include "andvarion/gfx/VFXManager.hpp"

#include <cmath>

namespace andv {

AreaDano::AreaDano(Vector2 centro, float raio, float duracao, float intervalo, float danoPorTick, Faccao f)
    : raioArea_(raio), duracao_(duracao), intervalo_(intervalo), dano_(danoPorTick) {
    pos = centro;
    faccao = f;
}

void AreaDano::atualizar(Mundo& m, float dt) {
    if (tempo_ == 0.f) {
        ParametrosVfx p;
        p.escala = escalaVfx;
        p.camada = CamadaVfx::Chao;
        p.duracaoMax = duracao_;
        vfxId_ = m.vfx().tocar(vfx, pos, p);
        if (corLuz.a) {
            Luz l;
            l.pos = pos;
            l.raio = raioArea_ * 1.8f;
            l.cor = corLuz;
            l.intensidade = 0.45f;
            l.brilho = 0.25f;
            l.oscilacao = 0.4f;
            luz_ = m.luz().adicionar(l);
        }
    }
    tempo_ += dt;
    proximoTick_ -= dt;
    if (proximoTick_ <= 0.f) {
        proximoTick_ = intervalo_;
        for (Combatente* alvo : m.alvosNoRaio(pos, raioArea_, faccao)) {
            Dano d;
            d.valor = dano_;
            d.origem = pos;
            d.fonte = faccao;
            d.atordoar = atordoar;
            alvo->receberDano(m, d);
        }
    }
    if (tempo_ >= duracao_) {
        if (vfxId_) m.vfx().parar(vfxId_);
        if (luz_) m.luz().remover(luz_);
        remover = true;
    }
}

void AreaDano::desenhar(const Mundo&) const {
    // Contorno sutil no chão: o jogador lê o tamanho da área mesmo no escuro.
    const float a = std::fmin(1.f, std::fmin(tempo_ * 4.f, (duracao_ - tempo_) * 4.f));
    DrawCircleLines(static_cast<int>(pos.x), static_cast<int>(pos.y), raioArea_,
                    Color{150, 60, 200, static_cast<unsigned char>(60 * a)});
}

} // namespace andv

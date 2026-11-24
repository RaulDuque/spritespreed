#pragma once
// Recursos de combate (Vida, Mana, Vigor) e o buff "Sede de Sangue".
//
// Ciclo de Sinergia Sangue-Arcano:
//   Lâmina acerta  -> drena essência  -> +Mana
//   Mana conjurada -> Sede de Sangue  -> próximo golpe/disparo com +dano e +velocidade
//   Distância      -> consome Vigor   (finaliza alvos de longe)
#include "andvarion/core/Plataforma.hpp"

#include <algorithm>

namespace andv {

struct Recurso {
    float atual = 100.f;
    float maximo = 100.f;
    float regenPorSeg = 0.f;
    float atrasoRegen = 0.f;     // após gastar, espera isso antes de regenerar
    float esperaRestante = 0.f;

    float fracao() const { return maximo > 0.f ? atual / maximo : 0.f; }
    bool  tem(float q) const { return atual >= q; }
    bool  gastar(float q);
    void  ganhar(float q) { atual = std::min(maximo, atual + q); }
    void  atualizar(float dt);
    void  definirMaximo(float novoMax, bool encher);
};

struct Recursos {
    Recurso vida{120.f, 120.f, 0.f, 0.f};
    Recurso mana{60.f, 60.f, 1.5f, 1.0f};      // mana regenera devagar: o caminho é cortar
    Recurso vigor{100.f, 100.f, 28.f, 0.8f};

    void atualizar(float dt) {
        vida.atualizar(dt);
        mana.atualizar(dt);
        vigor.atualizar(dt);
    }
};

class SedeDeSangue {
public:
    // Chamado ao conjurar: arma o bônus para os próximos `cargas` ataques.
    void ativar(float duracao, i32 cargas);
    void atualizar(float dt);

    bool ativa() const { return cargas_ > 0 && restante_ > 0.f; }
    float restante() const { return restante_; }
    i32 cargas() const { return cargas_; }

    // Consome uma carga ao iniciar um golpe de Lâmina ou disparo. Devolve o
    // multiplicador de dano (1 se inativa). Enquanto ativa, também acelera.
    float consumir();
    float multiplicadorVelocidade() const { return ativa() ? multVelocidade : 1.f; }

    float multDano = 1.6f;
    float multVelocidade = 1.35f;

private:
    float restante_ = 0.f;
    i32 cargas_ = 0;
};

} // namespace andv

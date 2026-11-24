#include "andvarion/combate/Recursos.hpp"

namespace andv {

bool Recurso::gastar(float q) {
    if (atual < q) return false;
    atual -= q;
    esperaRestante = atrasoRegen;
    return true;
}

void Recurso::atualizar(float dt) {
    if (esperaRestante > 0.f) {
        esperaRestante -= dt;
        return;
    }
    atual = std::min(maximo, atual + regenPorSeg * dt);
}

void Recurso::definirMaximo(float novoMax, bool encher) {
    maximo = std::max(1.f, novoMax);
    atual = encher ? maximo : std::min(atual, maximo);
}

void SedeDeSangue::ativar(float duracao, i32 cargas) {
    restante_ = std::max(restante_, duracao);
    cargas_ = std::max(cargas_, cargas);
}

void SedeDeSangue::atualizar(float dt) {
    if (restante_ > 0.f) {
        restante_ -= dt;
        if (restante_ <= 0.f) {
            restante_ = 0.f;
            cargas_ = 0;
        }
    }
}

float SedeDeSangue::consumir() {
    if (!ativa()) return 1.f;
    // Quem consome lê multiplicadorVelocidade() ANTES, e guarda os dois
    // multiplicadores para a ação inteira (golpe ou disparo).
    if (--cargas_ <= 0) {
        cargas_ = 0;
        restante_ = 0.f;
    }
    return multDano;
}

} // namespace andv

#include "andvarion/progressao/Progressao.hpp"

#include <algorithm>
#include <cmath>

namespace andv {

i32 Progressao::xpParaProximo(i32 nivel) {
    nivel = std::max(1, nivel);
    return static_cast<i32>(std::lround(40.0 * std::pow(static_cast<double>(nivel), 1.5)));
}

i32 Progressao::ganharXP(i32 quantidade) {
    if (quantidade <= 0 || nivel_ >= NIVEL_MAXIMO) return 0;
    xp_ += quantidade;
    i32 ganhos = 0;
    while (nivel_ < NIVEL_MAXIMO && xp_ >= xpParaProximo(nivel_)) {
        xp_ -= xpParaProximo(nivel_);
        ++nivel_;
        pontos_ += PONTOS_POR_NIVEL;
        ++ganhos;
    }
    if (nivel_ >= NIVEL_MAXIMO) xp_ = 0;
    return ganhos;
}

bool Progressao::gastarPontos(i32 custo) {
    if (custo < 0 || pontos_ < custo) return false;
    pontos_ -= custo;
    return true;
}

float Progressao::fracaoNivel() const {
    if (nivel_ >= NIVEL_MAXIMO) return 1.f;
    return static_cast<float>(xp_) / static_cast<float>(xpParaProximo(nivel_));
}

void Progressao::restaurar(i32 nivel, i32 xp, i32 pontos) {
    nivel_  = std::clamp(nivel, 1, NIVEL_MAXIMO);
    xp_     = std::max(0, xp);
    pontos_ = std::max(0, pontos);
}

} // namespace andv

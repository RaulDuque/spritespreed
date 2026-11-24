#pragma once
// Progressao — XP, níveis e pontos de habilidade.
#include "andvarion/core/Plataforma.hpp"

namespace andv {

class Progressao {
public:
    static constexpr i32 NIVEL_MAXIMO = 30;
    static constexpr i32 PONTOS_POR_NIVEL = 1;

    // XP necessário para ir do nível n para n+1 (curva 40 * n^1.5, arredondada).
    static i32 xpParaProximo(i32 nivel);

    // Soma XP; devolve quantos níveis foram ganhos (0 se nenhum).
    i32 ganharXP(i32 quantidade);

    bool gastarPontos(i32 custo);

    i32 nivel() const { return nivel_; }
    i32 xp() const { return xp_; }               // XP dentro do nível atual
    i32 pontos() const { return pontos_; }
    float fracaoNivel() const;                   // 0..1 para a barra de XP

    void restaurar(i32 nivel, i32 xp, i32 pontos);

private:
    i32 nivel_ = 1;
    i32 xp_ = 0;
    i32 pontos_ = 0;
};

} // namespace andv

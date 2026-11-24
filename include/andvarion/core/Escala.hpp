#pragma once
// Escala — relação entre unidades de mundo, pixels de arte e pixels de tela.
//
//   * A lógica (colisão, alcance, velocidade, luzes) usa UNIDADES DE MUNDO:
//     um tile = 16 unidades. Isso não mudou com o aumento de proporção.
//   * A câmera do mundo tem ZOOM 2: 1 unidade = 2 px da tela virtual (640x360).
//   * A arte (tiles, personagens) é desenhada em RESOLUÇÃO DOBRADA (tile de 32 px,
//     personagem ~55 px em quadros 48x64) e pintada com escala 1/2 no mundo, ou
//     seja, 1 px de arte = 1 px de tela: nítido, sem reamostragem.
//   * Posições de desenho são arredondadas para a grade de 1/ZOOM unidade
//     (= 1 px de tela), nunca para unidades inteiras.
#include <cmath>

namespace andv {

inline constexpr float ZOOM_MUNDO = 2.f;                  // px de tela por unidade de mundo
inline constexpr float ESCALA_ARTE = 2.f;                 // px de arte por unidade de mundo
inline constexpr float ESCALA_DESENHO = 1.f / ESCALA_ARTE; // escala ao pintar arte no mundo

// Arredonda para o pixel de tela mais próximo (em unidades de mundo).
inline float snapTela(float v) { return std::round(v * ZOOM_MUNDO) / ZOOM_MUNDO; }

} // namespace andv

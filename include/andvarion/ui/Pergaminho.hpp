#pragma once
// Pergaminho — painel de "pergaminho escuro" usado por diálogo, menus e tooltips.
#include <raylib.h>

namespace andv::ui {

// Paleta gótica compartilhada pela interface.
inline constexpr Color PERGAMINHO_FUNDO{34, 24, 20, 238};
inline constexpr Color PERGAMINHO_BORDA{122, 92, 58, 255};
inline constexpr Color PERGAMINHO_BORDA_ESCURA{58, 40, 28, 255};
inline constexpr Color DOURADO{222, 186, 112, 255};
inline constexpr Color SANGUE{196, 38, 48, 255};
inline constexpr Color ARCANO{124, 92, 220, 255};
inline constexpr Color OSSO{232, 222, 200, 255};
inline constexpr Color CINZA_TEXTO{160, 150, 140, 255};

// Triângulo preenchido em qualquer ordem de vértices (a raylib descarta a ordem horária).
void triangulo(Vector2 a, Vector2 b, Vector2 c, Color cor);
// Painel com moldura dupla, cantos rebitados e textura de fibras.
void desenharPergaminho(Rectangle r, unsigned char alfa = 238);
// Barra de recurso (vida, mana, vigor) com "fantasma" do valor anterior.
void desenharBarra(Rectangle r, float fracao, float fracaoFantasma, Color cor, Color fundo = {20, 12, 14, 220});

} // namespace andv::ui

#include "andvarion/ui/Pergaminho.hpp"

#include <algorithm>

namespace andv::ui {

void triangulo(Vector2 a, Vector2 b, Vector2 c, Color cor) {
    const float area = (b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y);
    if (area > 0.f) DrawTriangle(a, c, b, cor);
    else DrawTriangle(a, b, c, cor);
}

void desenharPergaminho(Rectangle r, unsigned char alfa) {
    Color fundo = PERGAMINHO_FUNDO;
    fundo.a = alfa;
    DrawRectangleRec(r, fundo);
    // Fibras do pergaminho: linhas horizontais irregulares, determinísticas.
    for (int y = 3; y < static_cast<int>(r.height) - 3; y += 3) {
        const int semente = (y * 37) % 11;
        const int x0 = static_cast<int>(r.x) + 4 + semente;
        const int w = static_cast<int>(r.width) - 8 - semente * 2;
        if (w > 0) DrawRectangle(x0, static_cast<int>(r.y) + y, w, 1, Color{50, 36, 28, static_cast<unsigned char>(alfa / 3)});
    }
    DrawRectangleLinesEx(r, 1.f, PERGAMINHO_BORDA_ESCURA);
    DrawRectangleLinesEx({r.x + 2, r.y + 2, r.width - 4, r.height - 4}, 1.f, PERGAMINHO_BORDA);
    // Rebites nos cantos.
    const Rectangle cantos[4] = {{r.x + 1, r.y + 1, 3, 3}, {r.x + r.width - 4, r.y + 1, 3, 3},
                                 {r.x + 1, r.y + r.height - 4, 3, 3}, {r.x + r.width - 4, r.y + r.height - 4, 3, 3}};
    for (const auto& c : cantos) DrawRectangleRec(c, DOURADO);
}

void desenharBarra(Rectangle r, float fracao, float fantasma, Color cor, Color fundo) {
    fracao = std::clamp(fracao, 0.f, 1.f);
    fantasma = std::clamp(fantasma, 0.f, 1.f);
    DrawRectangleRec(r, fundo);
    if (fantasma > fracao)
        DrawRectangleRec({r.x + 1, r.y + 1, (r.width - 2) * fantasma, r.height - 2}, Color{240, 230, 210, 200});
    DrawRectangleRec({r.x + 1, r.y + 1, (r.width - 2) * fracao, r.height - 2}, cor);
    // Brilho superior de 1 px.
    DrawRectangleRec({r.x + 1, r.y + 1, (r.width - 2) * fracao, 1}, ColorBrightness(cor, 0.35f));
    DrawRectangleLinesEx(r, 1.f, Color{0, 0, 0, 200});
}

} // namespace andv::ui

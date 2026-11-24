#include "andvarion/ui/Fonte.hpp"
#include "andvarion/core/Assets.hpp"

#include <cmath>
#include <vector>

namespace andv {

Fonte::Fonte(Assets& assets) {
    const std::string ttf = assets.caminho("fontes/principal.ttf");
    if (FileExists(ttf.c_str())) {
        std::vector<int> cps;
        for (int c = 32; c < 127; ++c) cps.push_back(c);
        for (int c = 160; c < 256; ++c) cps.push_back(c);  // Latin-1: acentos do português
        for (int c : {0x2014, 0x2026, 0x201C, 0x201D}) cps.push_back(c);  // — … “ ”
        fonte_ = LoadFontEx(ttf.c_str(), 16, cps.data(), static_cast<int>(cps.size()));
        propria_ = fonte_.texture.id != 0;
        if (propria_) SetTextureFilter(fonte_.texture, TEXTURE_FILTER_POINT);
    }
    if (!propria_) fonte_ = GetFontDefault();
}

Fonte::~Fonte() {
    if (propria_ && IsWindowReady()) UnloadFont(fonte_);
}

void Fonte::desenhar(const std::string& texto, Vector2 pos, float tamanho, Color cor, bool sombra) const {
    pos = {std::round(pos.x), std::round(pos.y)};
    if (sombra) DrawTextEx(fonte_, texto.c_str(), {pos.x + 1, pos.y + 1}, tamanho, espacamento(tamanho), Color{0, 0, 0, static_cast<unsigned char>(cor.a * 0.8f)});
    DrawTextEx(fonte_, texto.c_str(), pos, tamanho, espacamento(tamanho), cor);
}

void Fonte::desenharCentro(const std::string& texto, Vector2 centro, float tamanho, Color cor, bool sombra) const {
    desenhar(texto, {centro.x - largura(texto, tamanho) * 0.5f, centro.y - tamanho * 0.5f}, tamanho, cor, sombra);
}

float Fonte::largura(const std::string& texto, float tamanho) const {
    return MeasureTextEx(fonte_, texto.c_str(), tamanho, espacamento(tamanho)).x;
}

} // namespace andv

#pragma once
// Fonte — texto PT-BR com acentos.
//
// Se existir assets/fontes/principal.ttf, carrega com os codepoints Latin-1
// (á, ã, ç, é, ê, õ, ú...). Senão usa a fonte padrão da raylib, que já cobre
// ISO-8859-1. Tamanhos múltiplos de 10 ficam nítidos na tela virtual.
#include "andvarion/core/Plataforma.hpp"

#include <raylib.h>

#include <string>

namespace andv {

class Assets;

class Fonte {
public:
    explicit Fonte(Assets& assets);
    ~Fonte();
    Fonte(const Fonte&) = delete;
    Fonte& operator=(const Fonte&) = delete;

    void desenhar(const std::string& texto, Vector2 pos, float tamanho = 10.f, Color cor = RAYWHITE,
                  bool sombra = true) const;
    void desenharCentro(const std::string& texto, Vector2 centro, float tamanho = 10.f, Color cor = RAYWHITE,
                        bool sombra = true) const;
    float largura(const std::string& texto, float tamanho = 10.f) const;
    const Font& get() const { return fonte_; }

private:
    float espacamento(float tamanho) const { return propria_ ? 0.f : tamanho / 10.f; }
    Font fonte_{};
    bool propria_ = false;
};

} // namespace andv

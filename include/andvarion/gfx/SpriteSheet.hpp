#pragma once
// SpriteSheet = AtlasAseprite (metadados) + textura na GPU.
// ConjuntoSprites = todas as páginas de um personagem do agent-sprite-forge.
#include "andvarion/gfx/AtlasAseprite.hpp"

#include <raylib.h>

#include <memory>
#include <vector>

namespace andv {

class Textura;

class SpriteSheet {
public:
    SpriteSheet(AtlasAseprite atlas, std::shared_ptr<Textura> textura);

    const AtlasAseprite& atlas() const { return atlas_; }
    const Texture2D& textura() const;
    bool valida() const;

    Rectangle retangulo(i32 indiceQuadro) const;

    // Desenha o quadro com o pivô (âncora) em `pos`. `espelhar` reflete em
    // torno do x da âncora; `rotacaoGraus` gira em torno da âncora.
    void desenhar(i32 indiceQuadro, Vector2 pos, float escala = 1.f, bool espelhar = false,
                  float rotacaoGraus = 0.f, Color tinta = WHITE) const;

private:
    AtlasAseprite atlas_;
    std::shared_ptr<Textura> textura_;
};

class ConjuntoSprites {
public:
    void adicionar(std::shared_ptr<const SpriteSheet> pagina) { paginas_.push_back(std::move(pagina)); }

    // Procura um clipe em todas as páginas.
    struct Achado {
        const SpriteSheet* sheet = nullptr;
        const ClipeAtlas* clipe = nullptr;
    };
    Achado buscar(const std::string& nomeClipe) const;
    bool vazio() const { return paginas_.empty(); }
    const std::vector<std::shared_ptr<const SpriteSheet>>& paginas() const { return paginas_; }

private:
    std::vector<std::shared_ptr<const SpriteSheet>> paginas_;
};

} // namespace andv

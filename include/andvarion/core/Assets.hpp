#pragma once
// Assets — cache de texturas e sprite sheets com posse compartilhada.
//
// Texturas vivem em std::shared_ptr<Textura> com destrutor RAII (UnloadTexture);
// quem precisa de uma sheet guarda o shared_ptr e o cache nunca devolve ponteiro
// pendurado. Os caminhos são relativos à pasta assets/ ao lado do executável.
#include "andvarion/core/Plataforma.hpp"

#include <raylib.h>

#include <memory>
#include <string>
#include <unordered_map>

namespace andv {

class SpriteSheet;
class ConjuntoSprites;

// Dono RAII de uma Texture2D.
class Textura {
public:
    explicit Textura(Texture2D t) : tex_(t) {}
    ~Textura();
    Textura(const Textura&) = delete;
    Textura& operator=(const Textura&) = delete;

    const Texture2D& get() const { return tex_; }
    bool valida() const { return tex_.id != 0; }

private:
    Texture2D tex_;
};

class Assets {
public:
    Assets();

    // "vfx/tocha.json" -> "<raiz>/assets/vfx/tocha.json"
    std::string caminho(const std::string& relativo) const;
    bool existe(const std::string& relativo) const;
    std::string lerTexto(const std::string& relativo) const;  // vazio se faltar

    std::shared_ptr<Textura> textura(const std::string& relativo);
    // Uma página de atlas Aseprite (JSON + PNG ao lado).
    std::shared_ptr<const SpriteSheet> sheet(const std::string& jsonRelativo);
    // Todas as páginas de um personagem exportado pelo agent-sprite-forge
    // (todos os .json de assets/<pasta>/, exceto engine-export.json).
    std::shared_ptr<const ConjuntoSprites> personagem(const std::string& pastaRelativa);

    void limpar();

private:
    std::string raiz_;
    std::unordered_map<std::string, std::shared_ptr<Textura>> texturas_;
    std::unordered_map<std::string, std::shared_ptr<const SpriteSheet>> sheets_;
    std::unordered_map<std::string, std::shared_ptr<const ConjuntoSprites>> personagens_;
};

} // namespace andv

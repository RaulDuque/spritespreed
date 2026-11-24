#pragma once
// MapaTiles — mapa por tileset carregado de JSON (matriz com legenda) ou CSV.
//
// Camadas:
//   chao       desenhada primeiro, nunca colide
//   paredes    desenhada com o chão, sempre sólida
//   decoracao  entra no Y-sort (lápides, pilares, árvores); sólida se o tile
//              estiver em "solidos". Com "alto", desenha também o tile acima no
//              tileset (objetos de 16x32).
//
// Formatos aceitos para as camadas (ver docs/ARQUITETURA.md):
//   "linhas": ["####", "#..#"] + "legenda": {"#": {"chao":1,"parede":9}, ...}
//   "camadas": {"chao": [[1,1,...],...] | "mapas/x_chao.csv", ...}   (-1 = vazio)
//
// Objetos (spawn, tocha, luz, npc, inimigo, item, transicao, vfx) ficam em
// "objetos", com x/y em TILES (frações permitidas); aqui viram pixels.
#include "andvarion/core/Plataforma.hpp"

#include <raylib.h>

#include <memory>
#include <string>
#include <vector>

namespace andv {

class Assets;
class Textura;
class FilaRender;

struct ObjetoMapa {
    std::string tipo;               // "spawn", "tocha", "npc", ...
    std::string id, classe, nome, destino, spawn, rotulo, sprite, chave, uid, direcao;
    Vector2 pos{};                  // pixels (centro do tile se x/y inteiros)
    Rectangle area{};               // transições
    float raio = 0.f, escala = 1.f;
    Color cor{255, 170, 80, 255};
    std::vector<Vector2> patrulha;  // pixels
};

class MapaTiles {
public:
    static constexpr i32 VAZIO = -1;

    bool carregar(Assets& assets, const std::string& jsonRelativo, std::string* erro = nullptr);

    const std::string& id() const { return id_; }
    const std::string& nome() const { return nome_; }
    bool areaSegura() const { return seguro_; }
    Color ambiente() const { return ambiente_; }
    i32 largura() const { return largura_; }
    i32 altura() const { return altura_; }
    i32 tamanhoTile() const { return tile_; }
    Rectangle limitesPx() const { return {0, 0, static_cast<float>(largura_ * tile_), static_cast<float>(altura_ * tile_)}; }
    const std::vector<ObjetoMapa>& objetos() const { return objetos_; }
    const ObjetoMapa* spawn(const std::string& nome) const;

    // --- colisão ---
    bool solidoTile(i32 tx, i32 ty) const;
    bool solidoEm(Vector2 px) const;
    bool colideCirculo(Vector2 centro, float raio) const;
    // Move um círculo resolvendo eixo a eixo (desliza nas paredes). Devolve a nova posição.
    Vector2 moverCirculo(Vector2 pos, float raio, Vector2 delta) const;
    bool linhaDeVisao(Vector2 a, Vector2 b) const;

    // --- render ---
    void desenharChao(Rectangle visivel) const;
    void enfileirarDecoracao(FilaRender& fila, Rectangle visivel) const;

private:
    i32 indice(i32 tx, i32 ty) const { return ty * largura_ + tx; }
    bool dentro(i32 tx, i32 ty) const { return tx >= 0 && ty >= 0 && tx < largura_ && ty < altura_; }
    void desenharTile(i32 tile, float x, float y) const;

    std::string id_, nome_;
    bool seguro_ = false;
    Color ambiente_{12, 16, 42, 210};
    i32 largura_ = 0, altura_ = 0, tile_ = 16, tileArte_ = 16, colunasTileset_ = 1;  // tile_ em unidades; tileArte_ em px do PNG
    std::shared_ptr<Textura> tileset_;
    std::vector<i32> chao_, paredes_, decoracao_;
    std::vector<u8> decoracaoAlta_;
    std::vector<u8> solido_;
    std::vector<ObjetoMapa> objetos_;
};

} // namespace andv

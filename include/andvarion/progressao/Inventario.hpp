#pragma once
// Inventario — contagem de itens por id e catálogo (assets/dados/itens.json).
//
// Categorias: equipamento (tem "slot" e, se tiver visual, uma camada do
// paper-doll), consumível, material e item de missão.
#include "andvarion/core/Plataforma.hpp"

#include <map>
#include <string>

namespace andv {

enum class CategoriaItem : u8 { Equipamento, Consumivel, Material, Missao };
const char* nomeCategoria(CategoriaItem c);   // "Equipamentos", "Consumíveis", ...

struct DefinicaoItem {
    std::string id;
    std::string nome;        // "Fragmento de Ferro Amaldiçoado"
    std::string descricao;
    std::string vfx;         // chave do VFXManager para o item no chão
    CategoriaItem categoria = CategoriaItem::Material;
    std::string slot;        // equipamentos: "arma", "distancia", "armadura", "capa", "escudo"
    std::string visual;      // pasta do atlas da camada (paper-doll), ex.: "sprites/equip/armadura_cavaleiro"
    std::string arma;        // armas: id na tabela de Armas ("espada", "arco"...)
    std::map<std::string, float> atributos;  // "defesa", "vidaMax", "danoMagia"...
};

class Inventario {
public:
    // Carrega assets/dados/itens.json.
    bool carregarCatalogo(const std::string& json, std::string* erro = nullptr);

    void adicionar(const std::string& id, i32 qtd = 1);
    bool remover(const std::string& id, i32 qtd = 1);
    i32  quantidade(const std::string& id) const;

    const std::map<std::string, i32>& itens() const { return itens_; }
    void restaurar(std::map<std::string, i32> itens) { itens_ = std::move(itens); }

    const DefinicaoItem* definicao(const std::string& id) const;
    const std::map<std::string, DefinicaoItem>& catalogo() const { return catalogo_; }
    std::string nome(const std::string& id) const;  // id se não houver no catálogo

private:
    std::map<std::string, i32> itens_;
    std::map<std::string, DefinicaoItem> catalogo_;
};

} // namespace andv

#include "andvarion/progressao/Inventario.hpp"

#include <nlohmann/json.hpp>

namespace andv {

const char* nomeCategoria(CategoriaItem c) {
    switch (c) {
        case CategoriaItem::Equipamento: return "Equipamentos";
        case CategoriaItem::Consumivel:  return "Consumíveis";
        case CategoriaItem::Material:    return "Materiais";
        case CategoriaItem::Missao:      return "Itens de Missão";
    }
    return "";
}

bool Inventario::carregarCatalogo(const std::string& texto, std::string* erro) {
    try {
        const auto doc = nlohmann::json::parse(texto);
        for (const auto& i : doc.at("itens")) {
            DefinicaoItem d;
            d.id        = i.at("id").get<std::string>();
            d.nome      = i.value("nome", d.id);
            d.descricao = i.value("descricao", "");
            d.vfx       = i.value("vfx", "");
            const std::string cat = i.value("categoria", "material");
            d.categoria = cat == "equipamento" ? CategoriaItem::Equipamento
                        : cat == "consumivel"  ? CategoriaItem::Consumivel
                        : cat == "missao"      ? CategoriaItem::Missao
                                               : CategoriaItem::Material;
            d.slot      = i.value("slot", "");
            d.visual    = i.value("visual", "");
            d.arma      = i.value("arma", "");
            d.atributos = i.value("atributos", std::map<std::string, float>{});
            if (d.categoria == CategoriaItem::Equipamento && d.slot.empty())
                throw std::runtime_error("equipamento sem slot: " + d.id);
            catalogo_[d.id] = d;
        }
        return true;
    } catch (const std::exception& e) {
        if (erro) *erro = e.what();
        return false;
    }
}

void Inventario::adicionar(const std::string& id, i32 qtd) {
    if (qtd > 0) itens_[id] += qtd;
}

bool Inventario::remover(const std::string& id, i32 qtd) {
    auto it = itens_.find(id);
    if (it == itens_.end() || it->second < qtd) return false;
    it->second -= qtd;
    if (it->second == 0) itens_.erase(it);
    return true;
}

i32 Inventario::quantidade(const std::string& id) const {
    auto it = itens_.find(id);
    return it == itens_.end() ? 0 : it->second;
}

const DefinicaoItem* Inventario::definicao(const std::string& id) const {
    auto it = catalogo_.find(id);
    return it == catalogo_.end() ? nullptr : &it->second;
}

std::string Inventario::nome(const std::string& id) const {
    const auto* d = definicao(id);
    return d ? d->nome : id;
}

} // namespace andv

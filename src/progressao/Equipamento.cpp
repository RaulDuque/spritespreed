#include "andvarion/progressao/Equipamento.hpp"
#include "andvarion/progressao/Inventario.hpp"

#include <vector>

namespace andv {

namespace {
constexpr SlotEquipamento TODOS[] = {SlotEquipamento::Arma, SlotEquipamento::Distancia, SlotEquipamento::Armadura,
                                     SlotEquipamento::Capa, SlotEquipamento::Escudo};
}

const char* nomeSlot(SlotEquipamento s) {
    switch (s) {
        case SlotEquipamento::Arma:      return "Arma";
        case SlotEquipamento::Distancia: return "Distância";
        case SlotEquipamento::Armadura:  return "Armadura";
        case SlotEquipamento::Capa:      return "Capa";
        case SlotEquipamento::Escudo:    return "Escudo";
        default:                         return "";
    }
}

const char* chaveSlot(SlotEquipamento s) {
    switch (s) {
        case SlotEquipamento::Arma:      return "arma";
        case SlotEquipamento::Distancia: return "distancia";
        case SlotEquipamento::Armadura:  return "armadura";
        case SlotEquipamento::Capa:      return "capa";
        case SlotEquipamento::Escudo:    return "escudo";
        default:                         return "";
    }
}

std::optional<SlotEquipamento> slotDeChave(const std::string& chave) {
    for (SlotEquipamento s : TODOS)
        if (chave == chaveSlot(s)) return s;
    return std::nullopt;
}

ResultadoEquipar Equipamento::equipar(const std::string& itemId, const Inventario& inv) {
    const DefinicaoItem* d = inv.definicao(itemId);
    if (!d || d->categoria != CategoriaItem::Equipamento) return ResultadoEquipar::NaoEquipavel;
    const auto s = slotDeChave(d->slot);
    if (!s) return ResultadoEquipar::NaoEquipavel;
    if (inv.quantidade(itemId) <= 0) return ResultadoEquipar::NaoPossui;
    slots_[static_cast<std::size_t>(*s)] = itemId;
    return ResultadoEquipar::Ok;
}

bool Equipamento::desequipar(SlotEquipamento s) {
    if (s == SlotEquipamento::Arma || s == SlotEquipamento::Distancia) return false;
    slots_[static_cast<std::size_t>(s)].clear();
    return true;
}

bool Equipamento::equipado(const std::string& itemId) const {
    for (const auto& i : slots_)
        if (i == itemId) return true;
    return false;
}

float Equipamento::bonus(const std::string& atributo, const Inventario& inv) const {
    float total = 0.f;
    for (const auto& id : slots_) {
        if (id.empty()) continue;
        if (const DefinicaoItem* d = inv.definicao(id)) {
            auto it = d->atributos.find(atributo);
            if (it != d->atributos.end()) total += it->second;
        }
    }
    return total;
}

std::string Equipamento::proximo(SlotEquipamento s, const Inventario& inv, int passo) const {
    // Itens possuídos do slot, na ordem do catálogo (estável).
    std::vector<std::string> opcoes;
    for (const auto& [id, d] : inv.catalogo())
        if (d.categoria == CategoriaItem::Equipamento && d.slot == chaveSlot(s) && inv.quantidade(id) > 0)
            opcoes.push_back(id);
    if (opcoes.empty()) return item(s);
    int atual = 0;
    for (int i = 0; i < static_cast<int>(opcoes.size()); ++i)
        if (opcoes[static_cast<std::size_t>(i)] == item(s)) atual = i;
    const int n = static_cast<int>(opcoes.size());
    return opcoes[static_cast<std::size_t>(((atual + passo) % n + n) % n)];
}

void Equipamento::validar(const Inventario& inv) {
    for (auto& id : slots_)
        if (!id.empty() && inv.quantidade(id) <= 0) id.clear();
}

std::map<std::string, std::string> Equipamento::salvar() const {
    std::map<std::string, std::string> r;
    for (SlotEquipamento s : TODOS)
        if (!item(s).empty()) r[chaveSlot(s)] = item(s);
    return r;
}

void Equipamento::restaurar(const std::map<std::string, std::string>& dados, const Inventario& inv) {
    slots_ = {};
    for (const auto& [chave, id] : dados) equipar(id, inv);
}

} // namespace andv

#pragma once
// Equipamento — o que está em cada slot do personagem.
//
// Equipar não tira o item do inventário: o slot só aponta para ele. Os bônus
// vêm do catálogo ("atributos" de cada item) e o visual de cada slot vira uma
// camada do PaperDoll. Arma e Distância nunca ficam vazias.
#include "andvarion/core/Plataforma.hpp"

#include <array>
#include <map>
#include <optional>
#include <string>

namespace andv {

class Inventario;

enum class SlotEquipamento : u8 { Arma, Distancia, Armadura, Capa, Escudo, Total };

const char* nomeSlot(SlotEquipamento s);                 // "Arma", "Distância", ...
const char* chaveSlot(SlotEquipamento s);                // "arma", "distancia", ... (JSON)
std::optional<SlotEquipamento> slotDeChave(const std::string& chave);

enum class ResultadoEquipar : u8 { Ok, NaoPossui, NaoEquipavel };

class Equipamento {
public:
    ResultadoEquipar equipar(const std::string& itemId, const Inventario& inv);
    // Esvazia o slot (ignorado para Arma e Distância, que sempre têm algo).
    bool desequipar(SlotEquipamento s);
    const std::string& item(SlotEquipamento s) const { return slots_[static_cast<std::size_t>(s)]; }
    bool equipado(const std::string& itemId) const;

    // Soma do atributo em todos os itens equipados.
    float bonus(const std::string& atributo, const Inventario& inv) const;
    // Próximo item possuído do mesmo slot (troca rápida pela roda do mouse / R).
    std::string proximo(SlotEquipamento s, const Inventario& inv, int passo) const;

    // Remove do slot o que não está mais no inventário.
    void validar(const Inventario& inv);
    std::map<std::string, std::string> salvar() const;
    void restaurar(const std::map<std::string, std::string>& dados, const Inventario& inv);

private:
    std::array<std::string, static_cast<std::size_t>(SlotEquipamento::Total)> slots_{};
};

} // namespace andv

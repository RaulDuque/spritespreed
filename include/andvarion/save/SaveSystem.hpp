#pragma once
// SaveSystem — Save/Load em JSON ("Continuar" no menu principal).
//
// Só tipos de largura fixa (int32/float/string). O arquivo é gravado de forma
// atômica (.tmp + rename) para não corromper o save se o jogo fechar no meio.
#include "andvarion/core/Plataforma.hpp"
#include "andvarion/missoes/QuestManager.hpp"

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace andv {

struct SaveData {
    i32 versao = 1;
    std::string area = "vila_cinzavale";
    float posX = 0.f, posY = 0.f;
    bool temPosicao = false;               // falso = usar o spawn padrão da área

    i32 nivel = 1, xp = 0, pontos = 0;
    float vida = -1.f, mana = -1.f;        // -1 = cheio
    i32 frascos = 3, frascosMax = 3;
    std::string armaCorpo = "espada";
    std::string armaDistancia = "arco";
    float bonusDanoForja = 0.f;            // recompensa do Ferreiro

    std::vector<std::string> habilidades;
    std::map<std::string, i32> inventario;
    std::map<std::string, std::string> equipamento;   // slot -> item ("armadura": "armadura_cavaleiro")
    std::map<std::string, std::pair<i32, i32>> proficiencias;  // chave -> {nível, xp}
    std::vector<std::string> grimorio;                // magias nos slots 1, 2, 3, Q ("" = vazio)
    std::map<std::string, QuestManager::Instantaneo> missoes;
    std::vector<std::string> coletados;    // ids de coletáveis únicos já pegos ("ruinas:erva_1")
    double tempoJogoSeg = 0.0;
};

class SaveSystem {
public:
    explicit SaveSystem(std::string caminho);

    bool existe() const;
    bool salvar(const SaveData& d, std::string* erro = nullptr) const;
    std::optional<SaveData> carregar(std::string* erro = nullptr) const;

    static std::string paraJson(const SaveData& d);
    static std::optional<SaveData> deJson(const std::string& json, std::string* erro = nullptr);

    const std::string& caminho() const { return caminho_; }

private:
    std::string caminho_;
};

} // namespace andv

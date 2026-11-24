#pragma once
// Sessao — o estado persistente de uma partida (o que vai para o save).
// Criada em "Novo Jogo" ou "Continuar"; sobrevive às trocas de área.
#include "andvarion/dialogo/DialogoDados.hpp"
#include "andvarion/missoes/QuestManager.hpp"
#include "andvarion/progressao/ArvoreHabilidades.hpp"
#include "andvarion/progressao/Equipamento.hpp"
#include "andvarion/progressao/Proficiencia.hpp"
#include "andvarion/combate/Armas.hpp"

#include <array>
#include "andvarion/progressao/Inventario.hpp"
#include "andvarion/progressao/Progressao.hpp"
#include "andvarion/save/SaveSystem.hpp"

#include <memory>
#include <set>
#include <string>
#include <vector>

namespace andv {

class Assets;

struct Sessao {
    Sessao(EventBus& bus, Assets& assets);

    EventBus& bus;
    Progressao progressao;
    Inventario inventario;
    Equipamento equipamento;
    Proficiencia proficiencia;
    // Magias nos slots 1, 2, 3 e Q ("" = vazio).
    std::array<std::string, SLOTS_GRIMORIO> grimorio{"orbe_sangue", "nevoa_cortante", "", ""};
    ArvoreHabilidades arvore;
    BancoDialogos dialogos;
    std::unique_ptr<QuestManager> missoes;

    std::set<std::string> coletados;     // uids de itens únicos já pegos
    i32 frascos = 3, frascosMax = 3;
    float bonusDanoForja = 0.f;          // +% de dano da Lâmina (Ferreiro)
    double tempoJogo = 0.0;

    // Novo jogo (ou save antigo sem equipamento): itens e slots de itens.json → kitInicial.
    void aplicarKitInicial();
    // Esvazia slots de magias não aprendidas e coloca as aprendidas nos slots livres.
    void organizarGrimorio();
    // Põe a magia no slot (tirando-a de outro slot, se estiver lá).
    bool atribuirMagia(int slot, const std::string& feitico);

private:
    std::vector<std::string> kitItens_, kitEquipados_;
};

} // namespace andv

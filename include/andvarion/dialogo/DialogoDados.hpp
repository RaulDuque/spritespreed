#pragma once
// DialogoDados — o grafo de diálogos (sem renderização), lido de assets/dados/dialogos.json.
//
// Cada NPC tem "entradas" avaliadas em ordem: a primeira cujas condições forem
// verdadeiras escolhe o nó inicial. Assim o mesmo Ferreiro oferece a missão,
// cobra o progresso, recebe os fragmentos e depois só conversa.
//
// Condições (prefixo "!" nega):
//   missao:<id>:<ESTADO>              ESTADO = NAO_INICIADA | ATIVA | PRONTA_PARA_ENTREGAR | CONCLUIDA
//   pode_aceitar:<id>                 missão NAO_INICIADA e requisito cumprido
//   item:<id>:<qtd>                   inventário tem pelo menos qtd
//   objetivo:<missao>:<indice>:completo
//
// Ações (executadas ao escolher uma opção ou ao entrar num nó):
//   aceitar:<missao>   entregar:<missao>   curar   salvar
#include "andvarion/core/Plataforma.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace andv {

class QuestManager;
class Inventario;

struct OpcaoDialogo {
    std::string texto;
    std::string proximo;                  // vazio = encerra
    std::vector<std::string> acoes;
    std::vector<std::string> condicoes;   // opção só aparece se todas forem verdadeiras
};

struct NoDialogo {
    std::string id;
    std::string falante;                  // vazio = nome do NPC
    std::string texto;
    std::string proximo;                  // usado quando não há opções
    std::vector<std::string> acoes;       // ao entrar no nó
    std::vector<OpcaoDialogo> opcoes;
};

struct EntradaDialogo {
    std::vector<std::string> condicoes;
    std::string no;
};

struct DialogoNpc {
    std::string id;
    std::string nome;                     // "Bram, o Ferreiro"
    std::string retrato;                  // caminho relativo a assets/
    std::vector<EntradaDialogo> entradas;
    std::map<std::string, NoDialogo> nos;
};

class BancoDialogos {
public:
    bool carregar(const std::string& json, std::string* erro = nullptr);
    const DialogoNpc* npc(const std::string& id) const;
    const std::map<std::string, DialogoNpc>& npcs() const { return npcs_; }

private:
    std::map<std::string, DialogoNpc> npcs_;
};

// Avalia uma condição contra o estado do jogo.
bool avaliarCondicao(const std::string& condicao, const QuestManager& q, const Inventario& inv);
bool avaliarCondicoes(const std::vector<std::string>& condicoes, const QuestManager& q, const Inventario& inv);

// Nó inicial de um NPC dado o estado atual (nullopt se nenhuma entrada casar).
std::optional<std::string> escolherEntrada(const DialogoNpc& npc, const QuestManager& q, const Inventario& inv);

} // namespace andv

#pragma once
// QuestManager — missões com objetivos rastreáveis, dirigidas por eventos.
//
// Fluxo de estados:
//   NAO_INICIADA --aceitar()--> ATIVA --objetivos completos--> PRONTA_PARA_ENTREGAR
//                                  ^                                   |
//                                  +---- item perdido/consumido -------+
//   PRONTA_PARA_ENTREGAR --entregar()--> CONCLUIDA
//
// O QuestManager é um Observer do EventBus: ouve InimigoMorto, ItemColetado e
// NpcFalado e atualiza só as missões ATIVAS. Ele não sabe nada de inimigos,
// diálogo ou HUD. Quem dá a recompensa é quem ouve MissaoConcluida.
#include "andvarion/core/EventBus.hpp"

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace andv {

enum class TipoObjetivo : u8 { MatarInimigo, ColetarItem, FalarNpc };
enum class EstadoMissao : u8 { NaoIniciada, Ativa, ProntaParaEntregar, Concluida };

const char* nomeEstado(EstadoMissao e);              // "NAO_INICIADA", ...
std::optional<EstadoMissao> estadoDeTexto(const std::string& s);

struct Objetivo {
    TipoObjetivo tipo = TipoObjetivo::MatarInimigo;
    std::string  alvo;          // classe do inimigo, id do item ou id do NPC
    i32          quantidade = 1;
    i32          atual = 0;
    std::string  descricao;     // "Cultistas derrotados"
    bool completo() const { return atual >= quantidade; }
};

struct Recompensa {
    i32 xp = 0;
    std::map<std::string, i32> itens;  // itens recebidos
    std::string bonus;                 // efeito especial ("lamina_reforjada", "frasco_extra")
};

struct Missao {
    std::string id;
    std::string titulo;
    std::string descricao;
    bool        principal = false;
    std::string doador;          // NPC que oferece
    std::string entregarPara;    // NPC que recebe (padrão: doador)
    std::string requisito;       // missão que precisa estar CONCLUIDA antes
    bool        consumirItens = true;  // itens de COLETAR_ITEM são retirados na entrega
    std::vector<Objetivo> objetivos;
    Recompensa  recompensa;
    EstadoMissao estado = EstadoMissao::NaoIniciada;
};

class QuestManager {
public:
    using ContadorItens = std::function<i32(const std::string&)>;

    QuestManager(EventBus& bus, ContadorItens contarItem);

    bool carregar(const std::string& json, std::string* erro = nullptr);

    bool podeAceitar(const std::string& id) const;
    bool aceitar(const std::string& id);
    // Marca CONCLUIDA e publica MissaoConcluida. Devolve a recompensa a entregar.
    const Recompensa* entregar(const std::string& id);

    EstadoMissao estado(const std::string& id) const;
    const Missao* missao(const std::string& id) const;
    std::vector<const Missao*> rastreadas() const;  // ATIVA ou PRONTA, principal primeiro
    const std::vector<Missao>& todas() const { return missoes_; }

    // Recalcula objetivos COLETAR_ITEM pelo inventário (ex.: após carregar um save).
    void sincronizarItens();

    // Save/Load: estado + progresso de cada objetivo.
    struct Instantaneo {
        EstadoMissao estado;
        std::vector<i32> progresso;
    };
    std::map<std::string, Instantaneo> salvar() const;
    void restaurar(const std::map<std::string, Instantaneo>& dados);

private:
    Missao* buscar(const std::string& id);
    void aoEvento(const Evento& e);
    void reavaliar(Missao& m);

    EventBus& bus_;
    ContadorItens contarItem_;
    std::vector<Missao> missoes_;
    std::vector<Inscricao> inscricoes_;
};

} // namespace andv

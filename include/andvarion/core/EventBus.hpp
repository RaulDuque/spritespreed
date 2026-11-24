#pragma once
// EventBus — padrão Observer desacoplando gameplay de missões, HUD e conquistas.
//
// Quem publica (inimigo morre, item é coletado, NPC conversa) não conhece quem
// escuta (QuestManager, Notificacoes, HUD). Cada inscrição devolve uma
// `Inscricao` RAII: quando ela é destruída, o observador sai da lista sozinho,
// então não sobra callback pendurado apontando para objeto destruído.
#include "andvarion/core/Plataforma.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace andv {

enum class TipoEvento : u8 {
    InimigoMorto,      // id = classe do inimigo ("cultista", "carnical")
    ItemColetado,      // id = id do item, quantidade = quanto entrou no inventário
    NpcFalado,         // id = id do NPC
    AreaEntrada,       // id = id da área
    MissaoAtualizada,  // id = id da missão (qualquer mudança de estado/progresso)
    MissaoAceita,
    MissaoProntaParaEntregar,
    MissaoConcluida,
    NivelSubiu,        // quantidade = novo nível
    HabilidadeDesbloqueada,
    ProficienciaSubiu,  // id = arma ou escola, quantidade = novo nível
    JogadorFerido,
    JogadorMorreu,
};

struct Evento {
    TipoEvento  tipo;
    std::string id;
    i32         quantidade = 1;
};

class EventBus;

// Handle RAII de uma inscrição. Movível, não copiável.
class Inscricao {
public:
    Inscricao() = default;
    Inscricao(std::weak_ptr<EventBus*> barramento, u32 id) : barramento_(std::move(barramento)), id_(id) {}
    Inscricao(Inscricao&& o) noexcept { *this = std::move(o); }
    Inscricao& operator=(Inscricao&& o) noexcept;
    Inscricao(const Inscricao&) = delete;
    Inscricao& operator=(const Inscricao&) = delete;
    ~Inscricao() { cancelar(); }

    void cancelar();

private:
    std::weak_ptr<EventBus*> barramento_;
    u32 id_ = 0;
};

class EventBus {
public:
    using Observador = std::function<void(const Evento&)>;

    EventBus();
    ~EventBus();
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    // Escuta um tipo específico de evento.
    [[nodiscard]] Inscricao inscrever(TipoEvento tipo, Observador obs);
    // Escuta todos os eventos.
    [[nodiscard]] Inscricao inscreverTodos(Observador obs);

    // Entrega imediata. Observadores podem publicar novos eventos durante a
    // entrega (ex.: missão concluída -> XP -> nível subiu); eles entram numa fila
    // e são entregues em ordem, sem recursão.
    void publicar(const Evento& e);
    void publicar(TipoEvento tipo, std::string id = {}, i32 quantidade = 1) {
        publicar(Evento{tipo, std::move(id), quantidade});
    }

private:
    friend class Inscricao;
    void remover(u32 id);

    struct Registro {
        u32        id;
        bool       todos;
        TipoEvento tipo;
        Observador obs;
    };
    std::vector<Registro>     registros_;
    std::vector<Evento>       fila_;
    bool                      entregando_ = false;
    u32                       proximoId_  = 1;
    std::shared_ptr<EventBus*> vivo_;  // Inscricao guarda weak_ptr disso
};

} // namespace andv

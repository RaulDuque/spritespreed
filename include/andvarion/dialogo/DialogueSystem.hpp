#pragma once
// DialogueSystem — apresentação e navegação dos diálogos.
//
//  ┌─────────────────────────────────────────────────────────────────────┐
//  │ ┌──────┐  Bram, o Ferreiro                                         │
//  │ │retra-│  "O ferro dos carniçais está podre de maldição. Traga-me   │
//  │ │ to   │   três fragmentos e eu forjo algo digno da sua lâmina."▼  │
//  │ └──────┘                                                           │
//  └─────────────────────────────────────────────────────────────────────┘
//    pergaminho escuro na base da tela; retrato em pixel art à esquerda;
//    nome do NPC; texto em máquina de escrever (conta CODEPOINTS UTF-8);
//    opções navegáveis (W/S, setas, mouse) num painel acima, à direita.
//
// Ações conhecidas: aceitar:<missao>, entregar:<missao>. As demais
// ("curar", "salvar", ...) vão para o executor registrado pela cena.
#include "andvarion/core/Entrada.hpp"
#include "andvarion/dialogo/DialogoDados.hpp"

#include <raylib.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace andv {

class Assets;
class EventBus;
class Fonte;
class Textura;
class QuestManager;
class Inventario;

class DialogueSystem {
public:
    using Executor = std::function<void(const std::string& acao)>;

    DialogueSystem(const BancoDialogos& banco, QuestManager& missoes, Inventario& inventario, Assets& assets, EventBus& bus);

    void definirExecutor(Executor e) { executor_ = std::move(e); }

    bool iniciar(const std::string& npcId);
    bool ativo() const { return no_ != nullptr; }
    void encerrar();

    void atualizar(float dt, const ComandosJogador& cmd);
    void desenhar(const Fonte& fonte) const;

    static constexpr float CARACTERES_POR_SEGUNDO = 42.f;

private:
    void irPara(const std::string& noId);
    void escolher(int indice);
    void executar(const std::vector<std::string>& acoes);
    Rectangle retanguloOpcao(int i, const Fonte& fonte) const;
    bool completo() const { return static_cast<int>(digitados_) >= totalCodepoints_; }

    const BancoDialogos& banco_;
    QuestManager& missoes_;
    Inventario& inventario_;
    Assets& assets_;
    EventBus& bus_;
    Executor executor_;

    const DialogoNpc* npc_ = nullptr;
    const NoDialogo* no_ = nullptr;
    std::vector<const OpcaoDialogo*> opcoes_;
    std::shared_ptr<Textura> retrato_;
    int selecionada_ = 0;
    float digitados_ = 0.f;
    int totalCodepoints_ = 0;
    float tempo_ = 0.f;
    Vector2 mouse_{};
    mutable const Fonte* fonteCache_ = nullptr;  // para hit-test das opções
};

} // namespace andv

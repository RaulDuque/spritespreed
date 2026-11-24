#pragma once
// CenaJogo — a partida: mundo, jogador, diálogo, árvore, pausa, morte e transições.
#include "andvarion/cenas/Mundo.hpp"
#include "andvarion/cenas/Sessao.hpp"
#include "andvarion/core/EventBus.hpp"
#include "andvarion/core/Jogo.hpp"
#include "andvarion/dialogo/DialogueSystem.hpp"
#include "andvarion/entidades/Jogador.hpp"
#include "andvarion/gfx/CameraJogo.hpp"
#include "andvarion/gfx/Iluminacao.hpp"
#include "andvarion/ui/HUD.hpp"
#include "andvarion/ui/MenuArvoreHabilidades.hpp"
#include "andvarion/ui/MenuInventario.hpp"
#include "andvarion/ui/Notificacoes.hpp"

#include <memory>
#include <optional>
#include <vector>

namespace andv {

class CenaJogo final : public Cena {
public:
    CenaJogo(Jogo& jogo, std::optional<SaveData> save);
    ~CenaJogo() override;

    void lerEntrada() override;
    void atualizar(float dt) override;
    void prepararQuadro() override;
    void desenhar() override;
    bool usaCursorDoSistema() const override { return modo_ != Modo::Jogando; }

private:
    enum class Modo { Jogando, Dialogo, Arvore, Inventario, Pausa, Morto, Transicao };

    void atualizarPausa(const ComandosJogador& c);
    void atualizarMorte(const ComandosJogador& c);
    void atualizarTransicao(float dt);
    void entregarRecompensa(const std::string& missaoId);
    void executarAcaoDialogo(const std::string& acao);
    void salvar(bool silencioso = false);
    void despertar();
    void anunciarArea();
    void desenharPausa() const;
    void desenharMorte() const;
    void desenharMira(Vector2 m) const;

    std::unique_ptr<Sessao> sessao_;
    Jogador jogador_;
    SistemaIluminacao luz_;
    CameraJogo camera_;
    std::unique_ptr<Mundo> mundo_;
    std::unique_ptr<DialogueSystem> dialogo_;
    std::unique_ptr<Notificacoes> notificacoes_;
    HUD hud_;
    MenuArvoreHabilidades arvore_;
    MenuInventario inventario_;
    std::vector<Inscricao> inscricoes_;

    ComandosJogador cmd_;
    Modo modo_ = Modo::Jogando;
    int opcaoPausa_ = 0;
    float tempoMorte_ = 0.f;
    // Transição de área: fade-out -> carrega -> fade-in.
    std::optional<PedidoTransicao> transicao_;
    float fade_ = 1.f;          // começa preto e clareia
    bool carregou_ = true;
    std::string areaAnunciada_;
    float tempoAnuncio_ = 0.f;
};

} // namespace andv

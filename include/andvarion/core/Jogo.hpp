#pragma once
// Jogo — janela, laço principal com passo fixo (60 Hz) e pilha de cenas.
//
// Tudo é desenhado numa tela virtual de 640x360 (pixel art) e ampliado por um
// fator INTEIRO para a janela (sem pixels de larguras diferentes). O blit final
// passa pelo shader de atmosfera (vinheta/grão), se disponível.
#include "andvarion/core/Plataforma.hpp"

#include <raylib.h>

#include <memory>

namespace andv {

constexpr i32 LARGURA_VIRTUAL = 640;
constexpr i32 ALTURA_VIRTUAL = 360;
constexpr float PASSO_FIXO = 1.f / 60.f;

class Assets;
class VFXManager;
class EventBus;
class Fonte;
class SaveSystem;
class Jogo;

class Roteiro;

class Cena {
public:
    explicit Cena(Jogo& jogo) : jogo_(jogo) {}
    virtual ~Cena() = default;
    // Chamado uma vez por quadro de render, antes dos passos fixos (entrada).
    virtual void lerEntrada() {}
    virtual void atualizar(float dt) = 0;   // passo fixo
    // Passes em render targets próprios (lightmap etc.). Roda ANTES da tela
    // virtual ser ativada: render targets da raylib não podem ser aninhados.
    virtual void prepararQuadro() {}
    virtual void desenhar() = 0;            // na tela virtual
    virtual bool usaCursorDoSistema() const { return true; }

protected:
    Jogo& jogo_;
};

class Jogo {
public:
    Jogo();
    ~Jogo();
    Jogo(const Jogo&) = delete;
    Jogo& operator=(const Jogo&) = delete;

    int executar();

    // A troca acontece no fim do quadro (a cena atual termina o próprio update).
    void trocarCena(std::unique_ptr<Cena> nova);
    void sair() { sair_ = true; }

    Assets& assets() { return *assets_; }
    VFXManager& vfx() { return *vfx_; }
    EventBus& bus() { return *bus_; }
    Fonte& fonte() { return *fonte_; }
    SaveSystem& save() { return *save_; }

    Vector2 mouseVirtual() const;
    double tempo() const { return tempo_; }

private:
    void blit();

    std::unique_ptr<Assets> assets_;
    std::unique_ptr<VFXManager> vfx_;
    std::unique_ptr<EventBus> bus_;
    std::unique_ptr<Fonte> fonte_;
    std::unique_ptr<SaveSystem> save_;
    std::unique_ptr<Cena> cena_, proxima_;
    std::unique_ptr<Roteiro> roteiro_;   // ANDV_ROTEIRO (testes de fumaça)

    RenderTexture2D tela_{};
    Shader atmosfera_{};
    bool temShader_ = false;
    int locTempo_ = -1, locResolucao_ = -1;
    bool sair_ = false;
    double tempo_ = 0.0;
};

} // namespace andv

#pragma once
// SistemaIluminacao — escuridão noturna azulada + luzes 2D dinâmicas.
//
// Passo 1 (lightmap): uma RenderTexture do tamanho da tela virtual é limpa com
// a cor do ambiente (azul-noite, alfa = quão escuro). Cada luz é um gradiente
// radial desenhado com blend customizado  dst = dst * (1 - src.a), que "fura"
// a escuridão. Luzes sobrepostas se combinam de forma multiplicativa.
// Passo 2 (aplicar): o lightmap é desenhado sobre o mundo com alfa normal e,
// por cima, um brilho aditivo com a cor de cada luz (calor da lanterna/tochas).
// Passo 3 (pós): shader opcional assets/shaders/atmosfera.fs (vinheta, grão,
// separação fria/quente) aplicado no blit final em Jogo.
#include "andvarion/core/Plataforma.hpp"

#include <raylib.h>

#include <vector>

namespace andv {

struct Luz {
    Vector2 pos{};
    float raio = 64.f;
    Color cor{255, 180, 90, 255};   // cor do brilho aditivo
    float intensidade = 1.f;        // 0..1: quanto fura a escuridão
    float oscilacao = 0.f;          // 0..1: tremulação (tochas)
    float fase = 0.f;
    float brilho = 0.35f;           // força do brilho aditivo
    float vida = -1.f;              // >0: luz temporária (flash), apaga sozinha
    float vidaTotal = -1.f;
};

class SistemaIluminacao {
public:
    SistemaIluminacao() = default;
    ~SistemaIluminacao();
    SistemaIluminacao(const SistemaIluminacao&) = delete;
    SistemaIluminacao& operator=(const SistemaIluminacao&) = delete;

    void iniciar(i32 largura, i32 altura);
    void definirAmbiente(Color c) { ambiente_ = c; }
    Color ambiente() const { return ambiente_; }

    u32  adicionar(const Luz& l);
    void remover(u32 id);
    void mover(u32 id, Vector2 pos);
    Luz* luz(u32 id);
    void flash(Vector2 pos, float raio, Color cor, float duracao);  // luz temporária
    void limpar();

    void atualizar(float dt);
    // Monta o lightmap para a câmera atual (coordenadas de mundo -> tela virtual).
    void renderizar(const Camera2D& camera);
    // Desenha escuridão + brilho sobre o que já está no alvo atual.
    void aplicar() const;

private:
    struct Registro { u32 id; Luz luz; };
    std::vector<Registro> luzes_;
    RenderTexture2D lightmap_{};
    bool iniciado_ = false;
    Color ambiente_{12, 16, 42, 210};
    float tempo_ = 0.f;
    u32 proximoId_ = 1;
    Camera2D camera_{};
};

} // namespace andv

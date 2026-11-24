#pragma once
// VFXManager — toca os efeitos gerados pelo pixelfx (assets/vfx).
//
// tools/gerar_vfx.mjs renderiza cada receita do pixelfx numa sprite sheet com
// meta.pixelfx (fps, loop, pivô, act) e registra a chave em assets/vfx/indice.json.
// Aqui cada chave vira uma SpriteSheet; `tocar` cria uma instância com posição,
// rotação, escala e camada. Efeitos "loop" vivem até `parar` (com fade-out);
// os "one-shot" se removem sozinhos ao fim, como no pixelfx (`fx.done()`).
//
// Efeitos direcionais do pixelfx apontam para a direita. Com `orientar`, o
// ângulo de mira gira o efeito; para a esquerda ele é espelhado em vez de ficar
// de cabeça para baixo.
#include "andvarion/gfx/SpriteSheet.hpp"

#include <raylib.h>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace andv {

class Assets;

enum class CamadaVfx : u8 { Chao, Frente };

struct ParametrosVfx {
    float anguloGraus = 0.f;       // direção do efeito (0 = direita)
    bool  orientar = false;        // usa anguloGraus com espelhamento à esquerda
    float escala = 1.f;
    bool  espelhar = false;
    CamadaVfx camada = CamadaVfx::Frente;
    Color tinta = WHITE;
    bool  aditivo = false;         // brilho somado (bom para magia no escuro)
    float velocidade = 1.f;
    float duracaoMax = -1.f;       // loops: para sozinho depois disso (<0 = nunca)
    std::function<Vector2()> seguir;  // posição dinâmica (ex.: aura no jogador)
};

class VFXManager {
public:
    // Lê vfx/indice.json e carrega todas as sheets listadas.
    void carregar(Assets& assets);

    bool existe(const std::string& chave) const { return sheets_.count(chave) > 0; }
    std::shared_ptr<const SpriteSheet> sheet(const std::string& chave) const;

    u32  tocar(const std::string& chave, Vector2 pos, const ParametrosVfx& p = {});
    void parar(u32 id);                      // fade-out de 0,25 s
    void pararTodos();
    void mover(u32 id, Vector2 pos);
    bool ativo(u32 id) const;

    void atualizar(float dt);
    void desenhar(CamadaVfx camada) const;

    // Desenho sem estado (projéteis desenham o próprio efeito em loop).
    void desenharQuadro(const std::string& chave, float tempoSeg, Vector2 pos, const ParametrosVfx& p) const;

    // Ícone de UI: o quadro do meio do efeito, CENTRALIZADO e ampliado para
    // caber em `r` (cada efeito do pixelfx tem pivô e tamanho próprios).
    // `zoom` > 1 compensa o espaço vazio em volta do desenho no quadro.
    void desenharIcone(const std::string& chave, Rectangle r, float zoom = 2.2f, Color tinta = WHITE) const;

    // Hit-stop/tremor declarados no "act" do efeito (meta.pixelfx.act.hits).
    float congelamentoDoGolpe(const std::string& chave) const;
    float duracao(const std::string& chave) const;

private:
    struct Instancia {
        u32 id;
        std::shared_ptr<const SpriteSheet> sheet;
        Vector2 pos;
        ParametrosVfx p;
        float tempo = 0.f;
        bool loop = false;
        float fade = -1.f;   // >= 0: saindo
        bool morta = false;
    };
    void desenharInstancia(const SpriteSheet& s, float tempo, bool loop, Vector2 pos,
                           const ParametrosVfx& p, float alfa) const;

    std::unordered_map<std::string, std::shared_ptr<const SpriteSheet>> sheets_;
    std::vector<Instancia> instancias_;
    u32 proximoId_ = 1;
};

} // namespace andv

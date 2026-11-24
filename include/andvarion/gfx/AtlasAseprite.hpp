#pragma once
// AtlasAseprite — parser (sem GPU) do atlas JSON estilo Aseprite.
//
// É o formato comum aos dois pipelines de arte do projeto:
//
//  * agent-sprite-forge  (export_engine.py --target aseprite-json)
//      - frames nomeados "0","1",...; um frameTag por clipe
//      - tag.repeat == "1"  => clipe sem loop (one-shot)
//      - tag.data (string JSON) => {"events":[{"name":"hit","frame":N,"at_ms":T}]}
//      - meta.slices["anchor"] => pivô (pés); slice.data guarda anchor_px exato
//      - várias páginas: cada página é um JSON+PNG; um clipe nunca cruza páginas
//
//  * pixelfx  (tools/gerar_vfx.mjs -> spriteSheet)
//      - frames "<nome> <i>", um único frameTag
//      - meta.pixelfx = { fps, loop, frameSize, pivot, columns, act? }
//
// Também aceita o formato "hash" do Aseprite (frames como objeto), preservando
// a ordem do arquivo.
#include "andvarion/core/Plataforma.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace andv {

struct QuadroAtlas {
    i32 x = 0, y = 0, w = 0, h = 0;
    i32 duracaoMs = 100;
};

struct EventoClipe {
    std::string nome;    // "hit", "step_l", "release", ...
    i32 posicao = 0;     // índice dentro de ClipeAtlas::quadros
};

struct ClipeAtlas {
    std::string       nome;
    std::vector<i32>  quadros;      // linha do tempo TOCADA (pingpong/reverso já expandidos)
    std::vector<i32>  duracoesMs;   // uma por entrada de `quadros`
    bool              loop = true;
    std::vector<EventoClipe> eventos;
    i32               duracaoTotalMs = 0;

    // Posição na linha do tempo para um tempo em ms (com ou sem loop).
    i32 posicaoEm(float ms) const;
};

// Golpe declarado num "act" do pixelfx: em t segundos, congelar por `congelar` s.
struct GolpePixelfx {
    float t = 0.f, congelar = 0.f, tremor = 0.f;
};

struct MetaPixelfx {
    std::string id;
    float fps = 30.f;
    bool  loop = false;
    i32   colunas = 1;
    float preparacao = 0.f;              // act.windup: o corpo se prepara antes do efeito
    std::vector<GolpePixelfx> golpes;    // act.hits
};

class AtlasAseprite {
public:
    // Faz o parse do texto JSON. Em erro devolve nullopt e preenche `erro`.
    static std::optional<AtlasAseprite> deJson(const std::string& json, std::string* erro = nullptr);

    const std::string& imagem() const { return imagem_; }       // meta.image (relativo ao JSON)
    i32   larguraQuadro() const { return larguraQuadro_; }
    i32   alturaQuadro()  const { return alturaQuadro_; }
    float ancoraX() const { return ancoraX_; }                 // pivô em pixels do quadro
    float ancoraY() const { return ancoraY_; }

    const std::vector<QuadroAtlas>& quadros() const { return quadros_; }
    const std::map<std::string, ClipeAtlas>& clipes() const { return clipes_; }
    const ClipeAtlas* clipe(const std::string& nome) const;
    const std::optional<MetaPixelfx>& pixelfx() const { return pixelfx_; }

private:
    std::string imagem_;
    i32   larguraQuadro_ = 0, alturaQuadro_ = 0;
    float ancoraX_ = 0.f, ancoraY_ = 0.f;
    std::vector<QuadroAtlas> quadros_;
    std::map<std::string, ClipeAtlas> clipes_;
    std::optional<MetaPixelfx> pixelfx_;
};

} // namespace andv

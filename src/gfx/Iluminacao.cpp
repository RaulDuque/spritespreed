#include "andvarion/gfx/Iluminacao.hpp"

#include <rlgl.h>

#include <algorithm>
#include <cmath>

namespace andv {

namespace {

// Tremulação orgânica: soma de senos incomensuráveis (sem ruído aleatório por quadro).
float tremor(float t, float fase) {
    return 0.5f + 0.5f * (0.55f * std::sin(t * 7.3f + fase) + 0.30f * std::sin(t * 13.1f + fase * 2.1f) +
                          0.15f * std::sin(t * 23.7f + fase * 0.7f));
}

} // namespace

SistemaIluminacao::~SistemaIluminacao() {
    if (iniciado_ && IsWindowReady()) UnloadRenderTexture(lightmap_);
}

void SistemaIluminacao::iniciar(i32 largura, i32 altura) {
    if (iniciado_) UnloadRenderTexture(lightmap_);
    lightmap_ = LoadRenderTexture(largura, altura);
    SetTextureFilter(lightmap_.texture, TEXTURE_FILTER_BILINEAR);  // bordas de luz suaves
    iniciado_ = true;
}

u32 SistemaIluminacao::adicionar(const Luz& l) {
    luzes_.push_back({proximoId_, l});
    return proximoId_++;
}

void SistemaIluminacao::remover(u32 id) {
    luzes_.erase(std::remove_if(luzes_.begin(), luzes_.end(), [id](const Registro& r) { return r.id == id; }),
                 luzes_.end());
}

void SistemaIluminacao::mover(u32 id, Vector2 pos) {
    if (Luz* l = luz(id)) l->pos = pos;
}

Luz* SistemaIluminacao::luz(u32 id) {
    for (auto& r : luzes_)
        if (r.id == id) return &r.luz;
    return nullptr;
}

void SistemaIluminacao::flash(Vector2 pos, float raio, Color cor, float duracao) {
    Luz l;
    l.pos = pos;
    l.raio = raio;
    l.cor = cor;
    l.intensidade = 0.9f;
    l.brilho = 0.6f;
    l.vida = l.vidaTotal = duracao;
    adicionar(l);
}

void SistemaIluminacao::limpar() { luzes_.clear(); }

void SistemaIluminacao::atualizar(float dt) {
    tempo_ += dt;
    for (auto& r : luzes_)
        if (r.luz.vida > 0.f) r.luz.vida -= dt;
    luzes_.erase(std::remove_if(luzes_.begin(), luzes_.end(),
                                [](const Registro& r) { return r.luz.vidaTotal > 0.f && r.luz.vida <= 0.f; }),
                 luzes_.end());
}

void SistemaIluminacao::renderizar(const Camera2D& camera) {
    if (!iniciado_) return;
    camera_ = camera;
    BeginTextureMode(lightmap_);
    ClearBackground(ambiente_);
    // dst = src * 0 + dst * (1 - src.a): cada luz subtrai escuridão.
    rlSetBlendFactors(RL_ZERO, RL_ONE_MINUS_SRC_ALPHA, RL_FUNC_ADD);
    BeginBlendMode(BLEND_CUSTOM);
    BeginMode2D(camera);
    for (const auto& r : luzes_) {
        const Luz& l = r.luz;
        float k = 1.f, raio = l.raio;
        if (l.oscilacao > 0.f) {
            const float f = tremor(tempo_, l.fase);
            k -= l.oscilacao * 0.35f * f;
            raio *= 1.f - l.oscilacao * 0.12f * f;
        }
        if (l.vidaTotal > 0.f) k *= std::clamp(l.vida / l.vidaTotal, 0.f, 1.f);
        const unsigned char a = static_cast<unsigned char>(std::clamp(l.intensidade * k, 0.f, 1.f) * 255.f);
        // Núcleo + halo: dois gradientes dão uma queda mais natural que um só.
        DrawCircleGradient(static_cast<int>(l.pos.x), static_cast<int>(l.pos.y), raio, Color{0, 0, 0, a}, Color{0, 0, 0, 0});
        DrawCircleGradient(static_cast<int>(l.pos.x), static_cast<int>(l.pos.y), raio * 0.45f,
                           Color{0, 0, 0, static_cast<unsigned char>(a * 0.8f)}, Color{0, 0, 0, 0});
    }
    EndMode2D();
    EndBlendMode();
    EndTextureMode();
}

void SistemaIluminacao::aplicar() const {
    if (!iniciado_) return;
    const auto& t = lightmap_.texture;
    // RenderTexture vem de cabeça para baixo no OpenGL: altura negativa na fonte.
    DrawTexturePro(t, {0, 0, static_cast<float>(t.width), -static_cast<float>(t.height)},
                   {0, 0, static_cast<float>(t.width), static_cast<float>(t.height)}, {0, 0}, 0.f, WHITE);

    // Brilho aditivo colorido (calor da lanterna, tochas, flashes de magia).
    BeginBlendMode(BLEND_ADDITIVE);
    BeginMode2D(camera_);
    for (const auto& r : luzes_) {
        const Luz& l = r.luz;
        float k = l.brilho;
        if (l.oscilacao > 0.f) k *= 1.f - l.oscilacao * 0.4f * tremor(tempo_ + 0.37f, l.fase);
        if (l.vidaTotal > 0.f) k *= std::clamp(l.vida / l.vidaTotal, 0.f, 1.f);
        Color c = l.cor;
        c.a = static_cast<unsigned char>(std::clamp(k, 0.f, 1.f) * 255.f);
        DrawCircleGradient(static_cast<int>(l.pos.x), static_cast<int>(l.pos.y), l.raio * 0.6f, c, Color{c.r, c.g, c.b, 0});
    }
    EndMode2D();
    EndBlendMode();
}

} // namespace andv

#include "andvarion/gfx/VFXManager.hpp"
#include "andvarion/core/Assets.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>

namespace andv {

void VFXManager::carregar(Assets& assets) {
    const std::string indice = assets.lerTexto("vfx/indice.json");
    if (indice.empty()) {
        TraceLog(LOG_WARNING, "ANDVARION: vfx/indice.json ausente. Rode: node tools/gerar_vfx.mjs");
        return;
    }
    try {
        const auto doc = nlohmann::json::parse(indice);
        for (const auto& [chave, info] : doc.at("efeitos").items()) {
            if (auto s = assets.sheet("vfx/" + info.value("json", chave + ".json"))) sheets_[chave] = s;
        }
        TraceLog(LOG_INFO, "ANDVARION: %d efeitos pixelfx carregados", static_cast<int>(sheets_.size()));
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "ANDVARION: vfx/indice.json inválido: %s", e.what());
    }
}

std::shared_ptr<const SpriteSheet> VFXManager::sheet(const std::string& chave) const {
    auto it = sheets_.find(chave);
    return it == sheets_.end() ? nullptr : it->second;
}

u32 VFXManager::tocar(const std::string& chave, Vector2 pos, const ParametrosVfx& p) {
    auto s = sheet(chave);
    if (!s) return 0;
    Instancia i{proximoId_++, s, pos, p};
    const auto& px = s->atlas().pixelfx();
    const ClipeAtlas* c = s->atlas().clipes().empty() ? nullptr : &s->atlas().clipes().begin()->second;
    i.loop = px ? px->loop : (c && c->loop);
    instancias_.push_back(std::move(i));
    return instancias_.back().id;
}

void VFXManager::parar(u32 id) {
    for (auto& i : instancias_)
        if (i.id == id && i.fade < 0.f) i.fade = 0.25f;
}

void VFXManager::pararTodos() { instancias_.clear(); }

void VFXManager::mover(u32 id, Vector2 pos) {
    for (auto& i : instancias_)
        if (i.id == id) i.pos = pos;
}

bool VFXManager::ativo(u32 id) const {
    return std::any_of(instancias_.begin(), instancias_.end(), [id](const Instancia& i) { return i.id == id && !i.morta; });
}

void VFXManager::atualizar(float dt) {
    for (auto& i : instancias_) {
        i.tempo += dt * i.p.velocidade;
        if (i.p.seguir) i.pos = i.p.seguir();
        if (i.fade >= 0.f) {
            i.fade -= dt;
            if (i.fade <= 0.f) i.morta = true;
            continue;
        }
        if (i.loop) {
            if (i.p.duracaoMax >= 0.f && i.tempo >= i.p.duracaoMax) i.fade = 0.25f;
        } else {
            const auto& clipes = i.sheet->atlas().clipes();
            const float total = clipes.empty() ? 0.f : clipes.begin()->second.duracaoTotalMs / 1000.f;
            if (i.tempo >= total) i.morta = true;
        }
    }
    instancias_.erase(std::remove_if(instancias_.begin(), instancias_.end(), [](const Instancia& i) { return i.morta; }),
                      instancias_.end());
}

void VFXManager::desenharInstancia(const SpriteSheet& s, float tempo, bool loop, Vector2 pos,
                                   const ParametrosVfx& p, float alfa) const {
    const auto& clipes = s.atlas().clipes();
    if (clipes.empty()) return;
    const ClipeAtlas* pc = &clipes.begin()->second;
    // `loop` vem do meta.pixelfx, mas desenharQuadro força loop (projéteis);
    // para one-shot, prende no último quadro.
    const float ms = tempo * 1000.f;
    i32 posicao;
    if (loop && pc->duracaoTotalMs > 0) {
        const float t = std::fmod(ms, static_cast<float>(pc->duracaoTotalMs));
        float acc = 0.f;
        posicao = static_cast<i32>(pc->quadros.size()) - 1;
        for (std::size_t k = 0; k < pc->duracoesMs.size(); ++k) {
            acc += static_cast<float>(pc->duracoesMs[k]);
            if (t < acc) { posicao = static_cast<i32>(k); break; }
        }
    } else {
        posicao = pc->posicaoEm(ms);
    }
    const i32 quadro = pc->quadros[static_cast<std::size_t>(posicao)];

    float angulo = p.anguloGraus;
    bool espelhar = p.espelhar;
    if (p.orientar) {
        // Para a esquerda: espelha e gira o complemento, nunca de ponta-cabeça.
        float a = std::fmod(angulo, 360.f);
        if (a < 0) a += 360.f;
        if (a > 90.f && a < 270.f) {
            espelhar = !espelhar;
            angulo = a - 180.f;
        } else {
            angulo = a;
        }
    }
    Color tinta = p.tinta;
    tinta.a = static_cast<unsigned char>(std::clamp(alfa, 0.f, 1.f) * tinta.a);
    if (p.aditivo) BeginBlendMode(BLEND_ADDITIVE);
    s.desenhar(quadro, pos, p.escala, espelhar, angulo, tinta);
    if (p.aditivo) EndBlendMode();
}

void VFXManager::desenhar(CamadaVfx camada) const {
    for (const auto& i : instancias_) {
        if (i.p.camada != camada) continue;
        const float alfa = i.fade >= 0.f ? i.fade / 0.25f : 1.f;
        desenharInstancia(*i.sheet, i.tempo, i.loop, i.pos, i.p, alfa);
    }
}

void VFXManager::desenharQuadro(const std::string& chave, float tempoSeg, Vector2 pos, const ParametrosVfx& p) const {
    if (auto s = sheet(chave)) desenharInstancia(*s, tempoSeg, true, pos, p, 1.f);
}

void VFXManager::desenharIcone(const std::string& chave, Rectangle r, float zoom, Color tinta) const {
    auto s = sheet(chave);
    if (!s || s->atlas().clipes().empty()) return;
    const auto& a = s->atlas();
    const float fw = static_cast<float>(a.larguraQuadro()), fh = static_cast<float>(a.alturaQuadro());
    const float escala = zoom * std::min(r.width / fw, r.height / fh);
    // O pivô fica deslocado do centro do quadro: compensa para o centro cair no centro de r.
    const Vector2 pos{r.x + r.width * 0.5f + (a.ancoraX() - fw * 0.5f) * escala,
                      r.y + r.height * 0.5f + (a.ancoraY() - fh * 0.5f) * escala};
    ParametrosVfx p;
    p.escala = escala;
    p.tinta = tinta;
    BeginScissorMode(static_cast<int>(r.x), static_cast<int>(r.y), static_cast<int>(r.width), static_cast<int>(r.height));
    desenharInstancia(*s, duracao(chave) * 0.5f, true, pos, p, 1.f);
    EndScissorMode();
}

float VFXManager::congelamentoDoGolpe(const std::string& chave) const {
    auto s = sheet(chave);
    if (!s || !s->atlas().pixelfx()) return 0.f;
    float maior = 0.f;
    for (const auto& g : s->atlas().pixelfx()->golpes) maior = std::max(maior, g.congelar);
    return maior;
}

float VFXManager::duracao(const std::string& chave) const {
    auto s = sheet(chave);
    if (!s || s->atlas().clipes().empty()) return 0.f;
    return s->atlas().clipes().begin()->second.duracaoTotalMs / 1000.f;
}

} // namespace andv

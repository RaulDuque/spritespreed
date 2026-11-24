#include "andvarion/gfx/AtlasAseprite.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>

namespace andv {

using json = nlohmann::ordered_json;  // ordered: preserva a ordem do formato "hash"

i32 ClipeAtlas::posicaoEm(float ms) const {
    if (quadros.empty()) return 0;
    if (duracaoTotalMs <= 0) return 0;
    float t = ms;
    if (loop) {
        t = std::fmod(t, static_cast<float>(duracaoTotalMs));
        if (t < 0) t += static_cast<float>(duracaoTotalMs);
    } else if (t >= static_cast<float>(duracaoTotalMs)) {
        return static_cast<i32>(quadros.size()) - 1;
    }
    float acumulado = 0.f;
    for (std::size_t i = 0; i < duracoesMs.size(); ++i) {
        acumulado += static_cast<float>(duracoesMs[i]);
        if (t < acumulado) return static_cast<i32>(i);
    }
    return static_cast<i32>(quadros.size()) - 1;
}

const ClipeAtlas* AtlasAseprite::clipe(const std::string& nome) const {
    auto it = clipes_.find(nome);
    return it == clipes_.end() ? nullptr : &it->second;
}

namespace {

QuadroAtlas lerQuadro(const json& f) {
    QuadroAtlas q;
    const json& r = f.at("frame");
    q.x = r.value("x", 0);
    q.y = r.value("y", 0);
    q.w = r.value("w", 0);
    q.h = r.value("h", 0);
    q.duracaoMs = std::max(1, f.value("duration", 100));
    return q;
}

bool falhar(std::string* erro, const std::string& msg) {
    if (erro) *erro = msg;
    return false;
}

} // namespace

std::optional<AtlasAseprite> AtlasAseprite::deJson(const std::string& texto, std::string* erro) {
    AtlasAseprite a;
    try {
        const json doc = json::parse(texto);
        const json& meta = doc.at("meta");
        a.imagem_ = meta.value("image", "");

        // --- quadros (formato "array" ou "hash") ---
        const json& frames = doc.at("frames");
        if (frames.is_array()) {
            for (const auto& f : frames) a.quadros_.push_back(lerQuadro(f));
        } else if (frames.is_object()) {
            for (const auto& [nome, f] : frames.items()) a.quadros_.push_back(lerQuadro(f));
        }
        if (a.quadros_.empty()) {
            falhar(erro, "atlas sem quadros");
            return std::nullopt;
        }
        a.larguraQuadro_ = a.quadros_.front().w;
        a.alturaQuadro_  = a.quadros_.front().h;

        // --- metadados do pixelfx ---
        if (meta.contains("pixelfx")) {
            const json& p = meta["pixelfx"];
            MetaPixelfx m;
            m.id      = p.value("id", "");
            m.fps     = p.value("fps", 30.f);
            m.loop    = p.value("loop", false);
            m.colunas = p.value("columns", 1);
            if (p.contains("act") && p["act"].is_object()) {
                const json& act = p["act"];
                m.preparacao = act.value("windup", 0.f);
                if (act.contains("hits"))
                    for (const auto& h : act["hits"])
                        m.golpes.push_back({h.value("t", 0.f), h.value("freeze", 0.f), h.value("shake", 0.f)});
            }
            if (p.contains("pivot") && p["pivot"].is_array() && p["pivot"].size() == 2) {
                a.ancoraX_ = p["pivot"][0].get<float>();
                a.ancoraY_ = p["pivot"][1].get<float>();
            }
            a.pixelfx_ = m;
        } else {
            a.ancoraX_ = a.larguraQuadro_ * 0.5f;  // padrão: centro dos pés
            a.ancoraY_ = static_cast<float>(a.alturaQuadro_);
        }

        // --- âncora do agent-sprite-forge (slice "anchor") ---
        if (meta.contains("slices")) {
            for (const auto& s : meta["slices"]) {
                if (s.value("name", "") != "anchor" || !s.contains("keys") || s["keys"].empty()) continue;
                const json& k = s["keys"][0];
                const float bx = k.contains("bounds") ? k["bounds"].value("x", 0.f) : 0.f;
                const float by = k.contains("bounds") ? k["bounds"].value("y", 0.f) : 0.f;
                if (k.contains("pivot")) {
                    a.ancoraX_ = bx + k["pivot"].value("x", 0.f);
                    a.ancoraY_ = by + k["pivot"].value("y", 0.f);
                }
                // O pivô do Aseprite é inteiro; o valor exato (fracionário) vem em data.
                if (s.contains("data") && s["data"].is_string()) {
                    const json d = json::parse(s["data"].get<std::string>(), nullptr, false);
                    if (!d.is_discarded() && d.contains("anchor_px") && d["anchor_px"].size() == 2) {
                        a.ancoraX_ = d["anchor_px"][0].get<float>();
                        a.ancoraY_ = d["anchor_px"][1].get<float>();
                    }
                }
            }
        }

        // --- clipes (frameTags) ---
        const bool loopPadrao = a.pixelfx_ ? a.pixelfx_->loop : true;
        const json tags = meta.contains("frameTags") ? meta["frameTags"] : json::array();
        const i32 total = static_cast<i32>(a.quadros_.size());

        auto montar = [&](const std::string& nome, i32 de, i32 ate, const std::string& direcao,
                          bool loop, const json* dadosEventos) {
            de  = std::clamp(de, 0, total - 1);
            ate = std::clamp(ate, de, total - 1);
            ClipeAtlas c;
            c.nome = nome;
            c.loop = loop;
            std::vector<i32> autorado;
            for (i32 i = de; i <= ate; ++i) autorado.push_back(i);
            if (direcao == "reverse") {
                std::reverse(autorado.begin(), autorado.end());
                c.quadros = autorado;
            } else if (direcao == "pingpong" && autorado.size() > 2) {
                c.quadros = autorado;                                   // 0 1 2 3
                for (i32 i = static_cast<i32>(autorado.size()) - 2; i > 0; --i)
                    c.quadros.push_back(autorado[i]);                   // 2 1
            } else {
                c.quadros = autorado;
            }
            for (i32 q : c.quadros) {
                c.duracoesMs.push_back(a.quadros_[q].duracaoMs);
                c.duracaoTotalMs += a.quadros_[q].duracaoMs;
            }
            // Eventos do agent-sprite-forge: "frame" é o índice absoluto no atlas,
            // já na linha do tempo tocada (o exportador só emite "forward").
            if (dadosEventos && dadosEventos->contains("events")) {
                for (const auto& e : (*dadosEventos)["events"]) {
                    const i32 pos = e.value("frame", de) - de;
                    if (pos >= 0 && pos < static_cast<i32>(c.quadros.size()))
                        c.eventos.push_back({e.value("name", std::string("hit")), pos});
                }
            }
            a.clipes_[nome] = std::move(c);
        };

        if (tags.empty()) {
            montar("padrao", 0, total - 1, "forward", loopPadrao, nullptr);
        } else {
            for (const auto& t : tags) {
                bool loop = loopPadrao;
                if (t.contains("repeat")) {
                    // Aseprite: "repeat" é a contagem de repetições como string; "0"/ausente = infinito.
                    const std::string rep = t["repeat"].is_string() ? t["repeat"].get<std::string>()
                                                                    : std::to_string(t["repeat"].get<i32>());
                    loop = (rep == "0");
                }
                json dados;
                if (t.contains("data") && t["data"].is_string()) {
                    dados = json::parse(t["data"].get<std::string>(), nullptr, false);
                    if (dados.is_discarded()) dados = json();
                    if (dados.contains("loop_policy")) loop = dados["loop_policy"] == "cycle";
                }
                montar(t.value("name", "padrao"), t.value("from", 0), t.value("to", total - 1),
                       t.value("direction", "forward"), loop, dados.is_object() ? &dados : nullptr);
            }
        }
    } catch (const std::exception& ex) {
        falhar(erro, ex.what());
        return std::nullopt;
    }
    return a;
}

} // namespace andv

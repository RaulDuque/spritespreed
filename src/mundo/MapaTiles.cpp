#include "andvarion/mundo/MapaTiles.hpp"
#include "andvarion/core/Assets.hpp"
#include "andvarion/gfx/FilaRender.hpp"

#include <nlohmann/json.hpp>
#include <raymath.h>

#include <algorithm>
#include <cmath>
#include <set>
#include <sstream>

namespace andv {

using json = nlohmann::json;

namespace {

std::vector<std::vector<i32>> lerCsv(const std::string& texto) {
    std::vector<std::vector<i32>> linhas;
    std::istringstream ss(texto);
    std::string linha;
    while (std::getline(ss, linha)) {
        if (linha.empty() || linha[0] == '#') continue;
        std::vector<i32> l;
        std::istringstream ls(linha);
        std::string campo;
        while (std::getline(ls, campo, ',')) l.push_back(campo.empty() ? MapaTiles::VAZIO : std::stoi(campo));
        linhas.push_back(std::move(l));
    }
    return linhas;
}

// Decodifica uma linha UTF-8 em "caracteres" (cada um pode ter 1-4 bytes),
// para a legenda aceitar símbolos como "▲" além de ASCII.
std::vector<std::string> caracteres(const std::string& s) {
    std::vector<std::string> r;
    for (std::size_t i = 0; i < s.size();) {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        const std::size_t n = c < 0x80 ? 1 : (c >> 5) == 0x6 ? 2 : (c >> 4) == 0xE ? 3 : 4;
        r.push_back(s.substr(i, n));
        i += n;
    }
    return r;
}

} // namespace

bool MapaTiles::carregar(Assets& assets, const std::string& jsonRelativo, std::string* erro) {
    try {
        const json doc = json::parse(assets.lerTexto(jsonRelativo));
        id_ = doc.value("id", jsonRelativo);
        nome_ = doc.value("nome", id_);
        seguro_ = doc.value("seguro", false);
        tile_ = doc.value("tamanhoTile", 16);
        tileArte_ = doc.value("tamanhoTileArte", tile_);  // tileset em resolução dobrada: 32
        if (doc.contains("ambiente") && doc["ambiente"].size() == 4) {
            const auto& a = doc["ambiente"];
            ambiente_ = {a[0].get<u8>(), a[1].get<u8>(), a[2].get<u8>(), a[3].get<u8>()};
        }
        tileset_ = assets.textura(doc.at("tileset").get<std::string>());
        colunasTileset_ = tileset_->valida() ? std::max(1, tileset_->get().width / tileArte_) : 1;

        const std::set<i32> solidos = doc.value("solidos", std::set<i32>{});

        if (doc.contains("linhas")) {
            // --- matriz de caracteres + legenda ---
            const auto linhas = doc["linhas"].get<std::vector<std::string>>();
            altura_ = static_cast<i32>(linhas.size());
            largura_ = 0;
            std::vector<std::vector<std::string>> grade;
            for (const auto& l : linhas) {
                grade.push_back(caracteres(l));
                largura_ = std::max(largura_, static_cast<i32>(grade.back().size()));
            }
            const std::size_t n = static_cast<std::size_t>(largura_ * altura_);
            chao_.assign(n, VAZIO);
            paredes_.assign(n, VAZIO);
            decoracao_.assign(n, VAZIO);
            decoracaoAlta_.assign(n, 0);
            const json& legenda = doc.at("legenda");
            for (i32 y = 0; y < altura_; ++y) {
                for (i32 x = 0; x < largura_; ++x) {
                    const std::string c = x < static_cast<i32>(grade[y].size()) ? grade[y][x] : " ";
                    if (!legenda.contains(c)) continue;
                    const json& e = legenda[c];
                    const i32 k = indice(x, y);
                    chao_[k] = e.value("chao", VAZIO);
                    paredes_[k] = e.value("parede", VAZIO);
                    decoracao_[k] = e.value("decoracao", VAZIO);
                    decoracaoAlta_[k] = e.value("alto", false) ? 1 : 0;
                }
            }
        } else {
            // --- camadas numéricas (inline ou CSV, ex.: exportadas do Tiled) ---
            const json& camadas = doc.at("camadas");
            auto ler = [&](const char* nome) -> std::vector<std::vector<i32>> {
                if (!camadas.contains(nome)) return {};
                const json& c = camadas[nome];
                if (c.is_string()) return lerCsv(assets.lerTexto(c.get<std::string>()));
                return c.get<std::vector<std::vector<i32>>>();
            };
            const auto chao = ler("chao"), paredes = ler("paredes"), deco = ler("decoracao");
            altura_ = static_cast<i32>(chao.size());
            largura_ = altura_ ? static_cast<i32>(chao[0].size()) : 0;
            const std::size_t n = static_cast<std::size_t>(largura_ * altura_);
            chao_.assign(n, VAZIO);
            paredes_.assign(n, VAZIO);
            decoracao_.assign(n, VAZIO);
            decoracaoAlta_.assign(n, 0);
            auto copiar = [&](const std::vector<std::vector<i32>>& src, std::vector<i32>& dst) {
                for (i32 y = 0; y < std::min<i32>(altura_, static_cast<i32>(src.size())); ++y)
                    for (i32 x = 0; x < std::min<i32>(largura_, static_cast<i32>(src[y].size())); ++x)
                        dst[indice(x, y)] = src[y][x];
            };
            copiar(chao, chao_);
            copiar(paredes, paredes_);
            copiar(deco, decoracao_);
            const std::set<i32> altos = doc.value("altos", std::set<i32>{});
            for (std::size_t k = 0; k < n; ++k) decoracaoAlta_[k] = altos.count(decoracao_[k]) ? 1 : 0;
        }
        if (largura_ <= 0 || altura_ <= 0) throw std::runtime_error("mapa vazio");

        solido_.assign(static_cast<std::size_t>(largura_ * altura_), 0);
        for (std::size_t k = 0; k < solido_.size(); ++k)
            solido_[k] = (paredes_[k] != VAZIO) || (decoracao_[k] != VAZIO && solidos.count(decoracao_[k])) ? 1 : 0;

        // --- objetos ---
        objetos_.clear();
        const float t = static_cast<float>(tile_);
        auto paraPx = [t](float x, float y) {
            // Coordenadas inteiras = centro do tile; frações = posição livre.
            const bool inteiro = std::floor(x) == x && std::floor(y) == y;
            return inteiro ? Vector2{x * t + t * 0.5f, y * t + t * 0.5f} : Vector2{x * t, y * t};
        };
        if (doc.contains("objetos")) {
            for (const auto& o : doc["objetos"]) {
                ObjetoMapa m;
                m.tipo = o.at("tipo").get<std::string>();
                m.id = o.value("id", "");
                m.classe = o.value("classe", "");
                m.nome = o.value("nome", "");
                m.destino = o.value("destino", "");
                m.spawn = o.value("spawn", "");
                m.rotulo = o.value("rotulo", "");
                m.sprite = o.value("sprite", "");
                m.chave = o.value("chave", "");
                m.uid = o.value("uid", "");
                m.direcao = o.value("direcao", "baixo");
                m.raio = o.value("raio", 0.f) * (o.value("raioEmTiles", true) ? t : 1.f);
                m.escala = o.value("escala", 1.f);
                const float x = o.value("x", 0.f), y = o.value("y", 0.f);
                m.pos = paraPx(x, y);
                if (o.contains("w")) {
                    m.area = {x * t, y * t, o.value("w", 1.f) * t, o.value("h", 1.f) * t};
                    m.pos = {m.area.x + m.area.width * 0.5f, m.area.y + m.area.height * 0.5f};
                }
                if (o.contains("cor") && o["cor"].size() >= 3)
                    m.cor = {o["cor"][0].get<u8>(), o["cor"][1].get<u8>(), o["cor"][2].get<u8>(), 255};
                if (o.contains("patrulha"))
                    for (const auto& p : o["patrulha"]) m.patrulha.push_back(paraPx(p[0].get<float>(), p[1].get<float>()));
                objetos_.push_back(std::move(m));
            }
        }
        return true;
    } catch (const std::exception& e) {
        if (erro) *erro = std::string(jsonRelativo) + ": " + e.what();
        return false;
    }
}

const ObjetoMapa* MapaTiles::spawn(const std::string& nome) const {
    const ObjetoMapa* primeiro = nullptr;
    for (const auto& o : objetos_) {
        if (o.tipo != "spawn") continue;
        if (!primeiro) primeiro = &o;
        if (o.nome == nome) return &o;
    }
    return primeiro;
}

bool MapaTiles::solidoTile(i32 tx, i32 ty) const {
    if (!dentro(tx, ty)) return true;  // fora do mapa é parede
    return solido_[static_cast<std::size_t>(indice(tx, ty))] != 0;
}

bool MapaTiles::solidoEm(Vector2 p) const {
    return solidoTile(static_cast<i32>(std::floor(p.x / tile_)), static_cast<i32>(std::floor(p.y / tile_)));
}

bool MapaTiles::colideCirculo(Vector2 c, float r) const {
    const i32 x0 = static_cast<i32>(std::floor((c.x - r) / tile_)), x1 = static_cast<i32>(std::floor((c.x + r) / tile_));
    const i32 y0 = static_cast<i32>(std::floor((c.y - r) / tile_)), y1 = static_cast<i32>(std::floor((c.y + r) / tile_));
    for (i32 ty = y0; ty <= y1; ++ty) {
        for (i32 tx = x0; tx <= x1; ++tx) {
            if (!solidoTile(tx, ty)) continue;
            // Círculo x AABB: ponto mais próximo do retângulo ao centro.
            const float px = std::clamp(c.x, static_cast<float>(tx * tile_), static_cast<float>((tx + 1) * tile_));
            const float py = std::clamp(c.y, static_cast<float>(ty * tile_), static_cast<float>((ty + 1) * tile_));
            const float dx = c.x - px, dy = c.y - py;
            if (dx * dx + dy * dy < r * r) return true;
        }
    }
    return false;
}

Vector2 MapaTiles::moverCirculo(Vector2 pos, float r, Vector2 delta) const {
    // Sub-passos de no máximo 4 px: nada atravessa parede em dash rápido.
    const float dist = Vector2Length(delta);
    const i32 passos = std::max(1, static_cast<i32>(std::ceil(dist / 4.f)));
    const Vector2 d{delta.x / passos, delta.y / passos};
    for (i32 i = 0; i < passos; ++i) {
        const Vector2 tx{pos.x + d.x, pos.y};
        if (!colideCirculo(tx, r)) pos = tx;
        const Vector2 ty{pos.x, pos.y + d.y};
        if (!colideCirculo(ty, r)) pos = ty;
    }
    return pos;
}

bool MapaTiles::linhaDeVisao(Vector2 a, Vector2 b) const {
    const float dist = Vector2Distance(a, b);
    const i32 n = std::max(1, static_cast<i32>(dist / (tile_ * 0.5f)));
    for (i32 i = 1; i < n; ++i) {
        const Vector2 p = Vector2Lerp(a, b, static_cast<float>(i) / n);
        const i32 tx = static_cast<i32>(std::floor(p.x / tile_)), ty = static_cast<i32>(std::floor(p.y / tile_));
        if (dentro(tx, ty) && paredes_[static_cast<std::size_t>(indice(tx, ty))] != VAZIO) return false;
    }
    return true;
}

void MapaTiles::desenharTile(i32 tile, float x, float y) const {
    if (tile < 0 || !tileset_ || !tileset_->valida()) return;
    const float a = static_cast<float>(tileArte_);
    const Rectangle src{(tile % colunasTileset_) * a, (tile / colunasTileset_) * a, a, a};
    const float t = static_cast<float>(tile_);
    DrawTexturePro(tileset_->get(), src, {x, y, t, t}, {0, 0}, 0.f, WHITE);
}

void MapaTiles::desenharChao(Rectangle v) const {
    const i32 x0 = std::max(0, static_cast<i32>(v.x / tile_) - 1);
    const i32 y0 = std::max(0, static_cast<i32>(v.y / tile_) - 1);
    const i32 x1 = std::min(largura_ - 1, static_cast<i32>((v.x + v.width) / tile_) + 1);
    const i32 y1 = std::min(altura_ - 1, static_cast<i32>((v.y + v.height) / tile_) + 1);
    for (i32 y = y0; y <= y1; ++y)
        for (i32 x = x0; x <= x1; ++x) {
            const auto k = static_cast<std::size_t>(indice(x, y));
            desenharTile(chao_[k], static_cast<float>(x * tile_), static_cast<float>(y * tile_));
            desenharTile(paredes_[k], static_cast<float>(x * tile_), static_cast<float>(y * tile_));
        }
}

void MapaTiles::enfileirarDecoracao(FilaRender& fila, Rectangle v) const {
    const i32 x0 = std::max(0, static_cast<i32>(v.x / tile_) - 1);
    const i32 y0 = std::max(0, static_cast<i32>(v.y / tile_) - 1);
    const i32 x1 = std::min(largura_ - 1, static_cast<i32>((v.x + v.width) / tile_) + 1);
    const i32 y1 = std::min(altura_ - 1, static_cast<i32>((v.y + v.height) / tile_) + 2);
    for (i32 y = y0; y <= y1; ++y)
        for (i32 x = x0; x <= x1; ++x) {
            const auto k = static_cast<std::size_t>(indice(x, y));
            const i32 tile = decoracao_[k];
            if (tile == VAZIO) continue;
            const bool alto = decoracaoAlta_[k] != 0;
            const float px = static_cast<float>(x * tile_), py = static_cast<float>(y * tile_);
            // Pés do objeto = base do tile (menos 2 px para quem passa rente ficar atrás).
            fila.adicionar(py + tile_ - 2.f, [this, tile, alto, px, py] {
                if (alto) desenharTile(tile - colunasTileset_, px, py - tile_);
                desenharTile(tile, px, py);
            });
        }
}

} // namespace andv

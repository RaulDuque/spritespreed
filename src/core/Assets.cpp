#include "andvarion/core/Assets.hpp"
#include "andvarion/core/Texto.hpp"
#include "andvarion/gfx/SpriteSheet.hpp"

#include <algorithm>
#include <filesystem>

namespace andv {

namespace fs = std::filesystem;

// Caminhos usam a codificação NATIVA (no Windows, a página de código ANSI),
// igual à raylib (GetApplicationDirectory/fopen). u8path quebraria pastas com
// acento, como "Área de Trabalho".

Textura::~Textura() {
    if (tex_.id != 0 && IsWindowReady()) UnloadTexture(tex_);
}

Assets::Assets() {
    // Preferência: assets/ ao lado do executável (copiado pelo CMake); senão o diretório atual.
    const std::string aoLado = std::string(GetApplicationDirectory()) + "assets/";
    raiz_ = DirectoryExists(aoLado.c_str()) ? aoLado : std::string("assets/");
    TraceLog(LOG_INFO, "ANDVARION: assets em %s", raiz_.c_str());
}

std::string Assets::caminho(const std::string& relativo) const { return raiz_ + relativo; }

bool Assets::existe(const std::string& relativo) const { return FileExists(caminho(relativo).c_str()); }

std::string Assets::lerTexto(const std::string& relativo) const {
    auto t = texto::lerArquivo(caminho(relativo));
    if (!t) TraceLog(LOG_WARNING, "ANDVARION: arquivo ausente: %s", relativo.c_str());
    return t.value_or("");
}

std::shared_ptr<Textura> Assets::textura(const std::string& relativo) {
    auto it = texturas_.find(relativo);
    if (it != texturas_.end()) return it->second;
    Texture2D t{};
    if (existe(relativo)) {
        t = LoadTexture(caminho(relativo).c_str());
        SetTextureFilter(t, TEXTURE_FILTER_POINT);  // pixel art: sempre nearest
    } else {
        TraceLog(LOG_WARNING, "ANDVARION: textura ausente: %s", relativo.c_str());
    }
    auto p = std::make_shared<Textura>(t);
    texturas_[relativo] = p;
    return p;
}

std::shared_ptr<const SpriteSheet> Assets::sheet(const std::string& jsonRelativo) {
    auto it = sheets_.find(jsonRelativo);
    if (it != sheets_.end()) return it->second;

    std::string erro;
    const std::string conteudo = lerTexto(jsonRelativo);
    auto atlas = conteudo.empty() ? std::nullopt : AtlasAseprite::deJson(conteudo, &erro);
    if (!atlas) {
        if (!conteudo.empty()) TraceLog(LOG_WARNING, "ANDVARION: atlas inválido %s: %s", jsonRelativo.c_str(), erro.c_str());
        sheets_[jsonRelativo] = nullptr;
        return nullptr;
    }
    // meta.image é relativo ao próprio JSON.
    const std::string pasta = fs::path(jsonRelativo).parent_path().generic_string();
    const std::string imagem = (pasta.empty() ? "" : pasta + "/") + atlas->imagem();
    auto s = std::make_shared<const SpriteSheet>(std::move(*atlas), textura(imagem));
    sheets_[jsonRelativo] = s;
    return s;
}

std::shared_ptr<const ConjuntoSprites> Assets::personagem(const std::string& pasta) {
    auto it = personagens_.find(pasta);
    if (it != personagens_.end()) return it->second;

    auto conjunto = std::make_shared<ConjuntoSprites>();
    std::error_code ec;
    std::vector<std::string> jsons;
    for (const auto& e : fs::directory_iterator(fs::path(caminho(pasta)), ec)) {
        const auto nome = e.path().filename().string();
        if (e.path().extension() == ".json" && nome != "engine-export.json") jsons.push_back(nome);
    }
    std::sort(jsons.begin(), jsons.end());  // páginas em ordem estável
    for (const auto& j : jsons)
        if (auto s = sheet(pasta + "/" + j)) conjunto->adicionar(s);
    if (conjunto->vazio()) TraceLog(LOG_WARNING, "ANDVARION: nenhum atlas em %s", pasta.c_str());
    personagens_[pasta] = conjunto;
    return conjunto;
}

void Assets::limpar() {
    personagens_.clear();
    sheets_.clear();
    texturas_.clear();
}

} // namespace andv

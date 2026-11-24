#include "andvarion/save/SaveSystem.hpp"
#include "andvarion/core/Texto.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>

namespace andv {

using json = nlohmann::json;

SaveSystem::SaveSystem(std::string caminho) : caminho_(std::move(caminho)) {}

bool SaveSystem::existe() const {
    std::error_code ec;
    // Codificação nativa, a mesma de GetApplicationDirectory e fopen (ver Assets.cpp).
    return std::filesystem::exists(std::filesystem::path(caminho_), ec);
}

std::string SaveSystem::paraJson(const SaveData& d) {
    json j;
    j["versao"] = d.versao;
    j["area"] = d.area;
    if (d.temPosicao) j["posicao"] = {d.posX, d.posY};
    j["nivel"] = d.nivel;
    j["xp"] = d.xp;
    j["pontos"] = d.pontos;
    j["vida"] = d.vida;
    j["mana"] = d.mana;
    j["frascos"] = d.frascos;
    j["frascosMax"] = d.frascosMax;
    j["armaCorpo"] = d.armaCorpo;
    j["armaDistancia"] = d.armaDistancia;
    j["bonusDanoForja"] = d.bonusDanoForja;
    j["habilidades"] = d.habilidades;
    j["inventario"] = d.inventario;
    j["equipamento"] = d.equipamento;
    json prof = json::object();
    for (const auto& [chave, v] : d.proficiencias) prof[chave] = {v.first, v.second};
    j["proficiencias"] = prof;
    j["grimorio"] = d.grimorio;
    j["coletados"] = d.coletados;
    j["tempoJogoSeg"] = d.tempoJogoSeg;
    json m = json::object();
    for (const auto& [id, inst] : d.missoes)
        m[id] = {{"estado", nomeEstado(inst.estado)}, {"progresso", inst.progresso}};
    j["missoes"] = m;
    return j.dump(2);
}

std::optional<SaveData> SaveSystem::deJson(const std::string& texto, std::string* erro) {
    try {
        const json j = json::parse(texto);
        SaveData d;
        d.versao = j.value("versao", 1);
        d.area = j.value("area", d.area);
        if (j.contains("posicao") && j["posicao"].size() == 2) {
            d.posX = j["posicao"][0].get<float>();
            d.posY = j["posicao"][1].get<float>();
            d.temPosicao = true;
        }
        d.nivel = j.value("nivel", 1);
        d.xp = j.value("xp", 0);
        d.pontos = j.value("pontos", 0);
        d.vida = j.value("vida", -1.f);
        d.mana = j.value("mana", -1.f);
        d.frascos = j.value("frascos", 3);
        d.frascosMax = j.value("frascosMax", 3);
        d.armaCorpo = j.value("armaCorpo", d.armaCorpo);
        d.armaDistancia = j.value("armaDistancia", d.armaDistancia);
        d.bonusDanoForja = j.value("bonusDanoForja", 0.f);
        d.habilidades = j.value("habilidades", std::vector<std::string>{});
        d.inventario = j.value("inventario", std::map<std::string, i32>{});
        d.equipamento = j.value("equipamento", std::map<std::string, std::string>{});
        if (j.contains("proficiencias"))
            for (const auto& [chave, v] : j["proficiencias"].items())
                if (v.is_array() && v.size() == 2) d.proficiencias[chave] = {v[0].get<i32>(), v[1].get<i32>()};
        d.grimorio = j.value("grimorio", std::vector<std::string>{});
        d.coletados = j.value("coletados", std::vector<std::string>{});
        d.tempoJogoSeg = j.value("tempoJogoSeg", 0.0);
        if (j.contains("missoes")) {
            for (const auto& [id, mj] : j["missoes"].items()) {
                QuestManager::Instantaneo inst{EstadoMissao::NaoIniciada, {}};
                if (auto e = estadoDeTexto(mj.value("estado", "NAO_INICIADA"))) inst.estado = *e;
                inst.progresso = mj.value("progresso", std::vector<i32>{});
                d.missoes[id] = inst;
            }
        }
        return d;
    } catch (const std::exception& e) {
        if (erro) *erro = e.what();
        return std::nullopt;
    }
}

bool SaveSystem::salvar(const SaveData& d, std::string* erro) const {
    std::error_code ec;
    const auto pasta = std::filesystem::path(caminho_).parent_path();
    if (!pasta.empty()) std::filesystem::create_directories(pasta, ec);
    if (!texto::gravarArquivo(caminho_, paraJson(d))) {
        if (erro) *erro = "não foi possível gravar " + caminho_;
        return false;
    }
    return true;
}

std::optional<SaveData> SaveSystem::carregar(std::string* erro) const {
    const auto conteudo = texto::lerArquivo(caminho_);
    if (!conteudo) {
        if (erro) *erro = "save não encontrado: " + caminho_;
        return std::nullopt;
    }
    return deJson(*conteudo, erro);
}

} // namespace andv

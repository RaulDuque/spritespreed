#include "andvarion/cenas/Sessao.hpp"
#include "andvarion/core/Assets.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>

#include <raylib.h>

namespace andv {

namespace {
void avisarErro(const char* oque, const std::string& erro) {
    TraceLog(LOG_ERROR, "ANDVARION: %s inválido: %s", oque, erro.c_str());
}
} // namespace

Sessao::Sessao(EventBus& b, Assets& assets) : bus(b) {
    std::string erro;
    const std::string itens = assets.lerTexto("dados/itens.json");
    if (!inventario.carregarCatalogo(itens, &erro)) avisarErro("itens.json", erro);
    const auto doc = nlohmann::json::parse(itens, nullptr, false);
    if (!doc.is_discarded() && doc.contains("kitInicial")) {
        kitItens_ = doc["kitInicial"].value("itens", std::vector<std::string>{});
        kitEquipados_ = doc["kitInicial"].value("equipados", std::vector<std::string>{});
    }
    if (!arvore.carregar(assets.lerTexto("dados/arvore_habilidades.json"), &erro)) avisarErro("arvore_habilidades.json", erro);
    if (!dialogos.carregar(assets.lerTexto("dados/dialogos.json"), &erro)) avisarErro("dialogos.json", erro);
    missoes = std::make_unique<QuestManager>(bus, [this](const std::string& id) { return inventario.quantidade(id); });
    if (!missoes->carregar(assets.lerTexto("dados/missoes.json"), &erro)) avisarErro("missoes.json", erro);
}

void Sessao::organizarGrimorio() {
    for (auto& f : grimorio)
        if (!f.empty() && !arvore.feiticoLiberado(f)) f.clear();
    for (const auto& def : feiticos()) {
        if (!arvore.feiticoLiberado(def.id)) continue;
        if (std::find(grimorio.begin(), grimorio.end(), def.id) != grimorio.end()) continue;
        for (auto& f : grimorio)
            if (f.empty()) {
                f = def.id;
                break;
            }
    }
}

bool Sessao::atribuirMagia(int slot, const std::string& feitico) {
    if (slot < 0 || slot >= SLOTS_GRIMORIO || !arvore.feiticoLiberado(feitico)) return false;
    for (auto& f : grimorio)
        if (f == feitico) f.clear();
    grimorio[static_cast<std::size_t>(slot)] = feitico;
    return true;
}

void Sessao::aplicarKitInicial() {
    for (const auto& id : kitItens_)
        if (inventario.quantidade(id) == 0) inventario.adicionar(id);
    for (const auto& id : kitEquipados_) equipamento.equipar(id, inventario);
}

} // namespace andv

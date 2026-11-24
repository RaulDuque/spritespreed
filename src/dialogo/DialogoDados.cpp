#include "andvarion/dialogo/DialogoDados.hpp"
#include "andvarion/missoes/QuestManager.hpp"
#include "andvarion/progressao/Inventario.hpp"

#include <nlohmann/json.hpp>

#include <sstream>

namespace andv {

namespace {

std::vector<std::string> lerLista(const nlohmann::json& j, const char* chave) {
    if (!j.contains(chave)) return {};
    const auto& v = j[chave];
    if (v.is_string()) return {v.get<std::string>()};
    return v.get<std::vector<std::string>>();
}

std::vector<std::string> dividir(const std::string& s, char sep) {
    std::vector<std::string> partes;
    std::stringstream ss(s);
    std::string p;
    while (std::getline(ss, p, sep)) partes.push_back(p);
    return partes;
}

} // namespace

bool BancoDialogos::carregar(const std::string& texto, std::string* erro) {
    try {
        const auto doc = nlohmann::json::parse(texto);
        for (const auto& [id, n] : doc.at("npcs").items()) {
            DialogoNpc d;
            d.id      = id;
            d.nome    = n.value("nome", id);
            d.retrato = n.value("retrato", "");
            for (const auto& e : n.at("entradas"))
                d.entradas.push_back({lerLista(e, "se"), e.at("no").get<std::string>()});
            for (const auto& [noId, no] : n.at("nos").items()) {
                NoDialogo nd;
                nd.id      = noId;
                nd.falante = no.value("falante", "");
                nd.texto   = no.value("texto", "");
                nd.proximo = no.contains("proximo") && no["proximo"].is_string() ? no["proximo"].get<std::string>() : "";
                nd.acoes   = lerLista(no, "acoes");
                if (no.contains("opcoes")) {
                    for (const auto& o : no["opcoes"]) {
                        OpcaoDialogo op;
                        op.texto     = o.value("texto", "...");
                        op.proximo   = o.contains("proximo") && o["proximo"].is_string() ? o["proximo"].get<std::string>() : "";
                        op.acoes     = lerLista(o, "acoes");
                        op.condicoes = lerLista(o, "se");
                        nd.opcoes.push_back(std::move(op));
                    }
                }
                d.nos[noId] = std::move(nd);
            }
            // Validação: toda referência precisa existir, senão o jogo travaria no meio da conversa.
            auto existe = [&](const std::string& alvo) { return alvo.empty() || d.nos.count(alvo); };
            for (const auto& e : d.entradas)
                if (!d.nos.count(e.no)) throw std::runtime_error(id + ": entrada aponta para nó inexistente '" + e.no + "'");
            for (const auto& [noId, no] : d.nos) {
                if (!existe(no.proximo)) throw std::runtime_error(id + "/" + noId + ": proximo inexistente '" + no.proximo + "'");
                for (const auto& o : no.opcoes)
                    if (!existe(o.proximo)) throw std::runtime_error(id + "/" + noId + ": opção aponta para '" + o.proximo + "'");
            }
            npcs_[id] = std::move(d);
        }
        return true;
    } catch (const std::exception& e) {
        if (erro) *erro = e.what();
        return false;
    }
}

const DialogoNpc* BancoDialogos::npc(const std::string& id) const {
    auto it = npcs_.find(id);
    return it == npcs_.end() ? nullptr : &it->second;
}

bool avaliarCondicao(const std::string& condicaoBruta, const QuestManager& q, const Inventario& inv) {
    if (condicaoBruta.empty()) return true;
    const bool negar = condicaoBruta[0] == '!';
    const auto p = dividir(negar ? condicaoBruta.substr(1) : condicaoBruta, ':');
    bool r = false;
    if (p.size() == 3 && p[0] == "missao") {
        const auto e = estadoDeTexto(p[2]);
        r = e && q.estado(p[1]) == *e;
    } else if (p.size() == 2 && p[0] == "pode_aceitar") {
        r = q.podeAceitar(p[1]);
    } else if (p.size() == 3 && p[0] == "item") {
        r = inv.quantidade(p[1]) >= std::stoi(p[2]);
    } else if (p.size() == 4 && p[0] == "objetivo" && p[3] == "completo") {
        const Missao* m = q.missao(p[1]);
        const auto i = static_cast<std::size_t>(std::stoi(p[2]));
        r = m && i < m->objetivos.size() && m->objetivos[i].completo();
    }
    return negar ? !r : r;
}

bool avaliarCondicoes(const std::vector<std::string>& condicoes, const QuestManager& q, const Inventario& inv) {
    for (const auto& c : condicoes)
        if (!avaliarCondicao(c, q, inv)) return false;
    return true;
}

std::optional<std::string> escolherEntrada(const DialogoNpc& npc, const QuestManager& q, const Inventario& inv) {
    for (const auto& e : npc.entradas)
        if (avaliarCondicoes(e.condicoes, q, inv)) return e.no;
    return std::nullopt;
}

} // namespace andv

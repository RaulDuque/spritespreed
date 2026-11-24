#include "andvarion/progressao/ArvoreHabilidades.hpp"
#include "andvarion/progressao/Progressao.hpp"

#include <nlohmann/json.hpp>

namespace andv {

bool ArvoreHabilidades::carregar(const std::string& texto, std::string* erro) {
    try {
        const auto doc = nlohmann::json::parse(texto);
        nos_.clear();
        galhos_.clear();
        if (doc.contains("galhos")) {
            for (const auto& g : doc["galhos"]) {
                GalhoHabilidade gh;
                gh.id = g.at("id").get<std::string>();
                gh.nome = g.value("nome", gh.id);
                gh.ramo = g.value("ramo", "mago");
                gh.escola = g.value("escola", "");
                gh.angulo = g.value("angulo", 0.f);
                gh.comprimento = g.value("comprimento", 80.f);
                galhos_.push_back(gh);
            }
        }
        for (const auto& n : doc.at("nos")) {
            NoHabilidade h;
            h.id          = n.at("id").get<std::string>();
            h.nome        = n.value("nome", h.id);
            h.descricao   = n.value("descricao", "");
            h.galho       = n.value("galho", "");
            h.ordem       = n.value("ordem", 0);
            h.ramo        = n.value("ramo", "");
            h.custo       = n.value("custo", 1);
            h.nivelMinimo = n.value("nivelMinimo", 1);
            h.requisitos  = n.value("requisitos", std::vector<std::string>{});
            h.coluna      = n.value("coluna", 0);
            h.linha       = n.value("linha", 0);
            h.vfx         = n.value("vfx", "habilidade_desbloqueada");
            const auto& ef = n.at("efeito");
            const std::string tipo = ef.value("tipo", "atributo");
            h.efeito = tipo == "feitico" ? TipoEfeitoHabilidade::Feitico
                     : tipo == "passiva" ? TipoEfeitoHabilidade::Passiva
                                         : TipoEfeitoHabilidade::Atributo;
            h.alvo  = ef.value("alvo", "");
            h.valor = ef.value("valor", 0.f);
            nos_.push_back(std::move(h));
        }
        // Ramo herdado do galho e requisito implícito: o nó anterior do mesmo galho.
        for (auto& n : nos_) {
            if (n.galho.empty()) continue;
            const GalhoHabilidade* g = galho(n.galho);
            if (!g) throw std::runtime_error("nó " + n.id + " em galho inexistente: " + n.galho);
            if (n.ramo.empty()) n.ramo = g->ramo;
            if (n.requisitos.empty() && n.ordem > 0)
                for (const auto& outro : nos_)
                    if (outro.galho == n.galho && outro.ordem == n.ordem - 1) n.requisitos.push_back(outro.id);
        }
        // Nós com custo 0 vêm desbloqueados (habilidades iniciais).
        for (const auto& n : nos_)
            if (n.custo == 0) desbloqueados_.insert(n.id);
        return true;
    } catch (const std::exception& e) {
        if (erro) *erro = e.what();
        return false;
    }
}

const GalhoHabilidade* ArvoreHabilidades::galho(const std::string& id) const {
    for (const auto& g : galhos_)
        if (g.id == id) return &g;
    return nullptr;
}

const NoHabilidade* ArvoreHabilidades::no(const std::string& id) const {
    for (const auto& n : nos_)
        if (n.id == id) return &n;
    return nullptr;
}

ResultadoDesbloqueio ArvoreHabilidades::podeDesbloquear(const std::string& id, const Progressao& p) const {
    const NoHabilidade* n = no(id);
    if (!n) return ResultadoDesbloqueio::Inexistente;
    if (desbloqueado(id)) return ResultadoDesbloqueio::JaDesbloqueado;
    for (const auto& r : n->requisitos)
        if (!desbloqueado(r)) return ResultadoDesbloqueio::RequisitoFaltando;
    if (p.nivel() < n->nivelMinimo) return ResultadoDesbloqueio::NivelInsuficiente;
    if (p.pontos() < n->custo) return ResultadoDesbloqueio::SemPontos;
    return ResultadoDesbloqueio::Ok;
}

ResultadoDesbloqueio ArvoreHabilidades::desbloquear(const std::string& id, Progressao& p) {
    const auto r = podeDesbloquear(id, p);
    if (r != ResultadoDesbloqueio::Ok) return r;
    p.gastarPontos(no(id)->custo);
    desbloqueados_.insert(id);
    return r;
}

float ArvoreHabilidades::bonus(const std::string& atributo) const {
    float total = 0.f;
    for (const auto& n : nos_)
        if (n.efeito == TipoEfeitoHabilidade::Atributo && n.alvo == atributo && desbloqueado(n.id))
            total += n.valor;
    return total;
}

bool ArvoreHabilidades::temPassiva(const std::string& passiva) const {
    for (const auto& n : nos_)
        if (n.efeito == TipoEfeitoHabilidade::Passiva && n.alvo == passiva && desbloqueado(n.id)) return true;
    return false;
}

bool ArvoreHabilidades::feiticoLiberado(const std::string& feitico) const {
    for (const auto& n : nos_)
        if (n.efeito == TipoEfeitoHabilidade::Feitico && n.alvo == feitico && desbloqueado(n.id)) return true;
    return false;
}

void ArvoreHabilidades::restaurar(const std::vector<std::string>& ids) {
    desbloqueados_.clear();
    for (const auto& n : nos_)
        if (n.custo == 0) desbloqueados_.insert(n.id);
    for (const auto& id : ids)
        if (no(id)) desbloqueados_.insert(id);
}

const char* textoResultado(ResultadoDesbloqueio r) {
    switch (r) {
        case ResultadoDesbloqueio::Ok:                return "Habilidade desbloqueada!";
        case ResultadoDesbloqueio::JaDesbloqueado:    return "Você já domina esta habilidade.";
        case ResultadoDesbloqueio::SemPontos:         return "Pontos de habilidade insuficientes.";
        case ResultadoDesbloqueio::NivelInsuficiente: return "Nível insuficiente.";
        case ResultadoDesbloqueio::RequisitoFaltando: return "Desbloqueie as habilidades anteriores primeiro.";
        case ResultadoDesbloqueio::Inexistente:       return "Habilidade desconhecida.";
    }
    return "";
}

} // namespace andv

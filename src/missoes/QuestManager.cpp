#include "andvarion/missoes/QuestManager.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>

namespace andv {

const char* nomeEstado(EstadoMissao e) {
    switch (e) {
        case EstadoMissao::NaoIniciada:        return "NAO_INICIADA";
        case EstadoMissao::Ativa:              return "ATIVA";
        case EstadoMissao::ProntaParaEntregar: return "PRONTA_PARA_ENTREGAR";
        case EstadoMissao::Concluida:          return "CONCLUIDA";
    }
    return "NAO_INICIADA";
}

std::optional<EstadoMissao> estadoDeTexto(const std::string& s) {
    if (s == "NAO_INICIADA") return EstadoMissao::NaoIniciada;
    if (s == "ATIVA") return EstadoMissao::Ativa;
    if (s == "PRONTA_PARA_ENTREGAR") return EstadoMissao::ProntaParaEntregar;
    if (s == "CONCLUIDA") return EstadoMissao::Concluida;
    return std::nullopt;
}

QuestManager::QuestManager(EventBus& bus, ContadorItens contarItem)
    : bus_(bus), contarItem_(std::move(contarItem)) {
    auto ouvir = [this](const Evento& e) { aoEvento(e); };
    inscricoes_.push_back(bus_.inscrever(TipoEvento::InimigoMorto, ouvir));
    inscricoes_.push_back(bus_.inscrever(TipoEvento::ItemColetado, ouvir));
    inscricoes_.push_back(bus_.inscrever(TipoEvento::NpcFalado, ouvir));
}

bool QuestManager::carregar(const std::string& texto, std::string* erro) {
    try {
        const auto doc = nlohmann::json::parse(texto);
        missoes_.clear();
        for (const auto& j : doc.at("missoes")) {
            Missao m;
            m.id            = j.at("id").get<std::string>();
            m.titulo        = j.value("titulo", m.id);
            m.descricao     = j.value("descricao", "");
            m.principal     = j.value("principal", false);
            m.doador        = j.value("doador", "");
            m.entregarPara  = j.value("entregarPara", m.doador);
            m.requisito     = j.value("requisito", "");
            m.consumirItens = j.value("consumirItens", true);
            for (const auto& o : j.at("objetivos")) {
                Objetivo ob;
                const std::string tipo = o.at("tipo").get<std::string>();
                if (tipo == "MATAR_INIMIGO") ob.tipo = TipoObjetivo::MatarInimigo;
                else if (tipo == "COLETAR_ITEM") ob.tipo = TipoObjetivo::ColetarItem;
                else if (tipo == "FALAR_NPC") ob.tipo = TipoObjetivo::FalarNpc;
                else throw std::runtime_error("tipo de objetivo desconhecido: " + tipo);
                ob.alvo       = o.at("alvo").get<std::string>();
                ob.quantidade = std::max(1, o.value("quantidade", 1));
                ob.descricao  = o.value("descricao", ob.alvo);
                m.objetivos.push_back(ob);
            }
            if (j.contains("recompensa")) {
                const auto& r = j["recompensa"];
                m.recompensa.xp    = r.value("xp", 0);
                m.recompensa.bonus = r.value("bonus", "");
                if (r.contains("itens"))
                    for (const auto& [id, q] : r["itens"].items()) m.recompensa.itens[id] = q.get<i32>();
            }
            missoes_.push_back(std::move(m));
        }
        return true;
    } catch (const std::exception& e) {
        if (erro) *erro = e.what();
        return false;
    }
}

Missao* QuestManager::buscar(const std::string& id) {
    for (auto& m : missoes_)
        if (m.id == id) return &m;
    return nullptr;
}

const Missao* QuestManager::missao(const std::string& id) const {
    for (const auto& m : missoes_)
        if (m.id == id) return &m;
    return nullptr;
}

EstadoMissao QuestManager::estado(const std::string& id) const {
    const Missao* m = missao(id);
    return m ? m->estado : EstadoMissao::NaoIniciada;
}

bool QuestManager::podeAceitar(const std::string& id) const {
    const Missao* m = missao(id);
    if (!m || m->estado != EstadoMissao::NaoIniciada) return false;
    return m->requisito.empty() || estado(m->requisito) == EstadoMissao::Concluida;
}

bool QuestManager::aceitar(const std::string& id) {
    if (!podeAceitar(id)) return false;
    Missao& m = *buscar(id);
    m.estado = EstadoMissao::Ativa;
    bus_.publicar(TipoEvento::MissaoAceita, m.id);
    reavaliar(m);  // itens já carregados contam
    bus_.publicar(TipoEvento::MissaoAtualizada, m.id);
    return true;
}

const Recompensa* QuestManager::entregar(const std::string& id) {
    Missao* m = buscar(id);
    if (!m || m->estado != EstadoMissao::ProntaParaEntregar) return nullptr;
    m->estado = EstadoMissao::Concluida;
    bus_.publicar(TipoEvento::MissaoConcluida, m->id);
    bus_.publicar(TipoEvento::MissaoAtualizada, m->id);
    return &m->recompensa;
}

void QuestManager::reavaliar(Missao& m) {
    if (m.estado != EstadoMissao::Ativa && m.estado != EstadoMissao::ProntaParaEntregar) return;
    for (auto& o : m.objetivos)
        if (o.tipo == TipoObjetivo::ColetarItem && contarItem_)
            o.atual = std::min(o.quantidade, contarItem_(o.alvo));

    const bool tudo = std::all_of(m.objetivos.begin(), m.objetivos.end(),
                                  [](const Objetivo& o) { return o.completo(); });
    if (tudo && m.estado == EstadoMissao::Ativa) {
        m.estado = EstadoMissao::ProntaParaEntregar;
        bus_.publicar(TipoEvento::MissaoProntaParaEntregar, m.id);
    } else if (!tudo && m.estado == EstadoMissao::ProntaParaEntregar) {
        m.estado = EstadoMissao::Ativa;
    }
}

void QuestManager::aoEvento(const Evento& e) {
    for (auto& m : missoes_) {
        if (m.estado != EstadoMissao::Ativa && m.estado != EstadoMissao::ProntaParaEntregar) continue;
        bool mudou = false;
        for (auto& o : m.objetivos) {
            if (o.completo() && o.tipo != TipoObjetivo::ColetarItem) continue;
            if (o.alvo != e.id) continue;
            if ((e.tipo == TipoEvento::InimigoMorto && o.tipo == TipoObjetivo::MatarInimigo) ||
                (e.tipo == TipoEvento::NpcFalado && o.tipo == TipoObjetivo::FalarNpc)) {
                o.atual = std::min(o.quantidade, o.atual + std::max(1, e.quantidade));
                mudou = true;
            } else if (e.tipo == TipoEvento::ItemColetado && o.tipo == TipoObjetivo::ColetarItem) {
                mudou = true;  // reavaliar() relê o inventário
            }
        }
        if (mudou) {
            reavaliar(m);
            bus_.publicar(TipoEvento::MissaoAtualizada, m.id);
        }
    }
}

std::vector<const Missao*> QuestManager::rastreadas() const {
    std::vector<const Missao*> r;
    for (const auto& m : missoes_)
        if (m.estado == EstadoMissao::Ativa || m.estado == EstadoMissao::ProntaParaEntregar) r.push_back(&m);
    std::stable_sort(r.begin(), r.end(), [](const Missao* a, const Missao* b) { return a->principal > b->principal; });
    return r;
}

void QuestManager::sincronizarItens() {
    for (auto& m : missoes_) reavaliar(m);
}

std::map<std::string, QuestManager::Instantaneo> QuestManager::salvar() const {
    std::map<std::string, Instantaneo> r;
    for (const auto& m : missoes_) {
        Instantaneo i{m.estado, {}};
        for (const auto& o : m.objetivos) i.progresso.push_back(o.atual);
        r[m.id] = i;
    }
    return r;
}

void QuestManager::restaurar(const std::map<std::string, Instantaneo>& dados) {
    for (auto& m : missoes_) {
        auto it = dados.find(m.id);
        if (it == dados.end()) {
            m.estado = EstadoMissao::NaoIniciada;
            for (auto& o : m.objetivos) o.atual = 0;
            continue;
        }
        m.estado = it->second.estado;
        for (std::size_t i = 0; i < m.objetivos.size(); ++i)
            m.objetivos[i].atual = i < it->second.progresso.size() ? it->second.progresso[i] : 0;
    }
    sincronizarItens();
}

} // namespace andv

#include "andvarion/progressao/Proficiencia.hpp"

#include <algorithm>

namespace andv {

i32 Proficiencia::ganhar(const std::string& chave, i32 xp) {
    if (xp <= 0 || chave.empty()) return 0;
    auto& [nivel, atual] = dados_.try_emplace(chave, std::make_pair(1, 0)).first->second;
    if (nivel >= NIVEL_MAXIMO) return 0;
    atual += xp;
    i32 subiu = 0;
    while (nivel < NIVEL_MAXIMO && atual >= xpParaProximo(nivel)) {
        atual -= xpParaProximo(nivel);
        subiu = ++nivel;
    }
    if (nivel >= NIVEL_MAXIMO) atual = 0;
    return subiu;
}

i32 Proficiencia::nivel(const std::string& chave) const {
    auto it = dados_.find(chave);
    return it == dados_.end() ? 1 : it->second.first;
}

float Proficiencia::fracao(const std::string& chave) const {
    auto it = dados_.find(chave);
    if (it == dados_.end()) return 0.f;
    if (it->second.first >= NIVEL_MAXIMO) return 1.f;
    return static_cast<float>(it->second.second) / static_cast<float>(xpParaProximo(it->second.first));
}

void Proficiencia::restaurar(std::map<std::string, std::pair<i32, i32>> dados) {
    dados_.clear();
    for (auto& [chave, v] : dados)
        dados_[chave] = {std::clamp(v.first, 1, NIVEL_MAXIMO), std::max(0, v.second)};
}

const char* nomeProficiencia(const std::string& c) {
    static const std::map<std::string, const char*> nomes = {
        {"espada", "Espada"}, {"adaga", "Adagas"}, {"machado", "Machado"}, {"foice", "Foice"}, {"bastao", "Bastão"},
        {"arco", "Arco"}, {"besta", "Besta"}, {"bumerangue", "Bumerangue"},
        {"sangue", "Escola do Sangue"}, {"fogo", "Escola do Fogo"}, {"gelo", "Escola do Gelo"},
        {"raio", "Escola do Raio"}, {"arcano", "Escola Arcana"}, {"sombra", "Escola da Sombra"},
    };
    auto it = nomes.find(c);
    return it == nomes.end() ? "?" : it->second;
}

} // namespace andv

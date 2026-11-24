#pragma once
// Proficiencia — níveis de habilidade que sobem com o USO.
//
// Cada arma ("espada", "arco"...) e cada escola de magia ("sangue", "fogo",
// "gelo", "raio", "arcano", "sombra") tem nível 1..10. Acertos com a arma e
// conjurações da escola dão XP de proficiência. Bônus por nível acima do 1:
//   armas:   +4% de dano
//   escolas: +4% de dano mágico e -2% de custo de mana
#include "andvarion/core/Plataforma.hpp"

#include <map>
#include <string>
#include <utility>

namespace andv {

class Proficiencia {
public:
    static constexpr i32 NIVEL_MAXIMO = 10;
    static i32 xpParaProximo(i32 nivel) { return 15 * nivel; }

    // Devolve o novo nível se subiu, 0 se não.
    i32 ganhar(const std::string& chave, i32 xp);

    i32 nivel(const std::string& chave) const;
    float fracao(const std::string& chave) const;     // 0..1 para barra
    float bonusDano(const std::string& chave) const { return 0.04f * static_cast<float>(nivel(chave) - 1); }
    float reducaoCusto(const std::string& chave) const { return 0.02f * static_cast<float>(nivel(chave) - 1); }

    // chave -> {nível, xp dentro do nível}
    const std::map<std::string, std::pair<i32, i32>>& todas() const { return dados_; }
    void restaurar(std::map<std::string, std::pair<i32, i32>> dados);

private:
    std::map<std::string, std::pair<i32, i32>> dados_;
};

const char* nomeProficiencia(const std::string& chave);   // "Espada", "Escola do Fogo"...

} // namespace andv

#pragma once
// ArvoreHabilidades — árvore LITERAL carregada de assets/dados/arvore_habilidades.json.
//
// Tronco → três ramos grossos (Guerreiro, Arqueiro/Explorador, Mago) → galhos.
// Cada magia é um galho: o primeiro nó ensina a magia e os seguintes a
// melhoram. Os outros galhos são de atributos e passivas. Um nó exige, por
// padrão, o nó anterior do mesmo galho. Cada nó tem custo em pontos e um efeito:
//   "feitico"  -> libera um feitiço (alvo = id do feitiço)
//   "atributo" -> soma `valor` a um atributo (alvo = "vidaMax", "danoLamina", ...)
//   "passiva"  -> liga uma passiva (alvo = id da passiva)
// O jogo consulta os totais (bonus/temPassiva/feiticoLiberado) em vez de cada
// sistema guardar o próprio estado de upgrade.
#include "andvarion/core/Plataforma.hpp"

#include <set>
#include <string>
#include <vector>

namespace andv {

class Progressao;

enum class TipoEfeitoHabilidade : u8 { Feitico, Atributo, Passiva };

struct GalhoHabilidade {
    std::string id;
    std::string nome;                    // "Bola de Fogo"
    std::string ramo;                    // "guerreiro" | "arqueiro" | "mago"
    std::string escola;                  // cor/ícone: "fogo", "gelo"... (vazio = atributos)
    float angulo = 0.f;                  // graus a partir da vertical (negativo = esquerda)
    float comprimento = 80.f;            // px na tela da árvore
};

struct NoHabilidade {
    std::string id;
    std::string nome;
    std::string descricao;
    std::string ramo;                    // herdado do galho
    std::string galho;
    i32 ordem = 0;                       // posição ao longo do galho (0 = base)
    i32 custo = 1;
    i32 nivelMinimo = 1;
    std::vector<std::string> requisitos;
    TipoEfeitoHabilidade efeito = TipoEfeitoHabilidade::Atributo;
    std::string alvo;
    float valor = 0.f;
    i32 coluna = 0, linha = 0;           // posição na grade do menu
    std::string vfx;                     // VFX tocado ao desbloquear
};

enum class ResultadoDesbloqueio : u8 { Ok, JaDesbloqueado, SemPontos, NivelInsuficiente, RequisitoFaltando, Inexistente };

class ArvoreHabilidades {
public:
    bool carregar(const std::string& json, std::string* erro = nullptr);

    ResultadoDesbloqueio podeDesbloquear(const std::string& id, const Progressao& p) const;
    ResultadoDesbloqueio desbloquear(const std::string& id, Progressao& p);

    bool desbloqueado(const std::string& id) const { return desbloqueados_.count(id) > 0; }
    float bonus(const std::string& atributo) const;
    bool temPassiva(const std::string& passiva) const;
    bool feiticoLiberado(const std::string& feitico) const;

    const std::vector<NoHabilidade>& nos() const { return nos_; }
    const std::vector<GalhoHabilidade>& galhos() const { return galhos_; }
    const GalhoHabilidade* galho(const std::string& id) const;
    const NoHabilidade* no(const std::string& id) const;
    const std::set<std::string>& desbloqueados() const { return desbloqueados_; }
    void restaurar(const std::vector<std::string>& ids);

private:
    std::vector<NoHabilidade> nos_;
    std::vector<GalhoHabilidade> galhos_;
    std::set<std::string> desbloqueados_;
};

const char* textoResultado(ResultadoDesbloqueio r);

} // namespace andv

#pragma once
// Inimigo — cultistas (magia à distância) e carniçais (perseguição e bote).
//
// IA como máquina de estados enxuta:
//   Patrulha ──vê o jogador──► Perseguir ──no alcance──► Preparar (telegrafado)
//      ▲                          │                           │
//      └──perdeu de vista (3 s)───┘                           ▼
//                                 Recuperar ◄──────────── Atacar
//   Qualquer estado ──dano forte──► Atordoado;  vida 0 ──► Morrendo
#include "andvarion/entidades/Entidade.hpp"

#include <string>
#include <vector>

namespace andv {

struct DefinicaoInimigo {
    std::string classe;              // "cultista", "carnical"
    std::string nome;                // "Cultista da Catedral"
    std::string sprite;              // pasta em assets/
    float vida = 40.f;
    float velocidade = 40.f;
    float velocidadeAtaque = 0.f;    // bote do carniçal
    float raioVisao = 120.f;
    float alcance = 18.f;            // distância para começar a atacar
    float distanciaPreferida = 0.f;  // cultista mantém distância
    float dano = 10.f;
    float preparo = 0.5f;            // telegrafia (segundos)
    float duracaoAtaque = 0.3f;
    float recarga = 1.2f;
    bool  distancia = false;
    i32   xp = 15;
    std::string dropItem;
    float dropChance = 0.f;
};

const DefinicaoInimigo* definicaoInimigo(const std::string& classe);

class Inimigo final : public Combatente {
public:
    Inimigo(const DefinicaoInimigo& def, Vector2 pos, std::vector<Vector2> patrulha);

    void atualizar(Mundo& m, float dt) override;
    void desenhar(const Mundo& m) const override;
    bool receberDano(Mundo& m, const Dano& d) override;

    const DefinicaoInimigo& def() const { return def_; }

private:
    enum class Estado { Patrulha, Perseguir, Preparar, Atacar, Recuperar, Atordoado, Morrendo };
    void trocar(Estado e);
    void andarPara(Mundo& m, Vector2 alvo, float velocidade, float dt);
    void executarAtaque(Mundo& m);
    void morrer(Mundo& m);

    const DefinicaoInimigo& def_;
    std::vector<Vector2> patrulha_;
    std::size_t pontoPatrulha_ = 0;
    Estado estado_ = Estado::Patrulha;
    float tempoEstado_ = 0.f;
    float semVer_ = 0.f;
    float esperaPatrulha_ = 0.f;
    Vector2 alvoAtaque_{};
    Vector2 dirBote_{};
    bool acertouBote_ = false;
    float barraVisivel_ = 0.f;
};

} // namespace andv

#pragma once
// Entidade — base de tudo que vive no mundo (NPC, inimigo, projétil, item...).
// Combatente — entidade com vida, empurrão, flash de dano e animação.
#include "andvarion/core/Plataforma.hpp"
#include "andvarion/gfx/AnimationController.hpp"

#include <raylib.h>

namespace andv {

class Mundo;

enum class Faccao : u8 { Jogador, Inimigo, Neutra };

struct Dano {
    float valor = 0.f;
    Vector2 origem{};          // de onde veio (direção do empurrão)
    float empurrao = 0.f;      // px/s
    Faccao fonte = Faccao::Neutra;
    bool lamina = false;       // golpes de Lâmina drenam essência (mana)
    bool critico = false;
    float atordoar = 0.f;      // segundos sem agir
    float lentidao = 0.f;      // segundos lento
};

class Entidade {
public:
    virtual ~Entidade() = default;
    virtual void atualizar(Mundo& mundo, float dt) = 0;
    virtual void desenhar(const Mundo& mundo) const = 0;
    // Y dos pés para o Y-sort.
    virtual float ySort() const { return pos.y; }

    Vector2 pos{};
    float raio = 5.f;
    Faccao faccao = Faccao::Neutra;
    bool remover = false;
};

class Combatente : public Entidade {
public:
    bool vivo() const { return vida > 0.f; }
    // Aplica dano (respeitando i-frames). Devolve true se acertou.
    virtual bool receberDano(Mundo& mundo, const Dano& d);

    float vida = 100.f, vidaMax = 100.f;
    float flashDano = 0.f;         // segundos de tinta branca/vermelha
    float invulneravel = 0.f;      // i-frames
    float atordoado = 0.f;
    float lentidao = 0.f;          // Lança de Gelo: segundos andando a 45% da velocidade
    Vector2 empurrao{};            // velocidade de recuo, decai rápido
    Direcao direcao = Direcao::Baixo;
    AnimationController anim;

protected:
    void atualizarComum(Mundo& mundo, float dt);   // timers + empurrão com colisão
    void desenharSombra(float largura = 10.f) const;
    Color tinta() const;                           // pisca ao levar dano
};

} // namespace andv

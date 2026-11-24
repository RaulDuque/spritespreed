#include "andvarion/entidades/Entidade.hpp"
#include "andvarion/cenas/Mundo.hpp"
#include "andvarion/mundo/MapaTiles.hpp"

#include <raymath.h>

#include <algorithm>
#include <cmath>

namespace andv {

bool Combatente::receberDano(Mundo& mundo, const Dano& d) {
    if (!vivo() || invulneravel > 0.f) return false;
    vida = std::max(0.f, vida - d.valor);
    flashDano = 0.12f;
    if (d.empurrao > 0.f) {
        Vector2 dir = Vector2Subtract(pos, d.origem);
        if (Vector2Length(dir) < 0.01f) dir = {0, 1};
        empurrao = Vector2Scale(Vector2Normalize(dir), d.empurrao);
    }
    atordoado = std::max(atordoado, d.atordoar);
    lentidao = std::max(lentidao, d.lentidao);
    (void)mundo;
    return true;
}

void Combatente::atualizarComum(Mundo& mundo, float dt) {
    flashDano = std::max(0.f, flashDano - dt);
    invulneravel = std::max(0.f, invulneravel - dt);
    atordoado = std::max(0.f, atordoado - dt);
    lentidao = std::max(0.f, lentidao - dt);
    if (Vector2Length(empurrao) > 1.f) {
        pos = mundo.mapa().moverCirculo(pos, raio, Vector2Scale(empurrao, dt));
        empurrao = Vector2Scale(empurrao, std::exp(-10.f * dt));
    } else {
        empurrao = {0, 0};
    }
}

void Combatente::desenharSombra(float largura) const {
    DrawEllipse(static_cast<int>(std::round(pos.x)), static_cast<int>(std::round(pos.y)), largura * 0.5f, largura * 0.22f,
                Color{0, 0, 0, 90});
}

Color Combatente::tinta() const {
    if (flashDano > 0.f) return Color{255, 120, 120, 255};
    if (lentidao > 0.f) return Color{150, 200, 255, 255};   // congelando
    if (invulneravel > 0.f && std::fmod(invulneravel, 0.1f) < 0.05f) return Color{180, 170, 255, 170};
    return WHITE;
}

} // namespace andv

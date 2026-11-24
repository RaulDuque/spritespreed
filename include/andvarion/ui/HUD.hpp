#pragma once
// HUD — barras (Vida/Mana/Vigor/XP), feitiços com recarga, armas, frascos,
// indicador de Sede de Sangue e o rastreador de missões (canto superior direito).
#include <raylib.h>

namespace andv {

class Fonte;
class Jogador;
class VFXManager;
struct Sessao;

class HUD {
public:
    void atualizar(const Jogador& j, float dt);
    void desenhar(const Fonte& fonte, const Jogador& j, const Sessao& s, const VFXManager& vfx) const;

private:
    void desenharBarras(const Fonte& fonte, const Jogador& j, const Sessao& s) const;
    void desenharFeiticos(const Fonte& fonte, const Jogador& j, const Sessao& s, const VFXManager& vfx) const;
    void desenharRastreador(const Fonte& fonte, const Sessao& s) const;

    float vidaFantasma_ = 1.f, manaFantasma_ = 1.f, vigorFantasma_ = 1.f;
    float tempo_ = 0.f;
};

} // namespace andv

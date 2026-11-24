#pragma once
// Projetil — flecha, virote, bumerangue, Orbe de Sangue e o raio dos cultistas.
// O visual é o loop do pixelfx (VFXManager::desenharQuadro), girado na direção do voo.
#include "andvarion/combate/Armas.hpp"
#include "andvarion/entidades/Entidade.hpp"

#include <string>
#include <vector>

namespace andv {

class Projetil final : public Entidade {
public:
    Projetil(TipoProjetil tipo, Vector2 pos, Vector2 dir, float velocidade, float dano, float alcance, Faccao faccao);

    void atualizar(Mundo& m, float dt) override;
    void desenhar(const Mundo& m) const override;
    float ySort() const override { return pos.y + 6.f; }  // voa "acima" do chão

    bool perfurante = false;           // disparo carregado atravessa inimigos
    float empurrao = 50.f;
    std::string vfx;                   // loop do pixelfx
    float escalaVfx = 0.25f;
    Color tinta = WHITE;
    float raioExplosao = 0.f;          // > 0: explode ao tocar (Orbe de Sangue, Bola de Fogo)
    std::string vfxExplosao;
    float escalaExplosao = 0.45f;
    std::string vfxImpacto = "impacto_sangue";
    float lentidao = 0.f;              // Lança de Gelo
    std::string proficiencia;          // arma/escola que ganha XP ao acertar
    Color corLuz{0, 0, 0, 0};          // a != 0 => carrega uma luz

private:
    void explodir(Mundo& m);
    void destruir(Mundo& m);

    TipoProjetil tipo_;
    Vector2 dir_;
    float velocidade_, dano_, alcance_;
    float percorrido_ = 0.f, tempo_ = 0.f, giro_ = 0.f;
    bool voltando_ = false;
    std::vector<const Entidade*> atingidos_;
    unsigned luz_ = 0;
};

} // namespace andv

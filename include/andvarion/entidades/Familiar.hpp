#pragma once
// Familiar — espírito invocado (Invocação de Familiar). Orbita o jogador e
// dispara no inimigo visível mais próximo; some quando o tempo acaba.
#include "andvarion/entidades/Entidade.hpp"

namespace andv {

class Familiar final : public Entidade {
public:
    Familiar(float duracao, float faseOrbita, float danoTiro);
    void atualizar(Mundo& m, float dt) override;
    void desenhar(const Mundo& m) const override;
    float ySort() const override { return pos.y + 12.f; }   // flutua: desenha por cima de quem está perto

private:
    float restante_, fase_, dano_;
    float tempo_ = 0.f, proximoTiro_ = 0.6f;
    unsigned luz_ = 0;
};

} // namespace andv

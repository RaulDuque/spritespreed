#pragma once
// AreaDano — zona persistente (Névoa Cortante, Correntes Sombrias): fere em ticks.
#include "andvarion/entidades/Entidade.hpp"

#include <string>

namespace andv {

class AreaDano final : public Entidade {
public:
    AreaDano(Vector2 centro, float raio, float duracao, float intervalo, float danoPorTick, Faccao faccao);

    void atualizar(Mundo& m, float dt) override;
    void desenhar(const Mundo& m) const override;
    float ySort() const override { return pos.y - 1000.f; }  // sempre no chão

    std::string vfx;
    float escalaVfx = 0.3f;
    float atordoar = 0.f;            // Correntes Sombrias prendem
    Color corLuz{0, 0, 0, 0};

private:
    float raioArea_, duracao_, intervalo_, dano_;
    float tempo_ = 0.f, proximoTick_ = 0.f;
    unsigned vfxId_ = 0, luz_ = 0;
};

} // namespace andv

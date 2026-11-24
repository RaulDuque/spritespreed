#pragma once
// Coletavel — item no chão (fragmentos, ervas, selo). Encostar = coletar.
#include "andvarion/entidades/Entidade.hpp"

#include <string>

namespace andv {

class Coletavel final : public Entidade {
public:
    Coletavel(std::string itemId, Vector2 pos, std::string uid, std::string vfx);
    void atualizar(Mundo& m, float dt) override;
    void desenhar(const Mundo& m) const override;

    const std::string& item() const { return item_; }

private:
    std::string item_, uid_, vfx_;
    float tempo_ = 0.f;
    float atrasoColeta_ = 0.35f;     // itens soltos por inimigos "pulam" antes
    unsigned luz_ = 0;
};

} // namespace andv

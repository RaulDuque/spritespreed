#pragma once
// NPC — personagem com diálogo (Ferreiro, Curandeira, Vigia...).
// Acima da cabeça: "!" = tem missão para oferecer, "?" dourado = pronta para
// entregar, "?" cinza = missão em andamento.
#include "andvarion/entidades/Entidade.hpp"

#include <string>

namespace andv {

class NPC final : public Combatente {
public:
    NPC(std::string id, Vector2 pos, Direcao dir);
    void atualizar(Mundo& m, float dt) override;
    void desenhar(const Mundo& m) const override;

    const std::string& id() const { return id_; }
    void virarPara(Vector2 alvo);
    bool destacado = false;          // jogador perto: mostra "[E] Conversar"

private:
    std::string id_;
    float tempo_ = 0.f;
};

} // namespace andv

#pragma once
// CameraJogo — segue o jogador com suavização, respeita os limites do mapa e
// aplica tremor de tela ("trauma": cresce com impactos, decai sozinho).
#include "andvarion/core/Plataforma.hpp"

#include <raylib.h>

namespace andv {

class CameraJogo {
public:
    void configurar(float larguraTela, float alturaTela, float zoom);
    void definirLimites(Rectangle mundo) { limites_ = mundo; temLimites_ = true; }
    void focarImediato(Vector2 alvo);

    void seguir(Vector2 alvo, Vector2 olhar, float dt);  // `olhar` adianta a câmera na direção da mira
    void tremer(float trauma);                            // 0..1, acumula
    void atualizar(float dt);

    const Camera2D& camera() const { return cam_; }
    Vector2 telaParaMundo(Vector2 tela) const { return GetScreenToWorld2D(tela, cam_); }
    Rectangle visivel() const;  // retângulo do mundo na tela (para culling)

private:
    void prender();
    Camera2D cam_{};
    Vector2 foco_{};
    Rectangle limites_{};
    bool temLimites_ = false;
    float trauma_ = 0.f;
    float tempo_ = 0.f;
    float largura_ = 640.f, altura_ = 360.f, zoom_ = 1.f;  // largura/altura em px de tela
};

} // namespace andv

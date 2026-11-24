#include "andvarion/gfx/CameraJogo.hpp"

#include <raymath.h>

#include <algorithm>
#include <cmath>

namespace andv {

void CameraJogo::configurar(float largura, float altura, float zoom) {
    largura_ = largura;
    altura_ = altura;
    zoom_ = zoom;
    cam_.offset = {largura * 0.5f, altura * 0.5f};
    cam_.zoom = zoom;
    cam_.rotation = 0.f;
}

void CameraJogo::focarImediato(Vector2 alvo) {
    foco_ = alvo;
    prender();
    cam_.target = foco_;
}

void CameraJogo::seguir(Vector2 alvo, Vector2 olhar, float dt) {
    const Vector2 desejado = Vector2Add(alvo, Vector2Scale(olhar, 18.f));
    // Suavização exponencial independente de FPS.
    const float k = 1.f - std::exp(-8.f * dt);
    foco_ = Vector2Lerp(foco_, desejado, k);
    prender();
}

void CameraJogo::prender() {
    if (!temLimites_) return;
    // Meia tela em unidades de mundo.
    const float mx = largura_ * 0.5f / zoom_, my = altura_ * 0.5f / zoom_;
    if (limites_.width <= 2.f * mx) foco_.x = limites_.x + limites_.width * 0.5f;
    else foco_.x = std::clamp(foco_.x, limites_.x + mx, limites_.x + limites_.width - mx);
    if (limites_.height <= 2.f * my) foco_.y = limites_.y + limites_.height * 0.5f;
    else foco_.y = std::clamp(foco_.y, limites_.y + my, limites_.y + limites_.height - my);
}

void CameraJogo::tremer(float trauma) { trauma_ = std::min(1.f, trauma_ + trauma); }

void CameraJogo::atualizar(float dt) {
    tempo_ += dt;
    trauma_ = std::max(0.f, trauma_ - dt * 1.6f);
    const float s = trauma_ * trauma_;  // quadrático: impactos pequenos quase não tremem
    const Vector2 desvio{s * 6.f * std::sin(tempo_ * 53.f), s * 6.f * std::sin(tempo_ * 41.f + 1.3f)};
    // Arredonda o alvo para o pixel de tela: o mundo não "nada" entre pixels.
    cam_.target = {std::round((foco_.x + desvio.x) * zoom_) / zoom_, std::round((foco_.y + desvio.y) * zoom_) / zoom_};
}

Rectangle CameraJogo::visivel() const {
    const float w = largura_ / zoom_, h = altura_ / zoom_;
    return {cam_.target.x - w * 0.5f, cam_.target.y - h * 0.5f, w, h};
}

} // namespace andv

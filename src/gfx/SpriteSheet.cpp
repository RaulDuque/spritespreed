#include "andvarion/gfx/SpriteSheet.hpp"
#include "andvarion/core/Assets.hpp"

namespace andv {

SpriteSheet::SpriteSheet(AtlasAseprite atlas, std::shared_ptr<Textura> textura)
    : atlas_(std::move(atlas)), textura_(std::move(textura)) {}

const Texture2D& SpriteSheet::textura() const { return textura_->get(); }
bool SpriteSheet::valida() const { return textura_ && textura_->valida(); }

Rectangle SpriteSheet::retangulo(i32 i) const {
    const auto& qs = atlas_.quadros();
    if (qs.empty()) return {0, 0, 0, 0};
    if (i < 0 || i >= static_cast<i32>(qs.size())) i = 0;
    const auto& q = qs[static_cast<std::size_t>(i)];
    return {static_cast<float>(q.x), static_cast<float>(q.y), static_cast<float>(q.w), static_cast<float>(q.h)};
}

void SpriteSheet::desenhar(i32 indice, Vector2 pos, float escala, bool espelhar, float rotacao, Color tinta) const {
    if (!valida()) return;
    Rectangle fonte = retangulo(indice);
    float ancoraX = atlas_.ancoraX();
    const float ancoraY = atlas_.ancoraY();
    if (espelhar) {
        fonte.width = -fonte.width;            // raylib espelha com largura negativa
        ancoraX = -fonte.width - ancoraX;      // reflete o pivô: w - ax
    }
    const Rectangle destino{pos.x, pos.y, fonte.width < 0 ? -fonte.width * escala : fonte.width * escala,
                            fonte.height * escala};
    DrawTexturePro(textura(), fonte, destino, {ancoraX * escala, ancoraY * escala}, rotacao, tinta);
}

ConjuntoSprites::Achado ConjuntoSprites::buscar(const std::string& nome) const {
    for (const auto& p : paginas_)
        if (const ClipeAtlas* c = p->atlas().clipe(nome)) return {p.get(), c};
    return {};
}

} // namespace andv

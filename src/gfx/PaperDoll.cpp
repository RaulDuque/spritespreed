#include "andvarion/gfx/PaperDoll.hpp"

#include <algorithm>

namespace andv {

void PaperDoll::definir(CamadaVisual c, std::shared_ptr<const ConjuntoSprites> sprites) {
    if (sprites && sprites->vazio()) sprites = nullptr;
    camadas_[static_cast<std::size_t>(c)] = std::move(sprites);
}

std::array<CamadaVisual, 5> PaperDoll::ordem(Direcao d) {
    using C = CamadaVisual;
    switch (d) {
        case Direcao::Cima: case Direcao::CimaDireita: case Direcao::CimaEsquerda:
            return {C::Arma, C::Escudo, C::Corpo, C::Armadura, C::Capa};      // de costas
        case Direcao::Direita: case Direcao::Esquerda:
            return {C::Capa, C::Escudo, C::Corpo, C::Armadura, C::Arma};      // perfil: escudo no braço de trás
        default:
            return {C::Capa, C::Corpo, C::Armadura, C::Escudo, C::Arma};      // de frente
    }
}

std::vector<QuadroCongelado> PaperDoll::quadros(const AnimationController& corpo) const {
    std::vector<QuadroCongelado> r;
    const EstadoQuadro e = corpo.estado();
    if (!e.clipe) return r;
    for (CamadaVisual c : ordem(corpo.direcao())) {
        if (c == CamadaVisual::Corpo) {
            r.push_back(corpo.quadroAtual());
            continue;
        }
        const auto& conjunto = camadas_[static_cast<std::size_t>(c)];
        if (!conjunto) continue;
        const auto achado = conjunto->buscar(e.clipe->nome);
        if (!achado.clipe || achado.clipe->quadros.empty()) continue;  // camada sem esse clipe: omite
        const auto pos = static_cast<std::size_t>(std::min<i32>(e.posicao, static_cast<i32>(achado.clipe->quadros.size()) - 1));
        r.push_back({achado.sheet, achado.clipe->quadros[pos], e.espelhar});
    }
    return r;
}

void PaperDoll::desenhar(const AnimationController& corpo, Vector2 posPes, Color tinta, float escala) const {
    const Vector2 p{snapTela(posPes.x), snapTela(posPes.y)};
    for (const QuadroCongelado& q : quadros(corpo))
        if (q.sheet) q.sheet->desenhar(q.quadro, p, escala, q.espelhar, 0.f, tinta);
}

} // namespace andv

#pragma once
// PaperDoll — personagem montado em camadas sincronizadas (troca visual de equipamento).
//
//   Capa · Corpo base · Armadura/Túnica · Escudo · Arma
//
// Contrato com o agent-sprite-forge: TODAS as camadas são atlas com o mesmo
// tamanho de quadro, a mesma âncora (pés) e os mesmos nomes de clipe com a
// mesma contagem de quadros ("andar_direita" tem 6 quadros no corpo, na capa,
// na espada...). O AnimationController anima só o CORPO; cada camada desenha o
// quadro de mesmo índice do clipe de mesmo nome no próprio atlas. Equipar um
// item troca o atlas da camada e a animação segue sem pular quadro.
//
// Ordem de pintura muda com a direção: de costas (cima) a capa cobre o corpo e
// a arma fica atrás; de lado o escudo (braço de trás) fica atrás do corpo.
#include "andvarion/gfx/AnimationController.hpp"

#include <array>
#include <memory>
#include <vector>

namespace andv {

enum class CamadaVisual : u8 { Capa, Corpo, Armadura, Escudo, Arma, Total };

class PaperDoll {
public:
    // nullptr remove a camada. O corpo vem do AnimationController, não daqui.
    void definir(CamadaVisual c, std::shared_ptr<const ConjuntoSprites> sprites);
    bool tem(CamadaVisual c) const { return camadas_[static_cast<std::size_t>(c)] != nullptr; }

    void desenhar(const AnimationController& corpo, Vector2 posPes, Color tinta = WHITE,
                  float escala = ESCALA_DESENHO) const;
    // Todas as camadas do quadro atual, já na ordem de pintura (afterimages).
    std::vector<QuadroCongelado> quadros(const AnimationController& corpo) const;

    static std::array<CamadaVisual, 5> ordem(Direcao d);

private:
    std::array<std::shared_ptr<const ConjuntoSprites>, static_cast<std::size_t>(CamadaVisual::Total)> camadas_{};
};

} // namespace andv

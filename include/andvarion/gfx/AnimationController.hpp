#pragma once
// AnimationController — toca os clipes exportados pelo agent-sprite-forge.
//
// Convenção de nomes de clipe: "<acao>_<direcao>", em 8 ou 4 direções:
//     ocioso_baixo, andar_direita, ataque1_cima_esquerda, disparo_baixo_direita ...
// Também aceita os nomes em inglês que a forge costuma usar (idle_down,
// walk_right, attack1_up_left, ...).
//
// Resolução de um pedido (acao, direcao), na ordem:
//   1. o clipe exato em 8 direções
//   2. a direção cardinal mais próxima (4 direções)
//   3. o lado espelhado (esquerda <- direita), refletido em torno do x da âncora
//   4. o clipe sem direção ("morte")
//   5. "ocioso" naquela direção (nunca deixa o personagem invisível)
//
// Seguindo o runtime da forge:
//   * caminhada avança pela DISTÂNCIA percorrida (avancarDistancia), não pelo
//     relógio: bloqueado na parede, o pé não desliza;
//   * eventos ("hit", "step_l", "release") disparam uma vez por cruzamento;
//   * hit-stop congela só o relógio da pose (congelar).
#include "andvarion/core/Escala.hpp"
#include "andvarion/gfx/SpriteSheet.hpp"

#include <raylib.h>

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace andv {

enum class Direcao : u8 { Baixo, BaixoDireita, Direita, CimaDireita, Cima, CimaEsquerda, Esquerda, BaixoEsquerda };

Direcao direcaoDeVetor(Vector2 v, bool oitoDirecoes = true);
Vector2 vetorDeDirecao(Direcao d);
const char* nomeDirecao(Direcao d);

// Posição atual na linha do tempo: as camadas do paper-doll leem isto para
// mostrar o MESMO quadro do mesmo clipe nos atlas delas.
struct EstadoQuadro {
    const ClipeAtlas* clipe = nullptr;
    i32 posicao = 0;
    bool espelhar = false;
};

// Tudo o que é preciso para redesenhar um quadro depois (afterimages da esquiva).
struct QuadroCongelado {
    const SpriteSheet* sheet = nullptr;
    i32 quadro = 0;
    bool espelhar = false;
};

class AnimationController {
public:
    void definirConjunto(std::shared_ptr<const ConjuntoSprites> conjunto);
    bool temSprites() const { return conjunto_ && !conjunto_->vazio(); }

    // Troca de clipe. Mudar só a direção de um clipe em loop preserva a fase
    // (o passo não reinicia ao virar). `reiniciar` força começar do quadro 0.
    void tocar(const std::string& acao, Direcao dir, bool reiniciar = false);

    void atualizar(float dtSeg, float velocidade = 1.f);
    // Para locomoção: `passadaPx` = distância de um ciclo completo.
    void avancarDistancia(float distanciaPx, float passadaPx);
    void congelar(float segundos) { congelado_ = std::max(congelado_, segundos); }

    bool terminou() const;              // one-shot chegou ao fim
    float progresso() const;            // 0..1 dentro do clipe
    float duracaoSeg() const { return atual_.clipe ? atual_.clipe->duracaoTotalMs / 1000.f : 0.f; }
    const std::vector<std::string>& eventos() const { return eventos_; }  // cruzados nesta atualização
    bool disparou(const std::string& evento) const;

    const std::string& acao() const { return acao_; }
    Direcao direcao() const { return direcao_; }

    // `escala` padrão: arte em resolução dobrada pintada no mundo (ver Escala.hpp).
    void desenhar(Vector2 posPes, Color tinta = WHITE, float escala = ESCALA_DESENHO) const;
    QuadroCongelado quadroAtual() const;
    EstadoQuadro estado() const;

private:
    struct Resolucao {
        const SpriteSheet* sheet = nullptr;
        const ClipeAtlas* clipe = nullptr;
        bool espelhar = false;
    };
    Resolucao resolver(const std::string& acao, Direcao dir);
    void coletarEventos(float antesMs, float depoisMs);

    std::shared_ptr<const ConjuntoSprites> conjunto_;
    std::unordered_map<std::string, Resolucao> cache_;
    Resolucao atual_;
    std::string acao_;
    Direcao direcao_ = Direcao::Baixo;
    float tempoMs_ = 0.f;
    float congelado_ = 0.f;
    std::vector<std::string> eventos_;
};

} // namespace andv

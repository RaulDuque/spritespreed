#pragma once
// Roteiro — entrada roteirizada para testes de fumaça e capturas automáticas.
//
//   ANDV_ROTEIRO=tools/roteiros/fumaca.txt ./andvarion
//
// Cada linha: <segundos> <comando> [argumentos]   (# inicia comentário)
//   tecla <nome>            aperta por 1 quadro: enter e espaco esc k f r tab q 1 2 3 w s
//   mover <dx> <dy> <dur>   segura a direção por <dur> segundos
//   mira <x> <y>            cursor na tela virtual (480x270)
//   clique                  botão esquerdo (Lâmina / confirmar)
//   direito <dur>           segura o botão direito por <dur> s (mira/carga) e solta
//   captura <arquivo.png>   salva a tela (resolução da janela)
//   sair                    fecha o jogo (código 0)
#include "andvarion/core/Entrada.hpp"

#include <string>
#include <vector>

namespace andv {

class Roteiro {
public:
    bool carregar(const std::string& caminho);
    bool ativo() const { return ativo_; }

    // Avança o relógio do roteiro (uma vez por quadro de render).
    void avancar(float dt);
    // Sobrepõe os comandos deste quadro.
    void aplicar(ComandosJogador& c) const;
    bool temMouse() const { return temMouse_; }
    Vector2 mouse() const { return mouse_; }

    // Capturas e saída pendentes neste quadro (consumidas por Jogo).
    std::vector<std::string> capturas;
    bool pediuSaida = false;

private:
    struct Passo {
        float t;
        std::string cmd;
        std::vector<std::string> args;
    };
    std::vector<Passo> passos_;
    std::size_t proximo_ = 0;
    float tempo_ = 0.f;
    bool ativo_ = false;

    // Estado deste quadro
    std::vector<std::string> teclas_;
    bool clique_ = false, direitoInicio_ = false, direitoFim_ = false;
    float direitoAte_ = -1.f;
    Vector2 mover_{};
    float moverAte_ = -1.f;
    Vector2 mouse_{240, 135};
    bool temMouse_ = false;
};

// Roteiro global (nulo quando ANDV_ROTEIRO não está definido).
Roteiro* roteiroAtivo();
void definirRoteiroAtivo(Roteiro* r);

} // namespace andv

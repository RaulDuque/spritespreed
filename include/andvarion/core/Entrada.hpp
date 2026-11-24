#pragma once
// Entrada — traduz teclado/mouse em comandos de jogo (desacopla raylib da lógica).
//
//  Botão esquerdo ........ Lâmina (combo de 3)
//  Botão direito ......... Arma à distância (segurar = disparo carregado)
//  1, 2, 3, Q ............ Magia gótica
//  Espaço / Shift ........ Esquiva Sombria (dash com i-frames)
//  WASD / setas .......... Movimento        E ........ Interagir / conversar
//  F ..................... Frasco de cura    R ........ Trocar arma à distância
//  Roda do mouse / Tab ... Trocar arma corpo a corpo
//  K ..................... Árvore de Habilidades      I ........ Inventário
//  Esc ................... Pausa
#include "andvarion/core/Plataforma.hpp"

#include <raylib.h>

namespace andv {

struct ComandosJogador {
    Vector2 mover{};                 // normalizado (ou zero)
    Vector2 miraMundo{};             // cursor em coordenadas de mundo
    Vector2 mouseVirtual{};          // cursor na tela virtual (UI)
    bool laminaPressionado = false;
    bool distanciaPressionado = false;
    bool distanciaSegurando = false;
    bool distanciaSoltou = false;
    bool feitico[4] = {false, false, false, false};
    bool esquiva = false;
    bool interagir = false;
    bool frasco = false;
    i32  trocarCorpo = 0;            // -1, 0, +1
    bool trocarDistancia = false;
    bool arvore = false;
    bool inventario = false;
    bool pausar = false;
    // Navegação de menus/diálogo
    bool cima = false, baixo = false, esquerda = false, direita = false;
    bool confirmar = false, cancelar = false, clique = false;

    // Junta os "pressionados" de vários quadros de render sem update no meio
    // (o update roda a 60 Hz fixos; nenhum clique pode se perder).
    void acumular(const ComandosJogador& novo);
    // Depois do primeiro passo fixo, os eventos de borda são consumidos.
    void consumirBordas();
};

ComandosJogador lerComandos(Vector2 mouseVirtual, const Camera2D& camera);

} // namespace andv

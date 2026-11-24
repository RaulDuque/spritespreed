#include "andvarion/core/Entrada.hpp"
#include "andvarion/core/Roteiro.hpp"

#include <raymath.h>

namespace andv {

void ComandosJogador::acumular(const ComandosJogador& n) {
    mover = n.mover;
    miraMundo = n.miraMundo;
    mouseVirtual = n.mouseVirtual;
    distanciaSegurando = n.distanciaSegurando;
    laminaPressionado |= n.laminaPressionado;
    distanciaPressionado |= n.distanciaPressionado;
    distanciaSoltou |= n.distanciaSoltou;
    for (int i = 0; i < 4; ++i) feitico[i] |= n.feitico[i];
    esquiva |= n.esquiva;
    interagir |= n.interagir;
    frasco |= n.frasco;
    if (n.trocarCorpo != 0) trocarCorpo = n.trocarCorpo;
    trocarDistancia |= n.trocarDistancia;
    arvore |= n.arvore;
    inventario |= n.inventario;
    pausar |= n.pausar;
    cima |= n.cima;
    baixo |= n.baixo;
    esquerda |= n.esquerda;
    direita |= n.direita;
    confirmar |= n.confirmar;
    cancelar |= n.cancelar;
    clique |= n.clique;
}

void ComandosJogador::consumirBordas() {
    const Vector2 m = mover, mira = miraMundo, mv = mouseVirtual;
    const bool segurando = distanciaSegurando;
    *this = ComandosJogador{};
    mover = m;
    miraMundo = mira;
    mouseVirtual = mv;
    distanciaSegurando = segurando;
}

ComandosJogador lerComandos(Vector2 mouseVirtual, const Camera2D& camera) {
    ComandosJogador c;
    Vector2 m{0, 0};
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) m.y -= 1;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) m.y += 1;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) m.x -= 1;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) m.x += 1;
    c.mover = Vector2Length(m) > 0 ? Vector2Normalize(m) : m;

    c.mouseVirtual = mouseVirtual;
    c.miraMundo = GetScreenToWorld2D(mouseVirtual, camera);

    c.laminaPressionado = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    c.distanciaPressionado = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    c.distanciaSegurando = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
    c.distanciaSoltou = IsMouseButtonReleased(MOUSE_BUTTON_RIGHT);
    c.feitico[0] = IsKeyPressed(KEY_ONE);
    c.feitico[1] = IsKeyPressed(KEY_TWO);
    c.feitico[2] = IsKeyPressed(KEY_THREE);
    c.feitico[3] = IsKeyPressed(KEY_Q);
    c.esquiva = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_RIGHT_SHIFT);
    c.interagir = IsKeyPressed(KEY_E);
    c.frasco = IsKeyPressed(KEY_F);
    const float roda = GetMouseWheelMove();
    c.trocarCorpo = roda > 0 ? 1 : roda < 0 ? -1 : (IsKeyPressed(KEY_TAB) ? 1 : 0);
    c.trocarDistancia = IsKeyPressed(KEY_R);
    c.arvore = IsKeyPressed(KEY_K);
    c.inventario = IsKeyPressed(KEY_I);
    c.pausar = IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P);

    c.cima = IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP);
    c.baixo = IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN);
    c.esquerda = IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT);
    c.direita = IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT);
    c.confirmar = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_E) || IsKeyPressed(KEY_SPACE);
    c.cancelar = IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE);
    c.clique = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    if (const Roteiro* r = roteiroAtivo()) {
        r->aplicar(c);
        c.miraMundo = GetScreenToWorld2D(c.mouseVirtual, camera);
    }
    return c;
}

} // namespace andv

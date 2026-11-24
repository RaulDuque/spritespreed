#include "andvarion/entidades/NPC.hpp"
#include "andvarion/cenas/Mundo.hpp"
#include "andvarion/cenas/Sessao.hpp"

#include <raymath.h>

#include <cmath>

namespace andv {

NPC::NPC(std::string id, Vector2 p, Direcao dir) : id_(std::move(id)) {
    pos = p;
    raio = 6.f;
    direcao = dir;
    faccao = Faccao::Neutra;
    vida = vidaMax = 1.f;
}

void NPC::virarPara(Vector2 alvo) { direcao = direcaoDeVetor(Vector2Subtract(alvo, pos), false); }

void NPC::atualizar(Mundo&, float dt) {
    tempo_ += dt;
    anim.tocar("ocioso", direcao);
    anim.atualizar(dt);
}

void NPC::desenhar(const Mundo& m) const {
    desenharSombra(11.f);
    if (anim.temSprites()) {
        anim.desenhar(pos);
    } else {
        DrawRectangle(static_cast<int>(pos.x) - 5, static_cast<int>(pos.y) - 20, 10, 20, Color{120, 100, 90, 255});
    }

    // Marcador de missão.
    const QuestManager& q = *m.sessao().missoes;
    char marca = 0;
    Color cor = Color{222, 186, 112, 255};
    for (const auto& mi : q.todas()) {
        if (mi.entregarPara == id_ && mi.estado == EstadoMissao::ProntaParaEntregar) { marca = '?'; break; }
        if (mi.doador == id_ && q.podeAceitar(mi.id)) marca = '!';
        else if (!marca && (mi.doador == id_ || mi.entregarPara == id_) && mi.estado == EstadoMissao::Ativa) {
            marca = '?';
            cor = Color{150, 140, 130, 255};
        }
    }
    if (marca) {
        const float bob = std::sin(tempo_ * 4.f) * 1.5f;
        const char txt[2] = {marca, 0};
        // Em coordenadas de mundo com zoom 2: fonte 10 vira 20 px na tela (ícone grande de propósito).
        DrawText(txt, static_cast<int>(pos.x) - 2 + 1, static_cast<int>(pos.y - 42 + bob) + 1, 10, BLACK);
        DrawText(txt, static_cast<int>(pos.x) - 2, static_cast<int>(pos.y - 42 + bob), 10, cor);
    }
}

} // namespace andv

#include "andvarion/entidades/Coletavel.hpp"
#include "andvarion/cenas/Mundo.hpp"
#include "andvarion/cenas/Sessao.hpp"
#include "andvarion/core/EventBus.hpp"
#include "andvarion/entidades/Jogador.hpp"
#include "andvarion/gfx/Iluminacao.hpp"
#include "andvarion/gfx/VFXManager.hpp"

#include <raymath.h>

#include <cmath>

namespace andv {

Coletavel::Coletavel(std::string itemId, Vector2 p, std::string uid, std::string vfx)
    : item_(std::move(itemId)), uid_(std::move(uid)), vfx_(std::move(vfx)) {
    pos = p;
    raio = 6.f;
}

void Coletavel::atualizar(Mundo& m, float dt) {
    tempo_ += dt;
    atrasoColeta_ -= dt;
    if (!luz_) {
        Luz l;
        l.pos = pos;
        l.raio = 26.f;
        l.cor = {200, 210, 255, 255};
        l.intensidade = 0.35f;
        l.brilho = 0.2f;
        l.oscilacao = 0.2f;
        l.fase = pos.x;
        luz_ = m.luz().adicionar(l);
    }
    if (atrasoColeta_ > 0.f) return;
    if (Vector2Distance(pos, m.jogador().pos) > 12.f || !m.jogador().vivo()) return;

    Sessao& s = m.sessao();
    s.inventario.adicionar(item_);
    if (!uid_.empty()) s.coletados.insert(uid_);
    ParametrosVfx p;
    p.escala = 0.3f;
    m.vfx().tocar("coleta", pos, p);
    m.luz().remover(luz_);
    m.jogador().avisar("+1 " + s.inventario.nome(item_));
    TraceLog(LOG_INFO, "ANDVARION: coletado %s", item_.c_str());
    m.bus().publicar(TipoEvento::ItemColetado, item_, 1);
    remover = true;
}

void Coletavel::desenhar(const Mundo& m) const {
    const float bob = std::sin(tempo_ * 3.f) * 1.5f;
    DrawEllipse(static_cast<int>(pos.x), static_cast<int>(pos.y + 3), 4.f, 1.5f, Color{0, 0, 0, 80});
    ParametrosVfx p;
    p.escala = 0.22f;
    if (m.vfx().existe(vfx_)) {
        m.vfx().desenharQuadro(vfx_, tempo_, {pos.x, pos.y + 3 + bob}, p);
    } else {
        DrawPoly({pos.x, pos.y - 3 + bob}, 4, 4.f, tempo_ * 90.f, Color{220, 220, 255, 255});
    }
}

} // namespace andv

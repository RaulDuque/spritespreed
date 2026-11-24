#include "andvarion/cenas/CenaMenuPrincipal.hpp"
#include "andvarion/cenas/CenaJogo.hpp"
#include "andvarion/gfx/VFXManager.hpp"
#include "andvarion/save/SaveSystem.hpp"
#include "andvarion/ui/Fonte.hpp"
#include "andvarion/ui/Pergaminho.hpp"

#include <rlgl.h>

#include <cmath>
#include <cstdio>

namespace andv {

namespace {
const char* OPCOES[3] = {"Novo Jogo", "Continuar", "Sair"};
// A cena de fundo foi composta em 480x270; fica centralizada na tela virtual.
constexpr float LARGURA_DESENHO = 480.f;
constexpr float OX = (LARGURA_VIRTUAL - 480) * 0.5f;
constexpr float OY = (ALTURA_VIRTUAL - 270) * 0.5f;
}

CenaMenuPrincipal::CenaMenuPrincipal(Jogo& jogo) : Cena(jogo) {
    temSave_ = jogo.save().existe();
    selecionada_ = temSave_ ? 1 : 0;
}

Rectangle CenaMenuPrincipal::retanguloOpcao(int i) const {
    return {LARGURA_VIRTUAL * 0.5f - 60.f, OY + 150.f + i * 20.f, 120.f, 16.f};
}

void CenaMenuPrincipal::lerEntrada() {
    Camera2D c{};
    c.zoom = 1.f;
    cmd_.acumular(lerComandos(jogo_.mouseVirtual(), c));
}

void CenaMenuPrincipal::escolher(int i) {
    if (i == 0) {
        jogo_.trocarCena(std::make_unique<CenaJogo>(jogo_, std::nullopt));
    } else if (i == 1) {
        if (!temSave_) {
            mensagem_ = "Nenhum jogo salvo ainda.";
            tempoMensagem_ = 2.f;
            return;
        }
        std::string erro;
        auto dados = jogo_.save().carregar(&erro);
        if (!dados) {
            mensagem_ = "Save corrompido: " + erro;
            tempoMensagem_ = 3.f;
            return;
        }
        jogo_.trocarCena(std::make_unique<CenaJogo>(jogo_, std::move(dados)));
    } else {
        jogo_.sair();
    }
}

void CenaMenuPrincipal::atualizar(float dt) {
    tempo_ += dt;
    tempoMensagem_ -= dt;
    const ComandosJogador c = cmd_;
    cmd_.consumirBordas();

    if (c.cima) selecionada_ = (selecionada_ + 2) % 3;
    if (c.baixo) selecionada_ = (selecionada_ + 1) % 3;
    for (int i = 0; i < 3; ++i) {
        if (CheckCollisionPointRec(c.mouseVirtual, retanguloOpcao(i))) {
            if (c.mouseVirtual.x != mouseAnterior_.x || c.mouseVirtual.y != mouseAnterior_.y) selecionada_ = i;
            if (c.clique) {
                escolher(i);
                return;
            }
        }
    }
    mouseAnterior_ = c.mouseVirtual;
    if (c.confirmar && !c.clique) escolher(selecionada_);
}

void CenaMenuPrincipal::desenhar() {
    // Céu noturno em faixas (gradiente com degraus, estilo pixel art), tela inteira.
    const Color faixas[6] = {{8, 8, 20, 255}, {11, 11, 28, 255}, {15, 14, 36, 255}, {20, 17, 44, 255}, {26, 20, 50, 255}, {33, 22, 52, 255}};
    const int faixa = ALTURA_VIRTUAL / 6 + 1;
    for (int i = 0; i < 6; ++i) DrawRectangle(0, i * faixa, LARGURA_VIRTUAL, faixa, faixas[i]);
    const Color silhueta{6, 5, 10, 255};
    DrawRectangle(0, static_cast<int>(OY) + 210, LARGURA_VIRTUAL, ALTURA_VIRTUAL, silhueta);  // chão

    // Cena composta numa área 480x270, centralizada na tela.
    rlPushMatrix();
    rlTranslatef(OX, OY, 0.f);
    DrawCircle(380, 60, 26, Color{150, 40, 46, 255});   // lua sangrenta
    DrawCircle(386, 56, 22, Color{180, 60, 60, 255});
    DrawCircle(372, 66, 4, Color{140, 36, 42, 255});
    DrawRectangle(60, 130, 70, 90, silhueta);           // catedral
    ui::triangulo({60, 130}, {95, 70}, {130, 130}, silhueta);
    DrawRectangle(91, 50, 8, 24, silhueta);
    DrawRectangle(86, 56, 18, 4, silhueta);
    DrawRectangle(150, 160, 120, 60, silhueta);
    ui::triangulo({150, 160}, {210, 115}, {270, 160}, silhueta);
    DrawRectangle(330, 175, 40, 45, silhueta);
    ui::triangulo({330, 175}, {350, 140}, {370, 175}, silhueta);
    for (int i = 0; i < 3; ++i) DrawRectangle(100 + i * 10, 150, 4, 10, Color{255, 170, 70, static_cast<unsigned char>(120 + 60 * std::sin(tempo_ * 3 + i))});

    // Névoa, tochas e morcego do pixelfx.
    VFXManager& vfx = jogo_.vfx();
    ParametrosVfx nevoa;
    nevoa.escala = 1.5f;
    nevoa.tinta = Color{255, 255, 255, 150};
    vfx.desenharQuadro("nevoa_ambiente", tempo_, {LARGURA_DESENHO * 0.5f, 290.f}, nevoa);
    ParametrosVfx tocha;
    tocha.escala = 0.45f;
    vfx.desenharQuadro("tocha", tempo_, {110.f, 200.f}, tocha);
    vfx.desenharQuadro("tocha", tempo_ + 0.7f, {370.f, 200.f}, tocha);
    ParametrosVfx morcego;
    morcego.escala = 0.3f;
    vfx.desenharQuadro("morcego", tempo_, {300.f + 30.f * std::sin(tempo_ * 0.5f), 40.f + 8.f * std::sin(tempo_)}, morcego);

    const Fonte& f = jogo_.fonte();
    f.desenharCentro("ANDVARION", {LARGURA_DESENHO * 0.5f + 1, 62}, 40.f, Color{60, 10, 14, 255}, false);
    f.desenharCentro("ANDVARION", {LARGURA_DESENHO * 0.5f, 60}, 40.f, ui::OSSO, false);
    f.desenharCentro("O Sussurro sob a Catedral", {LARGURA_DESENHO * 0.5f, 96}, 10.f, ui::SANGUE);
    rlPopMatrix();

    // Opções em coordenadas de tela (as mesmas do teste de clique).
    const Rectangle primeira = retanguloOpcao(0);
    ui::desenharPergaminho({primeira.x - 10.f, primeira.y - 10.f, 140.f, 72.f});
    for (int i = 0; i < 3; ++i) {
        const Rectangle r = retanguloOpcao(i);
        const bool sel = i == selecionada_;
        const bool desabilitada = (i == 1 && !temSave_);
        if (sel) DrawRectangleRec(r, Color{90, 30, 36, 200});
        Color c = desabilitada ? Color{90, 80, 75, 255} : (sel ? ui::DOURADO : ui::OSSO);
        f.desenharCentro(OPCOES[i], {r.x + r.width * 0.5f, r.y + r.height * 0.5f}, 10.f, c);
    }
    if (tempoMensagem_ > 0.f) f.desenharCentro(mensagem_, {LARGURA_VIRTUAL * 0.5f, primeira.y + 72.f}, 10.f, ui::SANGUE);

    char rodape[96];
    std::snprintf(rodape, sizeof rodape, "v0.1  -  %s  -  W/S + Enter ou mouse",
                  sizeof(void*) == 4 ? "x86 32 bits" : "build de verificação 64 bits");
    f.desenhar(rodape, {6, ALTURA_VIRTUAL - 12.f}, 10.f, ui::CINZA_TEXTO);
}

} // namespace andv

#include "andvarion/ui/HUD.hpp"
#include "andvarion/cenas/Sessao.hpp"
#include "andvarion/core/Jogo.hpp"
#include "andvarion/entidades/Jogador.hpp"
#include "andvarion/gfx/VFXManager.hpp"
#include "andvarion/ui/Fonte.hpp"
#include "andvarion/ui/Pergaminho.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace andv {

namespace {
// Fantasma da barra: cai devagar até o valor real (mostra o dano recebido).
float aproximar(float fantasma, float real, float dt) {
    if (real >= fantasma) return real;
    return std::max(real, fantasma - dt * 0.6f);
}
} // namespace

void HUD::atualizar(const Jogador& j, float dt) {
    tempo_ += dt;
    vidaFantasma_ = aproximar(vidaFantasma_, j.vida / j.vidaMax, dt);
    manaFantasma_ = aproximar(manaFantasma_, j.recursos.mana.fracao(), dt);
    vigorFantasma_ = aproximar(vigorFantasma_, j.recursos.vigor.fracao(), dt);
}

void HUD::desenharBarras(const Fonte& fonte, const Jogador& j, const Sessao& s) const {
    ui::desenharPergaminho({4, 4, 138, 50}, 200);
    char buf[64];
    std::snprintf(buf, sizeof buf, "Nv %d", s.progressao.nivel());
    fonte.desenhar(buf, {10, 8}, 10.f, ui::DOURADO);
    if (s.progressao.pontos() > 0) {
        std::snprintf(buf, sizeof buf, "+%d (K)", s.progressao.pontos());
        const float pulso = 0.6f + 0.4f * std::sin(tempo_ * 5.f);
        fonte.desenhar(buf, {44, 8}, 10.f, Fade(ui::DOURADO, pulso));
    }
    ui::desenharBarra({10, 20, 126, 7}, j.vida / j.vidaMax, vidaFantasma_, ui::SANGUE);
    ui::desenharBarra({10, 29, 126, 5}, j.recursos.mana.fracao(), manaFantasma_, ui::ARCANO);
    ui::desenharBarra({10, 36, 126, 4}, j.recursos.vigor.fracao(), vigorFantasma_, Color{180, 170, 70, 255});
    ui::desenharBarra({10, 43, 126, 3}, s.progressao.fracaoNivel(), 0.f, ui::DOURADO, Color{20, 16, 10, 220});
    std::snprintf(buf, sizeof buf, "%d/%d", static_cast<int>(std::ceil(j.vida)), static_cast<int>(j.vidaMax));
    fonte.desenhar(buf, {136 - fonte.largura(buf), 8}, 10.f, ui::OSSO);

    // Sede de Sangue: o lembrete de que o próximo golpe está potencializado.
    if (j.sede.ativa()) {
        const float pulso = 0.55f + 0.45f * std::sin(tempo_ * 9.f);
        ui::desenharPergaminho({4, 56, 138, 16}, 200);
        std::snprintf(buf, sizeof buf, "SEDE DE SANGUE x%d", j.sede.cargas());
        fonte.desenhar(buf, {10, 59}, 10.f, Fade(Color{255, 70, 70, 255}, pulso));
        DrawRectangle(10, 69, static_cast<int>(126 * std::min(1.f, j.sede.restante() / 5.f)), 1, ui::SANGUE);
    }
}

void HUD::desenharFeiticos(const Fonte& fonte, const Jogador& j, const Sessao& s, const VFXManager& vfx) const {
    // Os 4 slots do grimório (teclas 1, 2, 3, Q) com a magia atribuída a cada um.
    const float tamanho = 24.f, espaco = 4.f;
    const float total = SLOTS_GRIMORIO * tamanho + (SLOTS_GRIMORIO - 1) * espaco;
    float x = LARGURA_VIRTUAL * 0.5f - total * 0.5f;
    const float y = ALTURA_VIRTUAL - tamanho - 6.f;
    for (int i = 0; i < SLOTS_GRIMORIO; ++i, x += tamanho + espaco) {
        const Rectangle r{x, y, tamanho, tamanho};
        const DefinicaoFeitico* f = feiticoPorId(s.grimorio[static_cast<std::size_t>(i)]);
        DrawRectangleRec(r, Color{18, 12, 14, 220});
        if (f) {
            // Ícone = o quadro do meio do próprio efeito do pixelfx, recortado no slot.
            vfx.desenharIcone(f->vfx, r);
            const float rec = j.recarga(f->id);
            if (rec > 0.f) {
                const float k = std::min(1.f, rec / f->recarga);
                DrawRectangleRec({x, y + tamanho * (1.f - k), tamanho, tamanho * k}, Color{0, 0, 0, 170});
            }
            if (!j.recursos.mana.tem(j.custoMana(*f))) DrawRectangleRec(r, Color{20, 20, 60, 120});
        }
        DrawRectangleLinesEx(r, 1.f, f ? ui::PERGAMINHO_BORDA : ui::PERGAMINHO_BORDA_ESCURA);
        fonte.desenhar(TECLAS_GRIMORIO[i], {x + 2, y + 1}, 10.f, f ? ui::OSSO : ui::CINZA_TEXTO);
    }
    // Efeitos ativos acima da barra.
    float ey = y - 12.f;
    if (j.escudoPontos > 0.f) {
        char b[48];
        std::snprintf(b, sizeof b, "Escudo Arcano %d", static_cast<int>(std::ceil(j.escudoPontos)));
        fonte.desenharCentro(b, {LARGURA_VIRTUAL * 0.5f, ey}, 10.f, Color{170, 150, 255, 255});
        ey -= 11.f;
    }
    if (j.encantamentoTempo > 0.f) {
        char b[48];
        std::snprintf(b, sizeof b, "Lâmina Encantada %ds", static_cast<int>(std::ceil(j.encantamentoTempo)));
        fonte.desenharCentro(b, {LARGURA_VIRTUAL * 0.5f, ey}, 10.f, Color{255, 170, 80, 255});
    }

    // Armas e frascos (canto inferior esquerdo).
    ui::desenharPergaminho({4, ALTURA_VIRTUAL - 40.f, 150, 36}, 200);
    fonte.desenhar("Lâmina: " + j.armaCorpo().nome, {9, ALTURA_VIRTUAL - 36.f}, 10.f, ui::OSSO);
    fonte.desenhar("Distância: " + j.armaDistancia().nome, {9, ALTURA_VIRTUAL - 25.f}, 10.f, ui::OSSO);
    char buf[48];
    std::snprintf(buf, sizeof buf, "Frascos de Sangue: %d/%d  [F]", s.frascos, s.frascosMax);
    fonte.desenhar(buf, {9, ALTURA_VIRTUAL - 14.f}, 10.f, Color{220, 120, 120, 255});
}

void HUD::desenharRastreador(const Fonte& fonte, const Sessao& s) const {
    const auto rastreadas = s.missoes->rastreadas();
    if (rastreadas.empty()) return;
    // Largura pelo texto mais longo (com teto), para nada sair da tela.
    float largura = 120.f;
    for (const Missao* m : rastreadas) {
        largura = std::max(largura, fonte.largura(m->titulo) + 12.f);
        for (const Objetivo& o : m->objetivos) largura = std::max(largura, fonte.largura("[ ] " + o.descricao + " 00/00") + 16.f);
    }
    largura = std::min(largura, 250.f);
    const float x = LARGURA_VIRTUAL - largura - 4.f;
    float altura = 6.f;
    for (const Missao* m : rastreadas) altura += 12.f + (m->estado == EstadoMissao::ProntaParaEntregar ? 11.f : m->objetivos.size() * 11.f);
    // Discreto: fundo semitransparente, sem moldura pesada.
    DrawRectangleRec({x, 4, largura, altura}, Color{10, 6, 8, 120});
    DrawRectangle(static_cast<int>(x), 4, 1, static_cast<int>(altura), Fade(ui::PERGAMINHO_BORDA, 0.7f));
    float y = 7.f;
    for (const Missao* m : rastreadas) {
        fonte.desenhar(m->titulo, {x + 5, y}, 10.f, m->principal ? ui::DOURADO : ui::OSSO);
        y += 12.f;
        if (m->estado == EstadoMissao::ProntaParaEntregar) {
            const auto* npc = s.dialogos.npc(m->entregarPara);
            fonte.desenhar("> Retorne a " + (npc ? npc->nome : m->entregarPara), {x + 9, y}, 10.f, Color{150, 220, 140, 255});
            y += 11.f;
            continue;
        }
        for (const Objetivo& o : m->objetivos) {
            char buf[96];
            if (o.quantidade > 1) std::snprintf(buf, sizeof buf, "%s %s %d/%d", o.completo() ? "[x]" : "[ ]", o.descricao.c_str(), o.atual, o.quantidade);
            else std::snprintf(buf, sizeof buf, "%s %s", o.completo() ? "[x]" : "[ ]", o.descricao.c_str());
            fonte.desenhar(buf, {x + 9, y}, 10.f, o.completo() ? ui::CINZA_TEXTO : ui::OSSO);
            y += 11.f;
        }
    }
}

void HUD::desenhar(const Fonte& fonte, const Jogador& j, const Sessao& s, const VFXManager& vfx) const {
    desenharBarras(fonte, j, s);
    desenharFeiticos(fonte, j, s, vfx);
    desenharRastreador(fonte, s);
    if (j.avisoTempo > 0.f) {
        const float a = std::min(1.f, j.avisoTempo * 2.f);
        fonte.desenharCentro(j.aviso, {LARGURA_VIRTUAL * 0.5f, ALTURA_VIRTUAL - 40.f}, 10.f, Fade(ui::OSSO, a));
    }
}

} // namespace andv

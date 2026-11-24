#include "andvarion/ui/MenuArvoreHabilidades.hpp"
#include "andvarion/cenas/Sessao.hpp"
#include "andvarion/core/Jogo.hpp"
#include "andvarion/core/Texto.hpp"
#include "andvarion/gfx/VFXManager.hpp"
#include "andvarion/ui/Fonte.hpp"
#include "andvarion/ui/Pergaminho.hpp"

#include <raymath.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace andv {

namespace {
constexpr Rectangle PAINEL{16, 12, LARGURA_VIRTUAL - 32.f, ALTURA_VIRTUAL - 24.f};
constexpr float RAIO_NO = 7.f;
const Vector2 BASE_TRONCO{LARGURA_VIRTUAL * 0.5f, PAINEL.y + PAINEL.height - 62.f};
const Vector2 FORQUILHA{LARGURA_VIRTUAL * 0.5f, PAINEL.y + PAINEL.height - 108.f};

const Color MADEIRA{78, 52, 34, 255};
const Color MADEIRA_CLARA{112, 78, 50, 255};
const Color MADEIRA_ESCURA{40, 26, 18, 255};

Color corEscola(const std::string& escola, const std::string& ramo) {
    if (escola == "sangue") return {200, 40, 50, 255};
    if (escola == "fogo") return {255, 140, 40, 255};
    if (escola == "gelo") return {130, 200, 255, 255};
    if (escola == "raio") return {240, 230, 120, 255};
    if (escola == "arcano") return {160, 120, 255, 255};
    if (escola == "sombra") return {130, 80, 200, 255};
    if (ramo == "guerreiro") return {200, 205, 220, 255};
    if (ramo == "arqueiro") return {120, 200, 120, 255};
    return {200, 180, 140, 255};
}

// Galho levemente curvo: ponto em t (0..1) com uma barriga perpendicular.
Vector2 pontoGalho(Vector2 inicio, float anguloGraus, float comprimento, float t) {
    const float a = anguloGraus * DEG2RAD;
    const Vector2 dir{std::sin(a), -std::cos(a)};
    const Vector2 perp{-dir.y, dir.x};
    const float curva = std::sin(t * PI) * 6.f * (anguloGraus >= 0 ? 1.f : -1.f);
    return Vector2Add(inicio, Vector2Add(Vector2Scale(dir, comprimento * t), Vector2Scale(perp, curva)));
}

// Galho grosso desenhado como sequência de segmentos (afina até a ponta).
void desenharMadeira(Vector2 a, float ang, float comp, float espessura0, float espessura1, Color cor) {
    const int n = 10;
    for (int k = 0; k < n; ++k) {
        const float t0 = static_cast<float>(k) / n, t1 = static_cast<float>(k + 1) / n;
        const float e = espessura0 + (espessura1 - espessura0) * t0;
        DrawLineEx(pontoGalho(a, ang, comp, t0), pontoGalho(a, ang, comp, t1), e + 2.f, MADEIRA_ESCURA);
        DrawLineEx(pontoGalho(a, ang, comp, t0), pontoGalho(a, ang, comp, t1), e, cor);
    }
}

struct InfoGalho { int total = 0; int aprendidos = 0; };

// ColorLerp só existe a partir da raylib 5.5.
Color misturar(Color a, Color b, float t) {
    auto m = [t](unsigned char x, unsigned char y) { return static_cast<unsigned char>(x + (y - x) * t); };
    return {m(a.r, b.r), m(a.g, b.g), m(a.b, b.b), m(a.a, b.a)};
}
} // namespace

Vector2 MenuArvoreHabilidades::pontaDoRamo(const std::string& ramo) {
    if (ramo == "guerreiro") return {LARGURA_VIRTUAL * 0.5f - 190.f, FORQUILHA.y - 30.f};
    if (ramo == "arqueiro") return {LARGURA_VIRTUAL * 0.5f + 190.f, FORQUILHA.y - 30.f};
    return {LARGURA_VIRTUAL * 0.5f, FORQUILHA.y - 52.f};
}

Vector2 MenuArvoreHabilidades::posicaoNo(const NoHabilidade& n, const ArvoreHabilidades& a) {
    const GalhoHabilidade* g = a.galho(n.galho);
    if (!g) return {0, 0};
    int total = 0;
    for (const auto& outro : a.nos())
        if (outro.galho == n.galho) ++total;
    // Nós da metade do galho até a ponta: na base os galhos do leque ainda estão juntos.
    const float t = 0.5f + 0.5f * (total > 1 ? static_cast<float>(n.ordem) / (total - 1) : 1.f);
    return pontoGalho(pontaDoRamo(g->ramo), g->angulo, g->comprimento, t);
}

void MenuArvoreHabilidades::atualizar(float dt, const ComandosJogador& cmd, Sessao& s) {
    if (!aberto_) return;
    tempo_ += dt;
    tempoMensagem_ = std::max(0.f, tempoMensagem_ - dt);
    for (auto& e : explosoes_) e.t += dt;
    explosoes_.erase(std::remove_if(explosoes_.begin(), explosoes_.end(), [](const Explosao& e) { return e.t > 1.8f; }),
                     explosoes_.end());
    if (cmd.arvore || cmd.cancelar || cmd.pausar) {
        fechar();
        return;
    }
    // Abas: clique ou A/D.
    for (int i = 0; i < 2; ++i)
        if (cmd.clique && CheckCollisionPointRec(cmd.mouseVirtual, {PAINEL.x + 10 + i * 96.f, PAINEL.y + 6, 92, 14})) aba_ = i;
    if (cmd.esquerda || cmd.direita) aba_ = 1 - aba_;
    if (aba_ != 0) return;

    const auto& nos = s.arvore.nos();
    focado_ = -1;
    for (std::size_t i = 0; i < nos.size(); ++i)
        if (Vector2Distance(cmd.mouseVirtual, posicaoNo(nos[i], s.arvore)) <= RAIO_NO + 2.f) focado_ = static_cast<int>(i);
    if (focado_ < 0) return;
    const NoHabilidade& n = nos[static_cast<std::size_t>(focado_)];

    // 1/2/3/Q sobre uma magia aprendida: coloca no slot do grimório.
    for (int slot = 0; slot < SLOTS_GRIMORIO; ++slot) {
        if (!cmd.feitico[slot] || n.efeito != TipoEfeitoHabilidade::Feitico) continue;
        if (s.atribuirMagia(slot, n.alvo)) {
            mensagem_ = n.nome + " no slot " + TECLAS_GRIMORIO[slot];
            TraceLog(LOG_INFO, "ANDVARION: grimório %s = %s", TECLAS_GRIMORIO[slot], n.alvo.c_str());
        }
        else mensagem_ = "Aprenda a magia antes de colocá-la no grimório.";
        tempoMensagem_ = 2.5f;
    }
    if (cmd.clique) {
        const auto r = s.arvore.desbloquear(n.id, s.progressao);
        mensagem_ = textoResultado(r);
        tempoMensagem_ = 2.5f;
        if (r == ResultadoDesbloqueio::Ok) {
            explosoes_.push_back({posicaoNo(n, s.arvore), 0.f, n.vfx});
            if (aoDesbloquear_) aoDesbloquear_(n);
        }
    }
}

void MenuArvoreHabilidades::desenharArvore(const Fonte& fonte, const Sessao& s, const VFXManager& vfx) const {
    const ArvoreHabilidades& a = s.arvore;
    // Céu atrás da copa e chão.
    DrawCircleGradient(static_cast<int>(FORQUILHA.x), static_cast<int>(FORQUILHA.y - 60), 230.f, Color{40, 28, 48, 160}, Color{0, 0, 0, 0});
    DrawEllipse(static_cast<int>(BASE_TRONCO.x), static_cast<int>(BASE_TRONCO.y + 4), 90, 8, Color{20, 14, 12, 200});

    // Tronco (com raízes) e os três ramos grossos.
    for (float r : {-1.f, 1.f})
        desenharMadeira(BASE_TRONCO, 180.f - 60.f * r, 26.f, 6.f, 1.f, MADEIRA);
    DrawLineEx(BASE_TRONCO, FORQUILHA, 16.f, MADEIRA_ESCURA);
    DrawLineEx(BASE_TRONCO, FORQUILHA, 13.f, MADEIRA);
    DrawLineEx({BASE_TRONCO.x - 3, BASE_TRONCO.y}, {FORQUILHA.x - 3, FORQUILHA.y}, 2.f, MADEIRA_CLARA);
    for (const char* ramo : {"guerreiro", "mago", "arqueiro"}) {
        const Vector2 ponta = pontaDoRamo(ramo);
        const float ang = std::atan2(ponta.x - FORQUILHA.x, -(ponta.y - FORQUILHA.y)) * RAD2DEG;
        desenharMadeira(FORQUILHA, ang, Vector2Distance(FORQUILHA, ponta), 11.f, 7.f, MADEIRA);
    }

    // Galhos: madeira que "acende" com a cor da escola conforme é aprendido.
    for (const auto& g : a.galhos()) {
        InfoGalho info;
        for (const auto& n : a.nos())
            if (n.galho == g.id) {
                ++info.total;
                info.aprendidos += a.desbloqueado(n.id) ? 1 : 0;
            }
        const Color cor = info.aprendidos > 0 ? misturar(MADEIRA, corEscola(g.escola, g.ramo), 0.35f) : MADEIRA;
        desenharMadeira(pontaDoRamo(g.ramo), g.angulo, g.comprimento, 5.f, 2.f, cor);
    }
    for (const char* ramo : {"guerreiro", "arqueiro"}) {
        const Vector2 p = pontaDoRamo(ramo);
        fonte.desenharCentro(std::string(ramo) == "guerreiro" ? "Guerreiro" : "Arqueiro", {p.x, p.y + 18.f}, 10.f, corEscola("", ramo));
    }
    fonte.desenharCentro("Mago", {FORQUILHA.x, FORQUILHA.y - 30.f}, 10.f, Color{200, 170, 255, 255});

    // Nós.
    const auto& nos = a.nos();
    for (std::size_t i = 0; i < nos.size(); ++i) {
        const NoHabilidade& n = nos[i];
        const GalhoHabilidade* g = a.galho(n.galho);
        const Vector2 c = posicaoNo(n, a);
        const bool tem = a.desbloqueado(n.id);
        const bool pode = a.podeDesbloquear(n.id, s.progressao) == ResultadoDesbloqueio::Ok;
        const Color cor = corEscola(g ? g->escola : "", n.ramo);
        if (tem) {
            // Folhas em volta do nó aprendido.
            for (int k = 0; k < 5; ++k) {
                const float ang = static_cast<float>(k) * 72.f + static_cast<float>(i) * 37.f;
                const Vector2 f{c.x + std::cos(ang * DEG2RAD) * 9.f, c.y + std::sin(ang * DEG2RAD) * 7.f};
                DrawEllipse(static_cast<int>(f.x), static_cast<int>(f.y), 3.f, 2.f, Color{70, 120, 60, 230});
            }
        }
        if (pode) {
            const float pulso = 0.5f + 0.5f * std::sin(tempo_ * 6.f);
            DrawCircleLinesV(c, RAIO_NO + 3.f, Fade(ui::DOURADO, pulso));
        }
        DrawCircleV(c, RAIO_NO + 1.f, MADEIRA_ESCURA);
        DrawCircleV(c, RAIO_NO, tem ? Fade(cor, 0.9f) : Color{26, 18, 14, 255});
        if (n.efeito == TipoEfeitoHabilidade::Feitico) {
            // Magia: o ícone é o próprio efeito do pixelfx, recortado no nó.
            if (const DefinicaoFeitico* f = feiticoPorId(n.alvo)) {
                vfx.desenharIcone(f->vfx, {c.x - RAIO_NO, c.y - RAIO_NO, RAIO_NO * 2, RAIO_NO * 2}, 2.4f,
                                  tem ? WHITE : Color{120, 110, 100, 200});
            }
            DrawCircleLinesV(c, RAIO_NO, tem ? ui::DOURADO : cor);
        } else {
            const char* simbolo = n.efeito == TipoEfeitoHabilidade::Passiva ? "o" : "+";
            fonte.desenharCentro(simbolo, {c.x, c.y}, 10.f, tem ? Color{20, 12, 10, 255} : cor, false);
            DrawCircleLinesV(c, RAIO_NO, tem ? ui::DOURADO : Color{70, 56, 46, 255});
        }
        if (static_cast<int>(i) == focado_) DrawCircleLinesV(c, RAIO_NO + 2.f, ui::OSSO);
    }
    for (const auto& e : explosoes_) {
        ParametrosVfx p;
        p.escala = 0.3f;
        p.aditivo = true;
        vfx.desenharQuadro(e.vfx.empty() ? "habilidade_desbloqueada" : e.vfx,
                           std::min(e.t, vfx.duracao("habilidade_desbloqueada") - 0.01f), {e.pos.x, e.pos.y + 6.f}, p);
    }

    // Descrição do nó sob o mouse.
    const Rectangle info{PAINEL.x + 8, PAINEL.y + PAINEL.height - 44, PAINEL.width - 196, 36};
    DrawRectangleRec(info, Color{14, 9, 8, 210});
    if (focado_ >= 0) {
        const auto& n = nos[static_cast<std::size_t>(focado_)];
        const GalhoHabilidade* g = a.galho(n.galho);
        char buf[96];
        std::snprintf(buf, sizeof buf, "  [%s · custo %d · nível %d]", g ? g->nome.c_str() : "", n.custo, n.nivelMinimo);
        fonte.desenhar(n.nome + buf, {info.x + 4, info.y + 4}, 10.f, ui::DOURADO);
        fonte.desenhar(n.descricao, {info.x + 4, info.y + 19}, 10.f, ui::OSSO);
    } else {
        fonte.desenhar("Clique num nó para aprender. Sobre uma magia, 1/2/3/Q a põe no grimório.", {info.x + 4, info.y + 4}, 10.f,
                       ui::CINZA_TEXTO);
        fonte.desenhar("A/D: abas    K: fechar", {info.x + 4, info.y + 19}, 10.f, ui::CINZA_TEXTO);
    }
    if (tempoMensagem_ > 0.f) fonte.desenharCentro(mensagem_, {LARGURA_VIRTUAL * 0.5f, info.y - 10.f}, 10.f, ui::DOURADO);
}

void MenuArvoreHabilidades::desenharGrimorio(const Fonte& fonte, const Sessao& s, const VFXManager& vfx) const {
    const Rectangle area{PAINEL.x + PAINEL.width - 184, PAINEL.y + PAINEL.height - 44, 176, 36};
    DrawRectangleRec(area, Color{14, 9, 8, 210});
    fonte.desenhar("Grimório", {area.x + 4, area.y + 2}, 10.f, ui::DOURADO);
    for (int i = 0; i < SLOTS_GRIMORIO; ++i) {
        const Rectangle r{area.x + 4 + i * 43.f, area.y + 13, 20, 20};
        DrawRectangleRec(r, Color{24, 16, 18, 255});
        if (const DefinicaoFeitico* f = feiticoPorId(s.grimorio[static_cast<std::size_t>(i)])) {
            vfx.desenharIcone(f->vfx, r);
        }
        DrawRectangleLinesEx(r, 1.f, ui::PERGAMINHO_BORDA);
        fonte.desenhar(TECLAS_GRIMORIO[i], {r.x + 22, r.y + 5}, 10.f, ui::OSSO);
    }
}

void MenuArvoreHabilidades::desenharProficiencias(const Fonte& fonte, const Sessao& s) const {
    static const char* armas[] = {"espada", "adaga", "machado", "foice", "bastao", "arco", "besta", "bumerangue"};
    static const char* escolas[] = {"sangue", "fogo", "gelo", "raio", "arcano", "sombra"};
    auto coluna = [&](const char* titulo, const char* const* chaves, int n, float x, bool escola) {
        fonte.desenhar(titulo, {x, PAINEL.y + 34}, 10.f, ui::DOURADO);
        for (int i = 0; i < n; ++i) {
            const float y = PAINEL.y + 52 + i * 30.f;
            const std::string c = chaves[i];
            const int nivel = s.proficiencia.nivel(c);
            char buf[64];
            std::snprintf(buf, sizeof buf, "%s  Nv %d", nomeProficiencia(c), nivel);
            fonte.desenhar(buf, {x, y}, 10.f, ui::OSSO);
            ui::desenharBarra({x, y + 12, 160, 5}, s.proficiencia.fracao(c), 0.f,
                              escola ? corEscola(c, "") : Color{200, 205, 220, 255});
            if (nivel > 1) {
                if (escola) std::snprintf(buf, sizeof buf, "+%d%% dano  -%d%% mana", (nivel - 1) * 4, (nivel - 1) * 2);
                else std::snprintf(buf, sizeof buf, "+%d%% dano", (nivel - 1) * 4);
                fonte.desenhar(buf, {x + 168, y + 8}, 10.f, Color{150, 220, 140, 255});
            }
        }
    };
    coluna("Armas (sobe acertando)", armas, 8, PAINEL.x + 24, false);
    coluna("Escolas de magia (sobe conjurando)", escolas, 6, PAINEL.x + 320, true);
}

void MenuArvoreHabilidades::desenhar(const Fonte& fonte, const Sessao& s, const VFXManager& vfx) const {
    if (!aberto_) return;
    DrawRectangle(0, 0, LARGURA_VIRTUAL, ALTURA_VIRTUAL, Color{0, 0, 0, 170});
    ui::desenharPergaminho(PAINEL);
    const char* abas[2] = {"Árvore", "Proficiências"};
    for (int i = 0; i < 2; ++i) {
        const Rectangle r{PAINEL.x + 10 + i * 96.f, PAINEL.y + 6, 92, 14};
        if (i == aba_) DrawRectangleRec(r, Color{90, 30, 36, 220});
        DrawRectangleLinesEx(r, 1.f, i == aba_ ? ui::DOURADO : ui::PERGAMINHO_BORDA_ESCURA);
        fonte.desenharCentro(abas[i], {r.x + r.width * 0.5f, r.y + 7}, 10.f, i == aba_ ? ui::DOURADO : ui::CINZA_TEXTO);
    }
    char buf[48];
    std::snprintf(buf, sizeof buf, "Pontos: %d", s.progressao.pontos());
    fonte.desenhar(buf, {PAINEL.x + PAINEL.width - 80, PAINEL.y + 8}, 10.f, s.progressao.pontos() > 0 ? ui::DOURADO : ui::CINZA_TEXTO);
    if (aba_ == 0) {
        desenharArvore(fonte, s, vfx);
        desenharGrimorio(fonte, s, vfx);
    } else {
        desenharProficiencias(fonte, s);
    }
}

} // namespace andv

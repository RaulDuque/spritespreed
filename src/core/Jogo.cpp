#include "andvarion/core/Jogo.hpp"
#include "andvarion/cenas/CenaMenuPrincipal.hpp"
#include "andvarion/core/Assets.hpp"
#include "andvarion/core/EventBus.hpp"
#include "andvarion/core/Roteiro.hpp"
#include "andvarion/gfx/VFXManager.hpp"
#include "andvarion/save/SaveSystem.hpp"
#include "andvarion/ui/Fonte.hpp"

#include <rlgl.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace andv {

Jogo::Jogo() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(LARGURA_VIRTUAL * 2, ALTURA_VIRTUAL * 2, "Andvarion — O Sussurro sob a Catedral");
    SetWindowMinSize(LARGURA_VIRTUAL, ALTURA_VIRTUAL);
    SetExitKey(KEY_NULL);  // Esc abre a pausa, não fecha o jogo
    SetTargetFPS(60);

    tela_ = LoadRenderTexture(LARGURA_VIRTUAL, ALTURA_VIRTUAL);
    SetTextureFilter(tela_.texture, TEXTURE_FILTER_POINT);

    assets_ = std::make_unique<Assets>();
    bus_ = std::make_unique<EventBus>();
    vfx_ = std::make_unique<VFXManager>();
    vfx_->carregar(*assets_);
    fonte_ = std::make_unique<Fonte>(*assets_);
    // Save ao lado do executável: o jogo é portátil (pendrive, pasta do Steam etc.).
    // ANDV_SAVE troca o arquivo (testes automáticos não mexem no save do jogador).
    const char* saveEnv = std::getenv("ANDV_SAVE");
    save_ = std::make_unique<SaveSystem>(saveEnv && *saveEnv ? std::string(saveEnv)
                                                             : std::string(GetApplicationDirectory()) + "saves/save1.json");

    const std::string fs = assets_->caminho("shaders/atmosfera.fs");
    if (FileExists(fs.c_str())) {
        atmosfera_ = LoadShader(nullptr, fs.c_str());
        temShader_ = atmosfera_.id != 0 && atmosfera_.id != rlGetShaderIdDefault();
        if (temShader_) {
            locTempo_ = GetShaderLocation(atmosfera_, "tempo");
            locResolucao_ = GetShaderLocation(atmosfera_, "resolucao");
        }
    }

    if (const char* r = std::getenv("ANDV_ROTEIRO")) {
        roteiro_ = std::make_unique<Roteiro>();
        if (roteiro_->carregar(r)) {
            definirRoteiroAtivo(roteiro_.get());
            TraceLog(LOG_INFO, "ANDVARION: roteiro de teste %s", r);
        } else {
            TraceLog(LOG_ERROR, "ANDVARION: roteiro inválido %s", r);
            roteiro_.reset();
        }
    }

    cena_ = std::make_unique<CenaMenuPrincipal>(*this);
}

Jogo::~Jogo() {
    // Ordem importa: tudo que segura textura morre antes do contexto OpenGL.
    definirRoteiroAtivo(nullptr);
    proxima_.reset();
    cena_.reset();
    vfx_.reset();
    fonte_.reset();
    assets_.reset();
    if (temShader_) UnloadShader(atmosfera_);
    UnloadRenderTexture(tela_);
    CloseWindow();
}

void Jogo::trocarCena(std::unique_ptr<Cena> nova) { proxima_ = std::move(nova); }

Vector2 Jogo::mouseVirtual() const {
    const float sw = static_cast<float>(GetScreenWidth()), sh = static_cast<float>(GetScreenHeight());
    float escala = std::min(sw / LARGURA_VIRTUAL, sh / ALTURA_VIRTUAL);
    if (escala >= 1.f) escala = std::floor(escala);
    const float ox = (sw - LARGURA_VIRTUAL * escala) * 0.5f, oy = (sh - ALTURA_VIRTUAL * escala) * 0.5f;
    if (roteiro_) return roteiro_->mouse();  // testes roteirizados ignoram o mouse real
    const Vector2 m = GetMousePosition();
    return {std::clamp((m.x - ox) / escala, 0.f, static_cast<float>(LARGURA_VIRTUAL - 1)),
            std::clamp((m.y - oy) / escala, 0.f, static_cast<float>(ALTURA_VIRTUAL - 1))};
}

void Jogo::blit() {
    const float sw = static_cast<float>(GetScreenWidth()), sh = static_cast<float>(GetScreenHeight());
    float escala = std::min(sw / LARGURA_VIRTUAL, sh / ALTURA_VIRTUAL);
    if (escala >= 1.f) escala = std::floor(escala);  // fator inteiro sempre que couber
    const float w = LARGURA_VIRTUAL * escala, h = ALTURA_VIRTUAL * escala;
    const Rectangle destino{std::floor((sw - w) * 0.5f), std::floor((sh - h) * 0.5f), w, h};

    BeginDrawing();
    ClearBackground(BLACK);
    if (temShader_) {
        const float t = static_cast<float>(tempo_);
        const float res[2] = {static_cast<float>(LARGURA_VIRTUAL), static_cast<float>(ALTURA_VIRTUAL)};
        SetShaderValue(atmosfera_, locTempo_, &t, SHADER_UNIFORM_FLOAT);
        SetShaderValue(atmosfera_, locResolucao_, res, SHADER_UNIFORM_VEC2);
        BeginShaderMode(atmosfera_);
    }
    DrawTexturePro(tela_.texture, {0, 0, static_cast<float>(LARGURA_VIRTUAL), -static_cast<float>(ALTURA_VIRTUAL)},
                   destino, {0, 0}, 0.f, WHITE);
    if (temShader_) EndShaderMode();
    if (roteiro_) {
        for (const auto& arquivo : roteiro_->capturas) TakeScreenshot(arquivo.c_str());
        roteiro_->capturas.clear();
    }
    EndDrawing();
}

int Jogo::executar() {
    double acumulado = 0.0;
    while (!sair_ && !WindowShouldClose()) {
        const double dt = std::min(static_cast<double>(GetFrameTime()), 0.25);  // trava espiral após travadas
        acumulado += dt;
        tempo_ += dt;

        if (roteiro_) {
            // Testes de fumaça também medem desempenho: FPS no log a cada 2 s.
            static double proximoLog = 2.0;
            static int quadros = 0;
            ++quadros;
            if (tempo_ >= proximoLog) {
                TraceLog(LOG_INFO, "ANDVARION: %d FPS", quadros / 2);
                proximoLog += 2.0;
                quadros = 0;
            }
            roteiro_->avancar(static_cast<float>(dt));
            if (roteiro_->pediuSaida) sair_ = true;
        }
        if (cena_->usaCursorDoSistema()) ShowCursor();
        else HideCursor();

        cena_->lerEntrada();
        while (acumulado >= PASSO_FIXO) {
            cena_->atualizar(PASSO_FIXO);
            acumulado -= PASSO_FIXO;
            if (proxima_) break;
        }
        if (proxima_) {
            cena_ = std::move(proxima_);
            acumulado = 0.0;
            continue;
        }

        cena_->prepararQuadro();
        BeginTextureMode(tela_);
        ClearBackground(BLACK);
        cena_->desenhar();
        EndTextureMode();
        blit();
    }
    return 0;
}

} // namespace andv

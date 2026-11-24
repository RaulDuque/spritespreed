#include "andvarion/dialogo/DialogueSystem.hpp"
#include "andvarion/core/Assets.hpp"
#include "andvarion/core/EventBus.hpp"
#include "andvarion/core/Jogo.hpp"
#include "andvarion/core/Texto.hpp"
#include "andvarion/missoes/QuestManager.hpp"
#include "andvarion/progressao/Inventario.hpp"
#include "andvarion/ui/Fonte.hpp"
#include "andvarion/ui/Pergaminho.hpp"

#include <algorithm>
#include <cmath>

namespace andv {

namespace {
// Layout na tela virtual 480x270.
// Ancorado na base da tela virtual.
constexpr float BASE = static_cast<float>(ALTURA_VIRTUAL);
constexpr Rectangle CAIXA{8, BASE - 76, LARGURA_VIRTUAL - 16.f, 72};
constexpr Rectangle MOLDURA_RETRATO{16, BASE - 68, 56, 56};
constexpr float TEXTO_X = 82.f;
constexpr float NOME_Y = BASE - 70;
constexpr float TEXTO_Y = BASE - 56;
constexpr float TEXTO_LARGURA = LARGURA_VIRTUAL - 100.f;
constexpr float ALTURA_LINHA = 11.f;
} // namespace

DialogueSystem::DialogueSystem(const BancoDialogos& banco, QuestManager& missoes, Inventario& inventario, Assets& assets,
                               EventBus& bus)
    : banco_(banco), missoes_(missoes), inventario_(inventario), assets_(assets), bus_(bus) {}

bool DialogueSystem::iniciar(const std::string& npcId) {
    npc_ = banco_.npc(npcId);
    if (!npc_) {
        TraceLog(LOG_WARNING, "ANDVARION: NPC sem diálogo: %s", npcId.c_str());
        return false;
    }
    const auto entrada = escolherEntrada(*npc_, missoes_, inventario_);
    if (!entrada) {
        npc_ = nullptr;
        return false;
    }
    retrato_ = npc_->retrato.empty() ? nullptr : assets_.textura(npc_->retrato);
    irPara(*entrada);
    return true;
}

void DialogueSystem::encerrar() {
    if (npc_) bus_.publicar(TipoEvento::NpcFalado, npc_->id);  // objetivos FALAR_NPC
    npc_ = nullptr;
    no_ = nullptr;
    opcoes_.clear();
}

void DialogueSystem::irPara(const std::string& noId) {
    if (noId.empty() || !npc_) {
        encerrar();
        return;
    }
    auto it = npc_->nos.find(noId);
    if (it == npc_->nos.end()) {
        encerrar();
        return;
    }
    no_ = &it->second;
    digitados_ = 0.f;
    totalCodepoints_ = texto::contarCodepoints(no_->texto);
    selecionada_ = 0;
    opcoes_.clear();
    for (const auto& o : no_->opcoes)
        if (avaliarCondicoes(o.condicoes, missoes_, inventario_)) opcoes_.push_back(&o);
    executar(no_->acoes);
}

void DialogueSystem::executar(const std::vector<std::string>& acoes) {
    for (const auto& a : acoes) {
        if (a.rfind("aceitar:", 0) == 0) missoes_.aceitar(a.substr(8));
        else if (a.rfind("entregar:", 0) == 0) missoes_.entregar(a.substr(9));
        else if (executor_) executor_(a);
    }
}

void DialogueSystem::escolher(int i) {
    if (i < 0 || i >= static_cast<int>(opcoes_.size())) return;
    const OpcaoDialogo* o = opcoes_[static_cast<std::size_t>(i)];
    const std::string proximo = o->proximo;  // copia: executar pode mudar o estado
    executar(o->acoes);
    if (ativo()) irPara(proximo);
}

Rectangle DialogueSystem::retanguloOpcao(int i, const Fonte& fonte) const {
    float largura = 120.f;
    for (const auto* o : opcoes_) largura = std::max(largura, fonte.largura(o->texto) + 22.f);
    const float n = static_cast<float>(opcoes_.size());
    const float x = CAIXA.x + CAIXA.width - largura;
    const float y0 = CAIXA.y - 6.f - n * 13.f;
    return {x + 4, y0 + 4 + static_cast<float>(i) * 13.f, largura - 8, 12};
}

void DialogueSystem::atualizar(float dt, const ComandosJogador& cmd) {
    if (!ativo()) return;
    tempo_ += dt;
    digitados_ = std::min(static_cast<float>(totalCodepoints_), digitados_ + dt * CARACTERES_POR_SEGUNDO);

    const bool avancar = cmd.confirmar || cmd.clique;
    if (cmd.cancelar) {
        encerrar();
        return;
    }
    if (!completo()) {
        if (avancar) digitados_ = static_cast<float>(totalCodepoints_);  // pula a digitação
        return;
    }
    if (opcoes_.empty()) {
        if (avancar) irPara(no_->proximo);
        return;
    }
    const int n = static_cast<int>(opcoes_.size());
    if (cmd.cima) selecionada_ = (selecionada_ + n - 1) % n;
    if (cmd.baixo) selecionada_ = (selecionada_ + 1) % n;
    // Mouse: passar por cima seleciona; clicar escolhe.
    if (fonteCache_ && (cmd.mouseVirtual.x != mouse_.x || cmd.mouseVirtual.y != mouse_.y || cmd.clique)) {
        mouse_ = cmd.mouseVirtual;
        for (int i = 0; i < n; ++i) {
            if (CheckCollisionPointRec(mouse_, retanguloOpcao(i, *fonteCache_))) {
                selecionada_ = i;
                if (cmd.clique) {
                    escolher(i);
                    return;
                }
            }
        }
        if (cmd.clique) return;  // clique fora das opções não escolhe
    }
    if (cmd.confirmar) escolher(selecionada_);
}

void DialogueSystem::desenhar(const Fonte& fonte) const {
    if (!ativo()) return;
    fonteCache_ = &fonte;
    ui::desenharPergaminho(CAIXA);

    // Retrato em pixel art à esquerda.
    DrawRectangleRec(MOLDURA_RETRATO, Color{14, 10, 12, 255});
    if (retrato_ && retrato_->valida()) {
        const Texture2D& t = retrato_->get();
        const float escala = std::max(1.f, std::floor(std::min(52.f / t.width, 52.f / t.height)));
        const float w = t.width * escala, h = t.height * escala;
        DrawTextureEx(t, {MOLDURA_RETRATO.x + (MOLDURA_RETRATO.width - w) * 0.5f, MOLDURA_RETRATO.y + (MOLDURA_RETRATO.height - h) * 0.5f},
                      0.f, escala, WHITE);
    }
    DrawRectangleLinesEx(MOLDURA_RETRATO, 1.f, ui::PERGAMINHO_BORDA);
    DrawRectangleLinesEx({MOLDURA_RETRATO.x - 2, MOLDURA_RETRATO.y - 2, MOLDURA_RETRATO.width + 4, MOLDURA_RETRATO.height + 4},
                         1.f, ui::PERGAMINHO_BORDA_ESCURA);

    // Nome.
    const std::string& nome = no_->falante.empty() ? npc_->nome : no_->falante;
    fonte.desenhar(nome, {TEXTO_X, NOME_Y}, 10.f, ui::DOURADO);
    const int linhaY = static_cast<int>(NOME_Y) + 11;
    DrawLine(static_cast<int>(TEXTO_X), linhaY, static_cast<int>(TEXTO_X + fonte.largura(nome) + 20), linhaY, ui::PERGAMINHO_BORDA_ESCURA);

    // Texto com efeito de máquina de escrever. A quebra usa o texto COMPLETO,
    // então as palavras não "pulam" de linha enquanto são digitadas.
    const auto linhas = texto::quebrarLinhas(no_->texto, TEXTO_LARGURA,
                                             [&fonte](const std::string& s) { return fonte.largura(s); });
    int restantes = static_cast<int>(digitados_);
    float y = TEXTO_Y;
    for (const auto& linha : linhas) {
        if (restantes <= 0) break;
        const int cp = texto::contarCodepoints(linha);
        fonte.desenhar(texto::prefixo(linha, restantes), {TEXTO_X, y}, 10.f, ui::OSSO);
        restantes -= cp + 1;  // +1 pelo espaço consumido na quebra
        y += ALTURA_LINHA;
    }

    if (!completo()) return;
    if (opcoes_.empty()) {
        // Indicador "continuar" piscando.
        if (std::fmod(tempo_, 0.8f) < 0.5f) {
            const float x = CAIXA.x + CAIXA.width - 14, yy = CAIXA.y + CAIXA.height - 12;
            ui::triangulo({x, yy}, {x + 4, yy + 5}, {x + 8, yy}, ui::DOURADO);
        }
        return;
    }
    // Painel de escolhas acima da caixa, à direita.
    const Rectangle primeira = retanguloOpcao(0, fonte);
    const float n = static_cast<float>(opcoes_.size());
    ui::desenharPergaminho({primeira.x - 4, primeira.y - 4, primeira.width + 8, n * 13.f + 6.f});
    for (int i = 0; i < static_cast<int>(opcoes_.size()); ++i) {
        const Rectangle r = retanguloOpcao(i, fonte);
        const bool sel = i == selecionada_;
        if (sel) DrawRectangleRec(r, Color{90, 30, 36, 200});
        fonte.desenhar((sel ? "> " : "  ") + opcoes_[static_cast<std::size_t>(i)]->texto, {r.x + 3, r.y + 1}, 10.f,
                       sel ? ui::DOURADO : ui::CINZA_TEXTO);
    }
}

} // namespace andv

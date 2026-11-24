#pragma once
// Mundo — uma área jogável carregada (mapa + entidades + luzes + VFX ambiente).
//
// É o "contexto" passado às entidades: elas pedem o mapa para colidir, o VFX
// para efeitos, o EventBus para avisar mortes/coletas e o próprio Mundo para
// criar projéteis e achar alvos. O Jogador não pertence ao Mundo (sobrevive à
// troca de área); o Mundo só guarda uma referência.
#include "andvarion/core/Entrada.hpp"
#include "andvarion/entidades/Entidade.hpp"
#include "andvarion/gfx/FilaRender.hpp"
#include "andvarion/mundo/MapaTiles.hpp"

#include <memory>
#include <optional>
#include <random>
#include <string>
#include <vector>

namespace andv {

class Jogo;
class Assets;
class VFXManager;
class SistemaIluminacao;
class CameraJogo;
class EventBus;
class Jogador;
class NPC;
struct Sessao;

struct PedidoTransicao {
    std::string destino;
    std::string spawn;
    std::string rotulo;
};

class Mundo {
public:
    Mundo(Jogo& jogo, Sessao& sessao, Jogador& jogador, SistemaIluminacao& luz, CameraJogo& camera);
    ~Mundo();

    // Carrega assets/mapas/<area>.json e popula entidades. `posicao` vem do save.
    bool carregarArea(const std::string& area, const std::string& spawn, std::optional<Vector2> posicao);

    void atualizar(float dt, const ComandosJogador& cmd);
    // Só luzes, VFX e câmera (diálogo aberto: o mundo "respira", mas ninguém age).
    void atualizarApresentacao(float dt);
    void desenhar() const;          // dentro de BeginMode2D

    // --- serviços para as entidades ---
    MapaTiles& mapa() { return mapa_; }
    const MapaTiles& mapa() const { return mapa_; }
    VFXManager& vfx();
    const VFXManager& vfx() const;
    SistemaIluminacao& luz() { return luz_; }
    CameraJogo& camera() { return camera_; }
    EventBus& bus();
    Assets& assets();
    Jogador& jogador() { return jogador_; }
    const Jogador& jogador() const { return jogador_; }
    Sessao& sessao() { return sessao_; }
    const Sessao& sessao() const { return sessao_; }
    float aleatorio(float a, float b);

    void adicionar(std::unique_ptr<Entidade> e);
    // Combatentes vivos da facção oposta a `atacante` dentro do raio.
    std::vector<Combatente*> alvosNoRaio(Vector2 centro, float raio, Faccao atacante);
    NPC* npcProximo(Vector2 pos, float raio);

    void hitStop(float segundos);
    bool congelado() const { return congelamento_ > 0.f; }

    void soltarItem(const std::string& itemId, Vector2 pos, const std::string& uid = {});
    void recompensarXP(i32 xp);

    std::optional<PedidoTransicao> pedidoTransicao;   // lido pela CenaJogo
    const ObjetoMapa* transicaoProxima() const { return transicaoProxima_; }

private:
    void popularObjetos();

    Jogo& jogo_;
    Sessao& sessao_;
    Jogador& jogador_;
    SistemaIluminacao& luz_;
    CameraJogo& camera_;
    MapaTiles mapa_;
    std::vector<std::unique_ptr<Entidade>> entidades_;
    std::vector<std::unique_ptr<Entidade>> novas_;   // criadas durante o update
    std::vector<u32> vfxAmbiente_;
    mutable FilaRender fila_;
    std::mt19937 rng_{1337u};
    float congelamento_ = 0.f;
    float tempoTransicao_ = 0.f;
    const ObjetoMapa* transicaoProxima_ = nullptr;
};

} // namespace andv

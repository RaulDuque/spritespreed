#pragma once
// Estados concretos do protagonista. Ver o diagrama em PlayerStateMachine.hpp.
#include "andvarion/combate/PlayerStateMachine.hpp"

namespace andv {

// Base da locomoção: trata as transições "globais" (atacar, mirar, conjurar,
// esquivar) que valem tanto parado quanto andando.
class EstadoLocomocao : public EstadoJogador {
protected:
    // Devolve true se trocou de estado.
    bool transicoesDeCombate(Jogador& j, Mundo& m, const ComandosJogador& cmd);
};

class EstadoOcioso final : public EstadoLocomocao {
public:
    const char* nome() const override { return "Ocioso"; }
    void entrar(Jogador& j, Mundo& m) override;
    void atualizar(Jogador& j, Mundo& m, const ComandosJogador& cmd, float dt) override;
};

class EstadoMovendo final : public EstadoLocomocao {
public:
    const char* nome() const override { return "Movendo"; }
    void entrar(Jogador& j, Mundo& m) override;
    void atualizar(Jogador& j, Mundo& m, const ComandosJogador& cmd, float dt) override;
};

// Botão esquerdo: combo de 3 golpes. Cada acerto drena essência -> Mana.
class EstadoAtaqueLamina final : public EstadoJogador {
public:
    explicit EstadoAtaqueLamina(int indice) : indice_(indice) {}
    const char* nome() const override { return "AtaqueLamina"; }
    void entrar(Jogador& j, Mundo& m) override;
    void atualizar(Jogador& j, Mundo& m, const ComandosJogador& cmd, float dt) override;
    bool interrompivelPorDano() const override { return indice_ < 2; }  // o 3º golpe tem super armadura

private:
    int indice_;               // 0, 1, 2
    float t_ = 0.f, duracao_ = 0.3f;
    float multDano_ = 1.f, multVel_ = 1.f;
    bool acertou_ = false;     // o golpe já foi executado
    bool proximo_ = false;     // clique guardado (buffer) para emendar
    Vector2 dirGolpe_{};
};

// Botão direito: mira com o cursor; segurar carrega um disparo perfurante.
class EstadoMirando final : public EstadoJogador {
public:
    const char* nome() const override { return "Mirando"; }
    void entrar(Jogador& j, Mundo& m) override;
    void sair(Jogador& j, Mundo& m) override;
    void atualizar(Jogador& j, Mundo& m, const ComandosJogador& cmd, float dt) override;

private:
    float carga_ = 0.f;
    unsigned vfxCarga_ = 0;
    bool avisouCarga_ = false;
};

// 1/2/3/Q: conjuração curta; ao liberar, ativa a Sede de Sangue.
class EstadoConjurando final : public EstadoJogador {
public:
    EstadoConjurando(int slot, Vector2 alvo) : slot_(slot), alvo_(alvo) {}
    const char* nome() const override { return "Conjurando"; }
    void entrar(Jogador& j, Mundo& m) override;
    void atualizar(Jogador& j, Mundo& m, const ComandosJogador& cmd, float dt) override;

private:
    int slot_;
    Vector2 alvo_;
    float t_ = 0.f;
    bool lancou_ = false;
};

// Espaço/Shift: impulso com i-frames, deixando afterimages e partículas.
class EstadoEsquiva final : public EstadoJogador {
public:
    explicit EstadoEsquiva(Vector2 direcao) : dir_(direcao) {}
    const char* nome() const override { return "Esquiva"; }
    void entrar(Jogador& j, Mundo& m) override;
    void atualizar(Jogador& j, Mundo& m, const ComandosJogador& cmd, float dt) override;
    bool interrompivelPorDano() const override { return false; }

private:
    Vector2 dir_;
    float t_ = 0.f;
    float proximaImagem_ = 0.f;
};

class EstadoAtordoado final : public EstadoJogador {
public:
    explicit EstadoAtordoado(float duracao) : duracao_(duracao) {}
    const char* nome() const override { return "Atordoado"; }
    void entrar(Jogador& j, Mundo& m) override;
    void atualizar(Jogador& j, Mundo& m, const ComandosJogador& cmd, float dt) override;

private:
    float duracao_, t_ = 0.f;
};

class EstadoMorto final : public EstadoJogador {
public:
    const char* nome() const override { return "Morto"; }
    void entrar(Jogador& j, Mundo& m) override;
    void atualizar(Jogador& j, Mundo& m, const ComandosJogador& cmd, float dt) override;
    bool interrompivelPorDano() const override { return false; }

private:
    float t_ = 0.f;
    bool avisou_ = false;
};

} // namespace andv

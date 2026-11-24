#pragma once
// PlayerStateMachine — máquina de estados do protagonista.
//
//                 ┌──────────── Esquiva (i-frames) ◄─────────────┐ Espaço/Shift (de quase todos)
//                 │                                              │
//   Ocioso ◄──► Movendo ──LMB──► AtaqueLamina(1)──►(2)──►(3)     │
//      ▲           │                   │ (cancela na recuperação)│
//      │           ├──RMB segurado──► Mirando ──solta──► dispara │
//      │           ├──1/2/3/Q──────► Conjurando ──► Sede de Sangue
//      │           └──dano─────────► Atordoado
//      └──────────────────────────── (fim de cada estado)
//                                    Morto (vida = 0)
//
// Cada estado é uma classe (std::unique_ptr), com entrar/atualizar/sair. As
// trocas ficam pendentes e são aplicadas no fim do update, então um estado
// nunca é destruído enquanto ainda executa o próprio código.
#include "andvarion/core/Entrada.hpp"

#include <memory>

namespace andv {

class Jogador;
class Mundo;

class EstadoJogador {
public:
    virtual ~EstadoJogador() = default;
    virtual const char* nome() const = 0;
    virtual void entrar(Jogador&, Mundo&) {}
    virtual void sair(Jogador&, Mundo&) {}
    virtual void atualizar(Jogador& j, Mundo& m, const ComandosJogador& cmd, float dt) = 0;
    // Pode ser interrompido por dano (atordoamento)? Ataques pesados têm "super armadura".
    virtual bool interrompivelPorDano() const { return true; }
};

class PlayerStateMachine {
public:
    void iniciar(Jogador& j, Mundo& m, std::unique_ptr<EstadoJogador> inicial);
    void trocar(std::unique_ptr<EstadoJogador> proximo) { pendente_ = std::move(proximo); }
    void atualizar(Jogador& j, Mundo& m, const ComandosJogador& cmd, float dt);
    // Aplica a troca pendente imediatamente (usado por eventos externos, ex.: dano).
    void aplicarPendente(Jogador& j, Mundo& m);

    const EstadoJogador* atual() const { return atual_.get(); }
    const char* nomeAtual() const { return atual_ ? atual_->nome() : "-"; }
    template <class E> bool em() const { return dynamic_cast<const E*>(atual_.get()) != nullptr; }

private:
    std::unique_ptr<EstadoJogador> atual_;
    std::unique_ptr<EstadoJogador> pendente_;
};

} // namespace andv

#include "andvarion/combate/PlayerStateMachine.hpp"

namespace andv {

void PlayerStateMachine::iniciar(Jogador& j, Mundo& m, std::unique_ptr<EstadoJogador> inicial) {
    if (atual_) atual_->sair(j, m);
    pendente_.reset();
    atual_ = std::move(inicial);
    atual_->entrar(j, m);
}

void PlayerStateMachine::aplicarPendente(Jogador& j, Mundo& m) {
    // Um estado pode pedir outra troca já no entrar() (ex.: ataque sem arma).
    for (int guarda = 0; pendente_ && guarda < 4; ++guarda) {
        if (atual_) atual_->sair(j, m);
        atual_ = std::move(pendente_);
        atual_->entrar(j, m);
    }
}

void PlayerStateMachine::atualizar(Jogador& j, Mundo& m, const ComandosJogador& cmd, float dt) {
    if (atual_) atual_->atualizar(j, m, cmd, dt);
    aplicarPendente(j, m);
}

} // namespace andv

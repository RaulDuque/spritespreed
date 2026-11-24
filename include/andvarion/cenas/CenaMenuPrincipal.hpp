#pragma once
// CenaMenuPrincipal — "Novo Jogo", "Continuar" (save JSON) e "Sair".
#include "andvarion/core/Entrada.hpp"
#include "andvarion/core/Jogo.hpp"

#include <string>

namespace andv {

class CenaMenuPrincipal final : public Cena {
public:
    explicit CenaMenuPrincipal(Jogo& jogo);
    void lerEntrada() override;
    void atualizar(float dt) override;
    void desenhar() override;

private:
    void escolher(int i);
    Rectangle retanguloOpcao(int i) const;

    ComandosJogador cmd_;
    int selecionada_ = 0;
    Vector2 mouseAnterior_{-1.f, -1.f};
    bool temSave_ = false;
    float tempo_ = 0.f;
    std::string mensagem_;
    float tempoMensagem_ = 0.f;
};

} // namespace andv

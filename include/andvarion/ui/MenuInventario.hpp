#pragma once
// MenuInventario — tecla I. Abas (Equipamentos, Consumíveis, Materiais, Missão),
// lista de itens, painel de slots e PRÉVIA ANIMADA do personagem: equipar um
// item troca a camada do paper-doll na hora, na prévia e no mapa.
#include "andvarion/core/Entrada.hpp"
#include "andvarion/progressao/Inventario.hpp"

#include <raylib.h>

#include <functional>
#include <string>
#include <vector>

namespace andv {

class Fonte;
class Jogador;
struct Sessao;

class MenuInventario {
public:
    // Chamado quando o equipamento muda (a cena reaplica o paper-doll no jogador).
    using AoMudar = std::function<void()>;

    void abrir() { aberto_ = true; selecionado_ = 0; }
    void fechar() { aberto_ = false; }
    bool aberto() const { return aberto_; }
    void definirCallback(AoMudar cb) { aoMudar_ = std::move(cb); }

    void atualizar(float dt, const ComandosJogador& cmd, Sessao& s);
    void desenhar(const Fonte& fonte, const Sessao& s, const Jogador& j) const;

private:
    std::vector<std::string> itensDaAba(const Sessao& s) const;
    Rectangle retanguloItem(int i) const;
    Rectangle retanguloAba(int i) const;
    void acionar(const std::string& id, Sessao& s);

    bool aberto_ = false;
    int aba_ = 0;
    int selecionado_ = 0;
    float tempo_ = 0.f;
    std::string mensagem_;
    float tempoMensagem_ = 0.f;
    AoMudar aoMudar_;
};

} // namespace andv

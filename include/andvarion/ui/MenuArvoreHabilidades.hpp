#pragma once
// MenuArvoreHabilidades — tecla K. Uma ÁRVORE LITERAL:
//
//                 galhos de magia em leque (Mago)
//        Guerreiro  \  |  /  Arqueiro/Explorador
//             \      \ | /      /
//              \______\|/______/
//                      |   tronco
//
// Cada galho é uma magia ou um conjunto de atributos; os nós ao longo dele são
// as melhorias, na ordem. Clique desbloqueia (toca o VFX do pixelfx no nó).
// Com o mouse sobre uma magia aprendida, 1/2/3/Q a colocam no slot do grimório.
// Aba "Proficiências": nível de cada arma e escola de magia.
#include "andvarion/core/Entrada.hpp"

#include <raylib.h>

#include <functional>
#include <string>
#include <vector>

namespace andv {

class Fonte;
class VFXManager;
struct Sessao;
struct NoHabilidade;
class ArvoreHabilidades;

class MenuArvoreHabilidades {
public:
    using AoDesbloquear = std::function<void(const NoHabilidade&)>;

    void abrir() { aberto_ = true; aba_ = 0; mensagem_.clear(); }
    void fechar() { aberto_ = false; }
    bool aberto() const { return aberto_; }
    void definirCallback(AoDesbloquear cb) { aoDesbloquear_ = std::move(cb); }

    void atualizar(float dt, const ComandosJogador& cmd, Sessao& s);
    void desenhar(const Fonte& fonte, const Sessao& s, const VFXManager& vfx) const;

    // Geometria (pública para os testes de layout não precisarem de janela).
    static Vector2 pontaDoRamo(const std::string& ramo);
    static Vector2 posicaoNo(const NoHabilidade& n, const ArvoreHabilidades& a);

private:
    void desenharArvore(const Fonte& fonte, const Sessao& s, const VFXManager& vfx) const;
    void desenharProficiencias(const Fonte& fonte, const Sessao& s) const;
    void desenharGrimorio(const Fonte& fonte, const Sessao& s, const VFXManager& vfx) const;

    bool aberto_ = false;
    int aba_ = 0;                // 0 = árvore, 1 = proficiências
    int focado_ = -1;
    std::string mensagem_;
    float tempoMensagem_ = 0.f;
    float tempo_ = 0.f;
    struct Explosao { Vector2 pos; float t; std::string vfx; };
    std::vector<Explosao> explosoes_;
    AoDesbloquear aoDesbloquear_;
};

} // namespace andv

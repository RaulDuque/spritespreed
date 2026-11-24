#include "andvarion/core/Roteiro.hpp"
#include "andvarion/core/Texto.hpp"

#include <raymath.h>

#include <algorithm>
#include <sstream>

namespace andv {

namespace {
Roteiro* g_roteiro = nullptr;
}

Roteiro* roteiroAtivo() { return g_roteiro; }
void definirRoteiroAtivo(Roteiro* r) { g_roteiro = r; }

bool Roteiro::carregar(const std::string& caminho) {
    const auto conteudo = texto::lerArquivo(caminho);
    if (!conteudo) return false;
    std::istringstream ss(*conteudo);
    std::string linha;
    while (std::getline(ss, linha)) {
        if (auto p = linha.find('#'); p != std::string::npos) linha.resize(p);
        std::istringstream ls(linha);
        Passo p;
        if (!(ls >> p.t >> p.cmd)) continue;
        std::string a;
        while (ls >> a) p.args.push_back(a);
        passos_.push_back(std::move(p));
    }
    std::stable_sort(passos_.begin(), passos_.end(), [](const Passo& a, const Passo& b) { return a.t < b.t; });
    ativo_ = !passos_.empty();
    return ativo_;
}

void Roteiro::avancar(float dt) {
    tempo_ += dt;
    teclas_.clear();
    clique_ = direitoInicio_ = direitoFim_ = false;
    if (direitoAte_ >= 0.f && tempo_ >= direitoAte_) {
        direitoFim_ = true;
        direitoAte_ = -1.f;
    }
    if (moverAte_ >= 0.f && tempo_ >= moverAte_) {
        mover_ = {0, 0};
        moverAte_ = -1.f;
    }
    while (proximo_ < passos_.size() && passos_[proximo_].t <= tempo_) {
        const Passo& p = passos_[proximo_++];
        auto arg = [&](std::size_t i, float padrao) { return i < p.args.size() ? std::stof(p.args[i]) : padrao; };
        if (p.cmd == "tecla" && !p.args.empty()) teclas_.push_back(p.args[0]);
        else if (p.cmd == "clique") clique_ = true;
        else if (p.cmd == "mira") { mouse_ = {arg(0, 240), arg(1, 135)}; temMouse_ = true; }
        else if (p.cmd == "mover") {
            const Vector2 d{arg(0, 0), arg(1, 0)};
            mover_ = Vector2Length(d) > 0 ? Vector2Normalize(d) : d;
            moverAte_ = tempo_ + arg(2, 1.f);
        } else if (p.cmd == "direito") {
            direitoInicio_ = true;
            direitoAte_ = tempo_ + arg(0, 0.1f);
        } else if (p.cmd == "captura" && !p.args.empty()) capturas.push_back(p.args[0]);
        else if (p.cmd == "sair") pediuSaida = true;
    }
}

void Roteiro::aplicar(ComandosJogador& c) const {
    if (Vector2Length(mover_) > 0.f) c.mover = mover_;
    for (const auto& t : teclas_) {
        if (t == "enter") c.confirmar = true;
        else if (t == "e") { c.interagir = true; c.confirmar = true; }
        else if (t == "espaco") { c.esquiva = true; c.confirmar = true; }
        else if (t == "esc") { c.pausar = true; c.cancelar = true; }
        else if (t == "k") c.arvore = true;
        else if (t == "i") c.inventario = true;
        else if (t == "a") c.esquerda = true;
        else if (t == "d") c.direita = true;
        else if (t == "f") c.frasco = true;
        else if (t == "r") c.trocarDistancia = true;
        else if (t == "tab") c.trocarCorpo = 1;
        else if (t == "1") c.feitico[0] = true;
        else if (t == "2") c.feitico[1] = true;
        else if (t == "3") c.feitico[2] = true;
        else if (t == "q") c.feitico[3] = true;
        else if (t == "w") c.cima = true;
        else if (t == "s") c.baixo = true;
    }
    if (clique_) { c.laminaPressionado = true; c.clique = true; }
    if (direitoInicio_) c.distanciaPressionado = true;
    if (direitoAte_ >= 0.f || direitoInicio_) c.distanciaSegurando = !direitoFim_;
    if (direitoFim_) { c.distanciaSoltou = true; c.distanciaSegurando = false; }
}

} // namespace andv

#pragma once
// Notificacoes — avisos curtos empilhados ("Missão aceita", "+1 Erva Pálida").
// É um Observer do EventBus: ouve missões, níveis e habilidades por conta própria.
#include "andvarion/core/EventBus.hpp"

#include <raylib.h>

#include <deque>
#include <functional>
#include <string>
#include <vector>

namespace andv {

class Fonte;

class Notificacoes {
public:
    using NomeMissao = std::function<std::string(const std::string&)>;
    Notificacoes(EventBus& bus, NomeMissao nomeMissao);

    void mostrar(const std::string& texto, Color cor, float duracao = 3.f);
    void atualizar(float dt);
    void desenhar(const Fonte& fonte) const;

private:
    struct Aviso {
        std::string texto;
        Color cor;
        float restante, total;
    };
    std::deque<Aviso> avisos_;
    std::vector<Inscricao> inscricoes_;
};

} // namespace andv

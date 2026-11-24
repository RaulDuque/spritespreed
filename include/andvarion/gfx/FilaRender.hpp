#pragma once
// FilaRender — ordenação de profundidade dinâmica (Y-Sorting).
//
// Tudo que "fica em pé" no mundo (personagens, NPCs, pilares, lápides, árvores)
// entra na fila com o Y dos pés; a fila desenha do menor Y para o maior. Quem
// está mais abaixo na tela fica na frente. Empates mantêm a ordem de inserção.
#include <functional>
#include <vector>

namespace andv {

class FilaRender {
public:
    void adicionar(float y, std::function<void()> desenhar) { itens_.push_back({y, ordem_++, std::move(desenhar)}); }
    void desenharTudo();  // ordena, desenha e esvazia
    std::size_t tamanho() const { return itens_.size(); }

private:
    struct Item {
        float y;
        unsigned ordem;
        std::function<void()> desenhar;
    };
    std::vector<Item> itens_;
    unsigned ordem_ = 0;
};

} // namespace andv

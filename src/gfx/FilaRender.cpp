#include "andvarion/gfx/FilaRender.hpp"

#include <algorithm>

namespace andv {

void FilaRender::desenharTudo() {
    std::sort(itens_.begin(), itens_.end(), [](const Item& a, const Item& b) {
        return a.y != b.y ? a.y < b.y : a.ordem < b.ordem;
    });
    for (auto& i : itens_) i.desenhar();
    itens_.clear();
    ordem_ = 0;
}

} // namespace andv

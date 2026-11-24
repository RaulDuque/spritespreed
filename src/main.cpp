// =============================================================================
//  ANDVARION — O Sussurro sob a Catedral
//  Action RPG gótico/medieval top-down em pixel art. C++17 + raylib, alvo x86.
//
//  Ponto de entrada: cria o Jogo (janela, assets, VFX do pixelfx, fonte, save)
//  e roda o laço. Toda a lógica vive nas cenas (src/cenas).
// =============================================================================
#include "andvarion/core/Jogo.hpp"
#include "andvarion/core/Plataforma.hpp"

#include <cstdio>
#include <exception>

int main() {
    std::printf("Andvarion %s (%d bits)\n", "0.1.0", static_cast<int>(sizeof(void*) * 8));
    try {
        andv::Jogo jogo;
        return jogo.executar();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Erro fatal: %s\n", e.what());
        return 1;
    }
}

# Andvarion — O Sussurro sob a Catedral

Action RPG gótico/medieval top-down em pixel art, inspirado em Dungeons & Dragons
e Bloodborne. C++17 + raylib 5.0, **compilado obrigatoriamente para x86 (32 bits)**.
Todo o jogo é em português.

* **Ciclo de Sinergia Sangue-Arcano**: corte para drenar mana, conjure para ativar a *Sede de Sangue*, finalize à distância.
* Arsenal: Espada, Adagas, Machado, Foice e Bastão; Arco, Besta e Bumerangue (vai e volta).
* **Equipamento em camadas (paper-doll)**: Sobretudo Gótico, Armadura de Cavaleiro, Túnica de Mago, Capa Carmesim e Escudo do Leão mudam o visual do herói em todas as animações.
* **Grimório com 12 magias** (Orbe de Sangue, Névoa Cortante, Correntes Sombrias, Sangria Vital, Bola de Fogo, Lança de Gelo, Relâmpago em Cadeia, Escudo Arcano, Drenar Vida, Invocação de Familiar, Teleporte Curto, Encantamento de Lâmina).
* **Árvore de Habilidades literal**: tronco, ramos Guerreiro/Arqueiro/Mago e um galho por magia com as melhorias dela.
* **Proficiência**: cada arma e escola de magia sobe de nível com o uso.
* Vila de Cinzavale (área segura) e Ruínas da Catedral (cultistas e carniçais com IA).
* Diálogos com retrato, máquina de escrever e escolhas; missões com rastreador.
* Noite azulada com luzes dinâmicas (lanterna na cintura, tochas oscilantes), Y-sort e pós-processamento.
* Animação de personagens no formato do **agent-sprite-forge**; todos os efeitos vêm do **pixelfx**.

## Compilar (32 bits)

Requisitos: CMake ≥ 3.21, um compilador com alvo x86 e internet na primeira
configuração (baixa raylib 5.0 e nlohmann/json, com hash fixado).

| Plataforma | Comandos |
|---|---|
| Windows + Visual Studio 2022 | `cmake --preset win32-msvc` → `cmake --build --preset win32-msvc-release` |
| Windows + MSYS2 MINGW32 / Linux → Windows | `cmake --preset mingw-i686` → `cmake --build --preset mingw-i686-release` |
| Linux x86 | `cmake --preset linux-i686` → `cmake --build --preset linux-i686-release` |

* MSYS2: `pacman -S mingw-w64-i686-toolchain mingw-w64-i686-cmake mingw-w64-i686-ninja`.
* Linux: `sudo dpkg --add-architecture i386` e `sudo apt install gcc-multilib g++-multilib ninja-build libx11-dev:i386 libxrandr-dev:i386 libxinerama-dev:i386 libxcursor-dev:i386 libxi-dev:i386 libgl-dev:i386`.

Sem preset (ou com um compilador de 64 bits), a configuração **para** com uma
mensagem explicando o que usar. O código tem um `static_assert(sizeof(void*) == 4)`.

O executável fica em `build/<preset>/<Config>/` (ou `build/<preset>/`) com a
pasta `assets/` copiada ao lado. O save vai para `saves/save1.json` ao lado do executável.

### Pacote para testar no Windows
```bash
cmake -S . -B build/win -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-i686.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build/win -j
python3 tools/empacotar_windows.py build/win        # -> dist/Andvarion-Win32/ e dist/Andvarion-Win32.zip
```
No Windows, extraia o zip e abra `TESTAR_NO_WINDOWS.bat` (jogar, testes automáticos com capturas, testes de lógica).
`ANDV_SAVE=<arquivo>` troca o arquivo de save (os testes automáticos usam um save próprio).

### Testes
```bash
ctest --preset linux-i686          # lógica pura: missões, diálogos, árvore, save, atlas...
```
Teste de fumaça com janela (entrada roteirizada + capturas de tela):
```bash
ANDV_ROTEIRO=tools/roteiros/fumaca.txt ./andvarion
ANDV_ROTEIRO=tools/roteiros/equipamento.txt ./andvarion   # inventário e troca visual
```

## Controles

| Entrada | Ação |
|---|---|
| WASD / setas | mover |
| Botão esquerdo | **Lâmina**: combo de 3 golpes; cada acerto drena essência → Mana |
| Botão direito | **Distância**: mira com o cursor (Vigor); segure para o disparo carregado perfurante |
| 1, 2, 3, Q | **Grimório**: as 4 magias nos slots (troque na árvore: passe o mouse numa magia aprendida e aperte 1/2/3/Q) |
| Espaço / Shift | **Esquiva Sombria**: dash com i-frames e afterimages |
| E | conversar / interagir |
| F | Frasco de Sangue (cura) |
| Roda do mouse / Tab | trocar arma corpo a corpo |
| R | trocar arma à distância |
| K | Árvore de Habilidades (árvore literal: um galho por magia) e aba de Proficiências |
| I | Inventário: equipar armas, armaduras, capa e escudo (o visual muda na hora) |
| Esc / P | pausa (salvar, árvore, voltar ao menu) |

## Missões

* **O Sussurro sob a Catedral** (principal, Vigia Aldric): fale com a Curandeira, investigue o altar profanado, derrote 5 cultistas e reporte.
* **Ferro Amaldiçoado** (Ferreiro Bram): 3 fragmentos que caem dos carniçais. Recompensa: lâmina reforjada (+20% de dano).
* **Cinzas da Memória** (Irmã Mirela, a Curandeira): 4 ervas pálidas nas ruínas. Recompensa: +2 Frascos de Sangue.

## Estrutura

```
CMakeLists.txt  CMakePresets.json
cmake/            Alvo32Bits.cmake, toolchains/ (mingw-i686, linux-i686)
include/andvarion/   headers (core, gfx, mundo, entidades, combate, progressao, missoes, dialogo, ui, save, cenas)
src/                 implementação (mesma divisão)
tests/testes.cpp     testes da lógica pura (andv_nucleo)
assets/
  sprites/<personagem>/   atlas Aseprite no formato agent-sprite-forge
  vfx/                    sprite sheets do pixelfx + indice.json
  mapas/                  áreas (matriz + legenda JSON)
  dados/                  missões, diálogos, árvore, itens
  retratos/ tilesets/ shaders/ fontes/
tools/
  gerar_vfx.mjs + vfx_receitas.json   pixelfx → assets/vfx
  gerar_placeholders.py               arte provisória no formato da forge
  roteiros/                           testes de fumaça
third_party/pixelfx/                  pixelfx 0.3.1 (MIT)
docs/ARQUITETURA.md  docs/PIPELINE_ARTE.md
```

Diagramas e fluxo detalhado: [docs/ARQUITETURA.md](docs/ARQUITETURA.md).
Como gerar e substituir a arte: [docs/PIPELINE_ARTE.md](docs/PIPELINE_ARTE.md).

## Licenças de terceiros
* raylib (zlib), nlohmann/json (MIT): baixadas no build.
* pixelfx (MIT): `third_party/pixelfx/LICENSE`. Os sheets em `assets/vfx` são renderizados por ele.
* Formato de exportação do agent-sprite-forge (MIT).

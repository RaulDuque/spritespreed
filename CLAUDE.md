# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Andvarion: a gothic top-down action RPG in C++17 + raylib 5.0. **The build target must stay x86 (32-bit)**: this is a hard project requirement, enforced by `cmake/Alvo32Bits.cmake` (configure fails when `CMAKE_SIZEOF_VOID_P != 4`) and by a `static_assert` in `include/andvarion/core/Plataforma.hpp`. All player-facing text, identifiers and comments are in Brazilian Portuguese; keep new code and data in PT-BR.

## Build, test, run

```bash
cmake --preset linux-i686            # or win32-msvc / mingw-i686
cmake --build --preset linux-i686-release
ctest --preset linux-i686            # logic tests (no window)
./build/linux-i686/Release/andv_testes missoes   # run one test group (substring filter)
ANDV_ROTEIRO=tools/roteiros/fumaca.txt ./build/linux-i686/Release/andvarion   # scripted smoke test w/ screenshots (also equipamento.txt)
```

* Dependencies (raylib 5.0, nlohmann/json 3.11.3) come via `FetchContent` with pinned hashes. `CMAKE_POLICY_VERSION_MINIMUM 3.5` is set because raylib 5.0's CMakeLists is rejected by CMake 4.x.
* Linux 32-bit needs gcc-multilib plus `:i386` X11/GL dev packages (see README). Do not "fix" a 64-bit configure failure by weakening the guard; for a host-only sanity build, define `ANDV_VERIFICACAO_HOST` and drop `-m32` in a scratch copy, never in the repo.
* `assets/` is copied next to the executable on every build; the game also falls back to `./assets/`.
* Regenerate effects: `node tools/gerar_vfx.mjs [chave...]` (or the `vfx` CMake target). Regenerate placeholder sprites/tileset/portraits: `python3 tools/gerar_placeholders.py`.

## Architecture (big picture)

* **Two layers.** `andv_nucleo` (static lib, no raylib) holds the pure logic: `EventBus`, `QuestManager`, `DialogoDados`, `ArvoreHabilidades`, `Progressao`, `Inventario`, `SaveSystem`, `AtlasAseprite`, weapon/spell tables (`combate/Armas`), `Recursos`/`SedeDeSangue`. Only this layer is unit-tested (`tests/testes.cpp`), and it must not include raylib. Everything else (rendering, entities, scenes) lives in the `andvarion` executable.
* **Loop** (`core/Jogo`): fixed 60 Hz update with accumulator; render into a 640x360 `RenderTexture`, then blit at an integer scale through `assets/shaders/atmosfera.fs`. `Cena::lerEntrada` runs once per render frame and accumulates edge-triggered inputs; `ComandosJogador::consumirBordas` clears them after the first fixed step. raylib render targets cannot nest, so any offscreen pass (the lightmap) goes in `Cena::prepararQuadro`, which runs before the virtual canvas is bound.
* **Scenes**: `CenaMenuPrincipal` → `CenaJogo` (modes: Jogando/Dialogo/Arvore/Pausa/Morto/Transicao). `CenaJogo` owns `Sessao` (persistent state that goes into the save), `Jogador` (survives area changes) and `Mundo` (the loaded area: `MapaTiles` + entities). `Mundo` is the context passed to entities (map collision, VFX, lights, bus, target queries, hit-stop). Pointers into `MapaTiles::objetos()` are invalidated by `carregarArea`.
* **Player combat** is a state machine (`combate/PlayerStateMachine` + `EstadosJogador`): states are `unique_ptr` classes, and transitions are deferred to the end of the update (`aplicarPendente` for external events like damage). Actions such as `executarGolpe`, `disparar`, `lancarFeitico` and `iniciarEsquiva` live on `Jogador`. The synergy loop: blade hits add mana, spending mana arms `SedeDeSangue`, and the next swing or shot reads `multiplicadorVelocidade()` and then `consumir()`.
* **Observer**: gameplay publishes `InimigoMorto` / `ItemColetado` / `NpcFalado` on `EventBus`. `QuestManager` updates only ACTIVE quests; `CenaJogo` handles `MissaoConcluida` (rewards and autosave), and `Notificacoes` shows toasts. Subscriptions are RAII `Inscricao` objects, so keep them as members of the observer.
* **Rendering order** in an area: lightmap (`gfx/Iluminacao`, subtractive custom blend) → ground/walls → `Chao` VFX layer → Y-sorted queue (`gfx/FilaRender`: tall decoration tiles, entities, player) → `Frente` VFX → lightmap overlay + additive glow → UI.

* **Scale** (`core/Escala.hpp`): game logic is in world units (1 tile = 16 units, unchanged). The world camera has zoom 2, and art is authored at double resolution (32 px tiles via `tamanhoTileArte`; characters ~55 px in 48x64 frames, anchor (24, 62)) and painted at `ESCALA_DESENHO` (0.5), so 1 art px = 1 screen px. Snap draw positions with `snapTela`. Text drawn inside `BeginMode2D` doubles in size, so put labels in screen space.
* **Paper-doll / equipment**: `Equipamento` (pure, in `andv_nucleo`) maps slots (arma, distancia, armadura, capa, escudo) to inventory item ids; `itens.json` gives each item `slot`, `arma`, `atributos` and `visual` (atlas folder). `Jogador::aplicarEquipamento` turns the slots into weapon stats and `PaperDoll` layers. Only the body layer is animated; every other layer draws the same clip name and frame index from its own atlas. The weapon layer shows the ranged weapon while aiming. Inventory UI: `ui/MenuInventario` (key I).

* **Grimoire, literal skill tree, proficiency**: `combate/Armas.cpp → feiticos()` lists all 12 spells (each with a school). `Sessao::grimorio` holds the 4 slotted spell ids (keys 1/2/3/Q). Cooldowns are per spell id (`Jogador::recargas_`), so re-slotting doesn't reset them. Spell behaviour lives in `Jogador::lancarFeitico` (branch on id). `arvore_habilidades.json` has `galhos` (branch: ramo guerreiro/arqueiro/mago, escola, angle, length) and `nos` (galho + ordem); a node implicitly requires the previous node of its galho. Spell upgrades are attribute nodes keyed `<spellId>.<stat>` (`dano`, `recarga`, `raio`, `alvos`, `duracao`...), read through `Jogador::bonusFeitico`. The tree UI (`ui/MenuArvoreHabilidades`) computes node positions from the branch geometry (`posicaoNo`); keep branch angles/lengths so nodes stay ≥ ~16 px apart. `Proficiencia` (pure): weapon ids and school ids level 1–10 with use (hits / casts), giving +4% damage per level (schools also −2% mana).

## Art pipelines (both produce Aseprite-style JSON atlases read by `AtlasAseprite`)

* **Characters: agent-sprite-forge** `export_engine.py --target aseprite-json` output goes in `assets/sprites/<personagem>/` (every `.json` in the folder except `engine-export.json` is one page). Clip names follow `<acao>_<direcao>` (PT-BR or English synonyms; see `docs/PIPELINE_ARTE.md`). Left-facing clips may be omitted, because `AnimationController` mirrors right-facing ones around the anchor. Tag `repeat:"1"` means one-shot; tag `data.events` (`hit`, `release`, `step_l`...) drive gameplay timing; the `anchor` slice is the feet pivot. Walking advances by distance travelled, not by time.
* **Effects: pixelfx** (vendored at `third_party/pixelfx`). For UI icons use `VFXManager::desenharIcone` (it centres and scales a frame of the effect, whatever its pivot). `tools/vfx_receitas.json` maps a C++ key to an effect id plus options (palette by `PAL` name). The generated `assets/vfx/indice.json` is what `VFXManager` loads. pixelfx effects point right; use `ParametrosVfx::orientar` for aimed effects. `meta.pixelfx.act.hits[].freeze` feeds hit-stop. The pixelfx bitmap font is ASCII-only.
* Current sprites, tileset and portraits are **code-drawn placeholders** from `tools/gerar_placeholders.py` (already in the forge format, one atlas per paper-doll layer under `assets/sprites/equip/<item>/`). The VFX are real pixelfx renders. `testeCamadasPaperDoll` fails if any equipment layer's clips, frame counts, durations, frame size or anchor diverge from the body atlas.
* No image-generation route is configured on this machine (`forge_doctor.py`: no API keys, no Codex/Grok CLI, no ffmpeg/numpy/Pillow). Never run paid generation without the user's go-ahead.

## Data-driven content (`assets/`)

Maps (`mapas/*.json`: char matrix + legend, objects in tile coords), quests, dialogue graphs with conditions/actions, skill tree and item catalog are JSON; see the table in `docs/ARQUITETURA.md`. `BancoDialogos::carregar` rejects dangling node references, and `testeDialogos` / `testeArvore` cross-check the shipped data, so run `ctest` after editing data files. New enemy types are added in `definicaoInimigo()` (`src/entidades/Inimigo.cpp`).

## Windows package

`python3 tools/empacotar_windows.py <build-dir>` builds `dist/Andvarion-Win32/` plus a `.zip`. It copies the exes (and refuses anything that isn't PE32 i386), `assets/`, the smoke scripts, `tools/windows/TESTAR_NO_WINDOWS.bat` and `LEIA-ME.txt`, converting the text files to CRLF. The MinGW toolchain file also accepts the Debian `-posix`/`-win32` compiler names. File paths must use the **native narrow encoding** (`std::filesystem::path(std::string)`), never `u8path`: raylib returns ANSI-codepage paths on Windows, and pt-BR desktops live under "Área de Trabalho". `ANDV_SAVE` overrides the save file.

## Prompt and report log (required on every prompt)

The user wants every prompt and every task report kept in the repo:

* `docs/prompts/NNN-tema-curto.md`: the user's prompt **verbatim** (typos included), with a header (`numero`, `data`, `tema`, link to the report). Add a row to `docs/prompts/README.md`.
* `docs/relatorios/NNN-tema-curto.md`: the report for that prompt, in PT-BR (what was asked, what was done, decisions, verification, bugs, pending items). Add a row to `docs/relatorios/README.md`.
* Use the next number after the highest existing one and the same `NNN-tema` for both files. Do this for every prompt, including small ones, and finish it before the final reply.

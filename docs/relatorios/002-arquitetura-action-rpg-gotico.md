---
numero: 002
data: 2026-11-24
prompt: ../prompts/002-arquitetura-action-rpg-gotico.md
status: concluído — falta compilar o binário 32 bits real numa máquina com toolchain x86
---

# Relatório 002 — Arquitetura e código do Action RPG gótico

## Resumo
Projeto criado do zero: C++17 + **raylib 5.0**, alvo obrigatório **x86 (32 bits)**,
100% PT-BR, ~8.000 linhas em `src/` e `include/andvarion/`. O jogo roda e foi
testado na tela do menu até a entrega de missão, morte e despertar.

## Decisões
| Decisão | Motivo |
|---|---|
| raylib (não SFML) | Compila em 32 bits por FetchContent sem dependências binárias; shaders e render targets simples. |
| Tela virtual 480×270, escala inteira | Pixel art nítido em qualquer janela. |
| Lógica pura separada em `andv_nucleo` (sem raylib) | Missões, diálogos, árvore, save e parser de atlas testáveis sem janela. |
| Mapas em JSON (matriz + legenda), CSV também aceito | Pedido de "matriz/JSON/CSV"; legível e editável à mão. |
| Arte de personagens provisória desenhada por código | Gerar com o agent-sprite-forge usa API paga/CLI externo; não rodei sem autorização. |

## Entregas
- **Build 32 bits**: `CMakeLists.txt`, `CMakePresets.json` (`win32-msvc`, `mingw-i686`, `linux-i686`), `cmake/Alvo32Bits.cmake` (recusa configuração com ponteiro ≠ 4 bytes, aplica `-m32` / `/MACHINE:X86`), toolchains em `cmake/toolchains/`, `static_assert` em `Plataforma.hpp`.
- **agent-sprite-forge**: `AtlasAseprite` lê o export `--target aseprite-json` (tags, `repeat`, eventos `hit`/`release`, slice `anchor`, várias páginas). `AnimationController` usa clipes `<acao>_<direcao>` em 4/8 direções, espelha a esquerda, avança a caminhada pela distância e dispara eventos.
- **pixelfx**: vendorizado em `third_party/pixelfx`; `tools/gerar_vfx.mjs` + `tools/vfx_receitas.json` geraram **30 efeitos reais** em `assets/vfx`. `VFXManager` toca, orienta, segue entidades e lê o hit-stop do `act`.
- **Iluminação/Y-sort**: lightmap noturno azulado, lanterna quente na cintura, tochas oscilantes, flashes de magia, shader de vinheta/grão; `FilaRender` ordena pelo Y dos pés.
- **Combate**: `PlayerStateMachine` com Ocioso, Movendo, AtaqueLamina (combo 3), Mirando (carga perfurante), Conjurando, Esquiva (i-frames + afterimages), Atordoado e Morto; Sede de Sangue; 5 armas corpo a corpo, 3 à distância (bumerangue volta); 4 feitiços.
- **Progressão**: XP/níveis, árvore interativa com 19 nós em 3 ramos (`dados/arvore_habilidades.json`).
- **Mundo**: Vila de Cinzavale e Ruínas da Catedral, cultistas (distância) e carniçais (bote) com IA de patrulha/perseguição/telegrafia, itens, transições.
- **Diálogo e missões**: `DialogueSystem` (pergaminho, retrato, nome, máquina de escrever UTF-8, opções); `QuestManager` Observer com os 4 estados, rastreador no HUD; as 3 missões pedidas.
- **Save/Load JSON**, menu principal, pausa, tela de morte.
- **Docs**: `README.md`, `docs/ARQUITETURA.md` (diagramas Mermaid), `docs/PIPELINE_ARTE.md`, `CLAUDE.md`.
- **Testes**: `tests/testes.cpp` e modo de teste de fumaça `ANDV_ROTEIRO=<arquivo>` (entrada roteirizada + capturas).

## Verificação
| Teste | Resultado |
|---|---|
| Testes de lógica (com AddressSanitizer/UBSan) | 114 verificações, 0 falhas |
| Configuração 64 bits sem preset | Recusada com instruções (trava funciona) |
| CMake completo (cópia com trava desligada, host 64 bits) | FetchContent, build, cópia de assets, ctest e jogo OK |
| Teste de fumaça com janela | 14 capturas, 52–60 FPS |
| Combate, drop, coleta, entrega, nível, árvore, morte e despertar | Conferidos por capturas e pelo save |

**Não verificado**: binário 32 bits real. A máquina não tem libc i386 nem toolchain i686.

## Bugs encontrados e corrigidos
1. Tela preta: o lightmap estava aninhado dentro da tela virtual (render targets da raylib não aninham) → `Cena::prepararQuadro`.
2. Crash ao despertar: ponteiro para objeto do mapa antigo → zerado em `carregarArea`.
3. CMake 4.x recusa a raylib 5.0 → `CMAKE_POLICY_VERSION_MINIMUM 3.5`.
4. Torres do menu sumidas (ordem dos vértices) → `ui::triangulo`.
5. Ajustes visuais: aura e carga que deixavam o herói rosa, Névoa pouco visível, sobreposição de textos na árvore e no rastreador.

## Pendências e próximos passos
- Compilar com `cmake --preset win32-msvc` (ou `mingw-i686`/`linux-i686`) numa máquina com toolchain x86.
- Substituir a arte provisória: o personagem de referência em `docs/assets-user/` pode virar o master still no agent-sprite-forge (exige autorização, pode ter custo).
- O banner de nível do pixelfx não tem acento ("NOVO NIVEL!"): a fonte dele é só ASCII.

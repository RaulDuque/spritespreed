# Pipeline de arte

Dois pipelines alimentam o jogo, e os dois terminam no **mesmo formato**: atlas
JSON estilo Aseprite + PNG, lido por `AtlasAseprite` (`src/gfx/AtlasAseprite.cpp`).

| O quê | Ferramenta | Saída | Quem lê |
|---|---|---|---|
| Protagonista, NPCs, inimigos (movimento e animação) | [agent-sprite-forge](https://github.com/0x0funky/agent-sprite-forge) | `assets/sprites/<personagem>/*.json + .png` | `Assets::personagem` → `AnimationController` |
| Magias, golpes, impactos, partículas, tochas, UI | [pixelfx](https://github.com/patch-gg/pixelfx) (em `third_party/pixelfx`) | `assets/vfx/<chave>.json + .png` + `assets/vfx/indice.json` | `VFXManager` |

> Os sprites, o tileset e os retratos em `assets/` hoje são **provisórios,
> desenhados por código** (`tools/gerar_placeholders.py`), já no formato da forge,
> para o jogo rodar de ponta a ponta. Os VFX já são os efeitos reais do pixelfx.

---

## 1. Personagens com o agent-sprite-forge

### Gerar
Com as skills da forge (Claude Code ou Codex):

1. **Master still** do personagem (`generate2dsprite`): uma imagem aprovada por personagem, na escala do jogo (~32 px de altura para a tela 480×270), finalização *pixel*.
2. **Animações** (`video2dsprite` para movimento fluido, ou `generate2dsprite` para sheets): um clipe por ação e por direção.
3. **Clipes** (`build_animation_clips.py`) e **exportação**:

```bash
python "<skill-dir>/scripts/export_engine.py" \
    --clips out/heroi-clips/animation-clips.json \
    --target aseprite-json --sampling nearest \
    --output-dir out/heroi-engine
```

4. Copie `out/heroi-engine/aseprite/*` para `assets/sprites/protagonista/`.
   Várias páginas (`heroi-00.json`, `heroi-01.json`...) funcionam: o jogo lê
   todos os `.json` da pasta (exceto `engine-export.json`).

### Nomes dos clipes (contrato com o C++)

`<acao>_<direcao>` com direções em PT-BR **ou** inglês:

| Ação | Uso no jogo | Sinônimos aceitos |
|---|---|---|
| `ocioso` | parado | `idle` |
| `andar` | locomoção (avança pela **distância**, não pelo relógio) | `walk`, `correr`, `run` |
| `ataque1`, `ataque2`, `ataque3` | combo de Lâmina (evento `hit` = momento do acerto) | `attack1..3`, `ataque`, `attack` |
| `mirar` / `disparo` | arma à distância | `aim` / `shoot`, `ranged` |
| `conjurar` | magia (evento `release` = momento do lançamento) | `cast` |
| `esquiva` | dash | `dash`, `roll` |
| `dano` | atordoado | `hurt`, `hit` |
| `morte` | sem direção | `death`, `die` |

Direções: `baixo`, `baixo_direita`, `direita`, `cima_direita`, `cima`,
`cima_esquerda`, `esquerda`, `baixo_esquerda` (ou `down`, `down_right`,
`right`, `up_right`, `up`, `up_left`, `left`, `down_left`).

**4 ou 8 direções**: o `AnimationController` tenta a direção exata, depois a
cardinal mais próxima. Se **esquerda não existir, espelha a direita** em torno
do x da âncora. Ou seja, basta exportar `baixo`, `cima` e `direita`.

### O que o parser usa do export da forge
* `frames[].duration`: tempo de cada quadro (ms).
* `meta.frameTags[]`: um clipe por tag; `repeat: "1"` = one-shot; `data` (string JSON) com `events` (`hit`, `release`, `step_l`, `step_r`...) e `loop_policy`.
* `meta.slices["anchor"]`: pivô dos pés (`data.anchor_px` exato tem prioridade sobre o `pivot` inteiro).
* Pingpong/reverse do Aseprite são expandidos na linha do tempo.

### Camadas de equipamento (paper-doll)
O herói não é um sprite único: é o **corpo base** (`assets/sprites/protagonista/`)
mais uma camada por item equipado (`assets/sprites/equip/<item>/`, caminho no
campo `visual` de `dados/itens.json`). Para cada camada nova gerada na forge:

1. Use o **master still do corpo base** como referência e peça só a peça (armadura, capa, arma...) vestida nele, com fundo transparente.
2. Gere **os mesmos clipes, com a mesma contagem de quadros e as mesmas durações** do corpo (a lista está em `CLIPES_HEROI`, em `tools/gerar_placeholders.py`).
3. Exporte com o **mesmo tamanho de quadro e a mesma âncora** (hoje 48×64, pés em (24, 62)).
4. Rode `ctest`: `testeCamadasPaperDoll` reprova qualquer camada fora do contrato.

### Escala
A arte é feita em resolução dobrada: personagem humanoide com ~55 px de altura
num quadro 48×64, monstros grandes ~87×86, tiles de 32 px. As imagens de
`docs/assets-user/` são **inspiração visual** (estilo, paleta, contorno), não
câmera: o jogo é top-down, com as direções baixo, cima e direita (a esquerda é espelhada).

### Rotas de geração
O `forge_doctor.py` mostrou que **nenhuma rota está configurada** nesta máquina.
Para gerar arte de verdade, o dono do projeto configura uma das rotas abaixo
(as chaves ficam fora do repositório, em `~/.config/agent-sprite-forge/config.json`
ou em variável de ambiente; nunca no código nem no chat):

| Rota | Variável | Uso |
|---|---|---|
| Google Gemini | `GEMINI_API_KEY` | imagens (master stills, sheets); `--tier hero` para os masters |
| OpenAI | `OPENAI_API_KEY` | imagens |
| xAI | `XAI_API_KEY` | imagens e **vídeo** (animação fluida via video2dsprite) |
| fal.ai | `FAL_KEY` | edições com referência e vários modelos de vídeo |
| BytePlus ModelArk | `ARK_API_KEY` | imagens e vídeo |
| Codex CLI / Grok CLI | login da própria assinatura | sem chave de API |

O processamento (recorte, chave de cor, montagem) também precisa de
`ffmpeg`, `numpy` e `Pillow`.

### Retratos e tileset
* Retratos de diálogo: PNG em `assets/retratos/<npc>.png` (48×48 recomendado, ampliado em fator inteiro). O caminho fica em `dados/dialogos.json → retrato`.
* Tileset: `assets/tilesets/gotico.png`, tiles de 32×32 px em 8 colunas (o mapa declara `tamanhoTileArte: 32`). Os índices estão na `legenda` de cada mapa. Um mapa pode ser pintado com a skill `generate2dmap` e exportado para camadas CSV (`"camadas": {"chao": "mapas/x_chao.csv"}`).

---

## 2. Efeitos com o pixelfx

### Receitas
`tools/vfx_receitas.json` liga uma **chave** (o nome usado no C++) a um efeito
do pixelfx e às opções dele:

```json
{ "chave": "orbe_sangue", "id": "arcane/arcane-orb", "opcoes": { "palette": "blood" } }
```

* `palette` aceita o nome de um ramp de `third_party/pixelfx/src/core/color.js` (`blood`, `shadow`, `arcane`, `metal`, `bone`...) ou uma lista de cores hex.
* `fps` opcional (padrão 15, para sheets menores).
* A fonte de texto do pixelfx só tem ASCII (por isso o banner é "NOVO NIVEL!", sem acento).

### Gerar
```bash
node tools/gerar_vfx.mjs                  # todas as receitas
node tools/gerar_vfx.mjs orbe_sangue      # só uma
cmake --build --preset <preset> --target vfx
```
Saída: `assets/vfx/<chave>.png` + `.json` (Aseprite + `meta.pixelfx`: `fps`, `loop`, `pivot`, `act.hits` com o hit-stop) e `assets/vfx/indice.json`.

### Usar no C++
```cpp
ParametrosVfx p;
p.anguloGraus = graus;   // efeitos do pixelfx apontam para a direita
p.orientar = true;       // à esquerda: espelha em vez de virar de ponta-cabeça
p.escala = 0.3f;
p.camada = CamadaVfx::Frente;
vfx.tocar("lamina_corte", posicao, p);                   // one-shot: some sozinho
u32 id = vfx.tocar("tocha", pos, {});                    // loop: vive até parar(id)
vfx.desenharQuadro("flecha", tempo, pos, p);             // sem estado (projéteis)
float congelar = vfx.congelamentoDoGolpe("lamina_corte"); // hit-stop do act
```

### Efeitos em uso

| Chave | Efeito pixelfx | Onde |
|---|---|---|
| `lamina_corte`, `lamina_final`, `adaga_corte`, `machado_corte`, `foice_corte`, `bastao_golpe` | melee/* | golpes por arma |
| `impacto_sangue`, `impacto_critico` | impacts/hit-spark, crit-hit | acertos |
| `flecha`, `virote`, `bumerangue` | projectiles/arrow, energy-disc | armas à distância |
| `carga_disparo` | arcane/spell-charge | disparo carregado |
| `orbe_sangue`, `orbe_sangue_explosao` | arcane/arcane-orb, explosions/implosion | Orbe de Sangue [1] |
| `nevoa_cortante` | nature/poison-cloud | Névoa Cortante [2] |
| `correntes_sombrias` | shadow/shadow-tendrils | Correntes Sombrias [3] |
| `sangria_vital` | shadow/life-steal | Sangria Vital [Q] |
| `sede_de_sangue` | status/rage | aura da Sede de Sangue |
| `esquiva_sombria` | arcane/blink-out | Esquiva Sombria |
| `tocha`, `braseiro` | fire/torch, fire/brazier | cenário |
| `subir_nivel`, `cura`, `habilidade_desbloqueada` | ui/level-up, holy/heal, arcane/mana-burst | progressão |
| `morte_inimigo`, `coleta`, `fragmento_ferro`, `erva_palida` | spawn/disintegrate, pickups/* | inimigos e itens |
| `morcego`, `nevoa_ambiente` | critters/bat, weather/fog | atmosfera |

Para um efeito novo: crie com a skill do pixelfx (`node tools/new.mjs` dentro de
`third_party/pixelfx`), adicione a receita e rode `gerar_vfx.mjs`.

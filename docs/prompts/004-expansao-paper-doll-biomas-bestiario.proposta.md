---
numero: 004
tipo: proposta de prompt (reescrita do texto colado no prompt 004)
status: APROVADA com as decisões do prompt 005 (abaixo) — fase 1 em andamento
original: 004-expansao-paper-doll-biomas-bestiario.md
---

# Prompt 004 (reescrito) — Expansão do Andvarion

## Objetivo
Evoluir o jogo que já existe no repositório (estado atual: `docs/assets-user/game-atual.png`)
sem quebrar o que funciona: HUD, barras de Vida/Mana/Vigor, rastreador de missões,
diálogo com retrato, Ciclo de Sinergia Sangue-Arcano, save/load, iluminação noturna,
Y-sort e a regra **x86 (32 bits)**. Tudo continua 100% em PT-BR.

## Referências visuais (ler antes de qualquer código)
| Pasta / arquivo | O que define |
|---|---|
| `docs/assets-user/game-atual.png` | O ponto de partida: base funcional a manter. |
| `docs/assets-user/agent-sprite-forge-reference/` | Estilo-alvo de TODOS os sprites: chibi/JRPG em pixel art, contorno escuro, sombras marcadas. Humanoides ~33×55 px (espadachim 33×55, maga 45×56, esqueleto 50×55); monstros grandes ~87×86 px (dragão). Clipes de referência com contagem de quadros: IDLE 2, WALK 8, ATTACK 21, HURT 18, VICTORY 14; dragão ROAR 18, LUNGE 30, FALL 13. |
| `docs/assets-user/estilo-exeplo-personagem .png` | O protagonista (jovem guerreiro mágico de cabelo prateado longo) e as **peças separadas** de equipamento: corpo base, cabelo, casaco/sobretudo, capa, armadura de placas, armadura roxa, espada, escudo, capuz. |
| `docs/assets-user/tema/` | Ambientação, iluminação e mapa-múndi das regiões. |

Toda a arte de personagens e monstros é gerada com o **agent-sprite-forge**
(master still aprovado → clipes → `export_engine.py --target aseprite-json`).
Todos os efeitos vêm do **pixelfx** (receitas em `tools/vfx_receitas.json`). PixelLab.ai
e Aseprite são opcionais, só para retoques manuais fora do pipeline.

## 1. Personagem modular (paper-doll)
- O protagonista passa a ser desenhado em **camadas sincronizadas**, de trás para frente: `Capa (atrás)` → `CorpoBase` (pele, cabelo prateado, calça) → `Armadura/Túnica` → `Capa (frente)` → `Arma/Escudo`. A capa troca de lado (atrás/à frente) conforme a direção.
- Todas as camadas usam **o mesmo tamanho de quadro, a mesma âncora (pés) e os mesmos nomes/índices de clipe**. Equipar um item troca só o atlas daquela camada; a animação continua no mesmo quadro.
- Cada peça é um atlas do agent-sprite-forge gerado a partir do mesmo master still, para alinhar pixel a pixel.
- Itens com visual: Armadura de Cavaleiro, Túnica de Mago, Sobretudo Gótico, Espada, Cajado, Escudo (mais as armas atuais). A troca aparece em tempo real em idle, walk, attack, hurt (e victory).

## 2. Combate, magias e progressão
- **Manter** o combate híbrido atual: corpo a corpo (espada, adaga, machado, foice, bastão) e distância (arco, besta, bumerangue de ida e volta).
- **Grimório** (novas magias, cada uma com VFX do pixelfx): Bola de Fogo, Lança de Gelo, Relâmpago em Cadeia, Escudo Arcano, Drenar Vida, Invocação de Familiar, Teleporte Curto, Encantamento de Lâmina.
- **Inventário** com três categorias: equipamentos (por slot), consumíveis e materiais.
- **Árvore de Habilidades** com ramos **Guerreiro, Arqueiro/Explorador e Mago**.
- **Proficiência**: cada arma e cada escola de magia sobe de nível com o uso e dá bônus próprios.

## 3. Mundo, cidades e clima
- Mundo interconectado por transições de área: cidade medieval humana (muralhas, taverna, ferreiro, catedral), cidade élfica (floresta ancestral, arquitetura orgânica), fortaleza anã (montanha, forjas, minas), região costeira (praia, porto, recifes, água rasa/funda), floresta densa e masmorras.
- **Controlador global de clima e luz** com transições suaves e partículas/shaders: Dia Ensolarado, Dia Nublado/Chuvoso, Noite com Névoa (o visual atual), Tempestade com Raios, Nevasca (regiões frias), Brisa Costeira. O clima altera a cor/intensidade do ambiente e as partículas na tela, e cada região define os climas possíveis.

## 4. Bestiário inspirado em D&D
Inimigos organizados por **bioma** e **nível de desafio**, cada um com sprite no estilo da referência e IA própria:
- Mortos-vivos e cultistas: esqueleto guerreiro, zumbi, carniçal, aparição, necromante, lich.
- Humanoides e goblinoides: goblin, hobgoblin, bugbear, orc, bandido, cavaleiro corrompido.
- Monstruosidades e aberrações: mímico (baú falso), observador, devorador de mentes, cubo gelatinoso, lobisomem.
- Aquáticos e costeiros: sahuagin, elemental da água, caranguejo gigante, sereia sombria, hidra.
- Dragões e chefes: dragões elementais Azul/Gelo, Vermelho/Fogo e Negro/Ácido, com sopro, investida (LUNGE), rugido (ROAR) e queda (FALL).

## 5. Narrativa, NPCs e missões
- Todo texto em PT-BR.
- Caixa de diálogo com retrato em pixel art para todos os NPCs (mercadores, ferreiros, líderes élficos, mestres anões, curandeiras).
- Campanha principal que atravessa as regiões + missões secundárias próprias de cada cidade, no rastreador atual.

## Diretriz de execução
1. Inspecionar a arquitetura atual (`CLAUDE.md`, `docs/ARQUITETURA.md`) e listar um **plano técnico curto**: módulos novos e alterados.
2. Implementar em **fases** (cada fase compila, passa nos testes e roda o teste de fumaça). Sugestão de ordem: (1) paper-doll + inventário por slots → (2) grimório + árvore nova + proficiência → (3) clima → (4) bestiário em módulo de dados → (5) regiões e cidades → (6) campanha e missões.
3. Código limpo, modular e testável: regras novas na camada de lógica pura `andv_nucleo` com testes; conteúdo em JSON (`assets/dados`, `assets/mapas`), não fixo no C++.
4. Registrar prompt e relatório em `docs/prompts` e `docs/relatorios`.

## Pontos que dependem da sua decisão
1. **Visão top-down × referência lateral.** As imagens de referência mostram o personagem de lado (estilo batalha JRPG). O jogo é top-down e precisa de pelo menos *baixo, cima e lado* (o lado espelhado vira a esquerda). Gero essas 3 direções no estilo da referência?
2. **Escala.** Sprites de ~55 px numa tela de 480×270 com tiles de 16 px ficam grandes (1/5 da altura da tela). Proposta: subir a tela virtual para 640×360 e os tiles para 32 px. Ou prefere manter 480×270?
3. **Ramos da árvore e magias atuais.** Guerreiro/Arqueiro/Mago **substituem** Lâmina/Sangue/Sombra e as magias de sangue atuais, ou **somam-se** a elas (a Sede de Sangue continua como mecânica central)?
4. **Custo da arte.** Gerar dezenas de personagens, camadas e monstros com o agent-sprite-forge usa sua chave de API (ou o CLI local do Codex/Grok) e pode custar caro. Posso começar só pelo protagonista com camadas e 2 ou 3 inimigos, aprovando cada master still com você?
5. **Escopo por entrega.** Fazer tudo de uma vez é muito grande para revisar. Começo pela fase 1 e paro para você avaliar?

## Decisões do usuário (prompt 005)
1. **Top-down.** As imagens de referência são só inspiração visual (estilo, paleta, contorno), não a câmera nem o enquadramento. Personagens com as direções baixo, cima e lado (esquerda espelhada).
2. **Proporção maior.** Tela virtual 640×360, câmera com zoom 2 (cada tile de 16 unidades de mundo ocupa 32 px na tela) e arte em resolução dobrada: tiles de 32 px e personagens de ~55 px em quadros de 48×64.
3. **Árvore literal.** A árvore de habilidades é desenhada como uma árvore de verdade: tronco, e **cada magia nova é um galho**, com as melhorias dela como nós ao longo do galho. *(Fase 2.)*
4. **Geração de arte.** Ainda não há rota: nenhuma chave de API nem CLI local configurados. Enquanto não houver, a arte continua provisória (desenhada por código), sempre no formato do agent-sprite-forge.
5. **Por fases.** Começa pela fase 1 (escala + paper-doll + inventário por slots) e para para avaliação.

---
numero: 005
data: 2026-11-24
prompt: ../prompts/005-decisoes-expansao.md
status: fase 1 concluída — aguardando avaliação antes da fase 2
---

# Relatório 005 — Decisões da expansão e fase 1 (escala + paper-doll + inventário)

## Decisões recebidas
1. O jogo é top-down; as imagens de referência são só inspiração visual.
2. Aumentar a proporção.
3. Árvore de habilidades literal: cada magia nova é um galho *(fase 2)*.
4. Pergunta: existe alguma API para gerar a arte?
5. Trabalhar por fases, parando para avaliação.

As decisões foram gravadas na proposta `prompts/004-...proposta.md`.

## Resposta sobre APIs (pergunta 4)
Rodei o `forge_doctor.py` do agent-sprite-forge (só verificação, nada foi gerado):
- **Nenhuma chave configurada**: OpenAI, Gemini, xAI, BytePlus, fal = não.
- **Nenhum CLI local** (Codex/Grok).
- Faltam `ffmpeg`, `numpy` e `Pillow` (processamento dos quadros).

Opções do usuário (detalhes em `docs/PIPELINE_ARTE.md` → "Rotas de geração"):
`GEMINI_API_KEY` (imagens), `XAI_API_KEY` (imagens + vídeo para animação fluida),
`OPENAI_API_KEY`, `FAL_KEY`, `ARK_API_KEY`, ou o login do Codex/Grok CLI (sem chave).
A chave vai em `~/.config/agent-sprite-forge/config.json` ou em variável de ambiente,
**configurada pelo próprio usuário**, nunca colada no chat.

## O que foi feito (fase 1)
### Escala maior
- Tela virtual **480×270 → 640×360**; câmera do mundo com **zoom 2** (`core/Escala.hpp`).
- Arte em resolução dobrada: tiles de **32 px** (`tamanhoTileArte` nos mapas), personagens com **~55 px** em quadros **48×64**, âncora (24, 62), pintados com escala 0,5 (1 px de arte = 1 px de tela).
- A lógica não mudou de unidade: velocidades, alcances, colisão, luzes e mapas continuam iguais.
- Interface reposicionada (diálogo na base, menus centralizados, avisos, árvore, pausa, tela de morte, menu principal). O "[E] Conversar" foi para a camada de tela (no mundo ele dobraria de tamanho).

### Paper-doll (troca visual de equipamento)
- `gfx/PaperDoll`: Capa · Corpo · Armadura · Escudo · Arma. Só o corpo anima; cada camada desenha o mesmo clipe e o mesmo índice de quadro no próprio atlas. A ordem muda com a direção (de costas a capa cobre o corpo; de perfil o escudo fica atrás).
- A mão mostra a arma à distância enquanto mira e logo após o disparo.
- As afterimages da esquiva congelam todas as camadas.

### Equipamento e inventário
- `progressao/Equipamento` (lógica pura, testada): slots arma, distância, armadura, capa e escudo; bônus somados do catálogo; troca rápida só entre itens possuídos; save/load.
- `dados/itens.json`: categorias (equipamento, consumível, material, missão), `slot`, `arma`, `atributos`, `visual`, mais o `kitInicial`.
- Itens novos: Sobretudo Gótico, Armadura de Cavaleiro (defesa +14, vida +20, velocidade −8%), Túnica de Mago (mana +25, magia +15%), Capa Carmesim, Escudo do Leão; as 8 armas viraram itens.
- **Defesa** reduz o dano recebido: dano × 100 / (100 + defesa).
- `ui/MenuInventario` (**tecla I**): abas, lista com [E] nos equipados, prévia animada do herói em 2x, slots, atributos e descrição.
- Saves antigos sem equipamento recebem o kit inicial ao carregar.

### Arte provisória em camadas
`tools/gerar_placeholders.py` foi reescrito com uma `Pose` compartilhada por quadro.
Gera o corpo base, **13 camadas de equipamento** (3 armaduras, capa, escudo, 8 armas),
NPCs e inimigos na nova proporção e o tileset de 32 px: 1916 quadros no formato do agent-sprite-forge.

## Verificação
| Teste | Resultado |
|---|---|
| Testes de lógica (ASan/UBSan) | **613 verificações, 0 falhas**, incluindo `equipamento` e `paperdoll` |
| `testeCamadasPaperDoll` | as 13 camadas têm os mesmos clipes, quadros, durações, tamanho e âncora do corpo |
| Teste de fumaça `tools/roteiros/equipamento.txt` | inventário, equipar armadura/escudo/machado/túnica, mundo, esquiva, mira: 13 capturas, 53–60 FPS |
| Teste de fumaça `tools/roteiros/fumaca.txt` (regressão) | 14 capturas, diálogo/missão/árvore/ruínas OK na nova escala |

## Observações
- O kit inicial já traz todos os equipamentos, para facilitar a avaliação. Numa fase de conteúdo, armadura e escudo devem virar recompensa ou loja.
- O escudo ainda é só visual + defesa (não há bloqueio ativo).
- A arte continua provisória (desenhada por código) até haver uma rota de geração configurada.

## Próximo passo (aguardando aprovação)
Fase 2: grimório com as 8 magias novas e a **árvore literal** (tronco + um galho por magia, com melhorias ao longo do galho), ramos Guerreiro/Arqueiro/Mago e proficiência por arma/escola.

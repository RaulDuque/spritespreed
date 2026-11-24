---
numero: 004
data: 2026-11-24
prompt: ../prompts/004-expansao-paper-doll-biomas-bestiario.md
status: aguardando aprovação do usuário (nenhum código alterado)
---

# Relatório 004 — Reescrita do prompt de expansão

## Pedido
Transformar o texto colado (explicação sobre o agent-sprite-forge e paper-doll + um
"mega prompt" para Codex/Cursor) num prompt, para o usuário avaliar se foi entendido.
**Esperar aprovação antes de implementar.**

## O que foi feito
- Prompt original salvo na íntegra em `docs/prompts/004-...md`.
- Versão reescrita em `docs/prompts/004-...proposta.md`, organizada em: referências, paper-doll, combate/magias/progressão, mundo e clima, bestiário, narrativa e diretriz de execução em fases.
- Conferi as referências citadas: `agent-sprite-forge-reference/` tem 2 imagens (tamanhos 33×55, 45×56, 50×55, 87×86 e clipes IDLE 2, WALK 8, ATTACK 21, HURT 18, VICTORY 14, ROAR 18, LUNGE 30, FALL 13). `tema/Full Map.png` é grande demais para o visualizador (acima de 2576 px) e não foi aberto.
- Nenhum código, asset ou geração de arte foi alterado ou executado.

## Diferenças entre o texto colado e a proposta
- A parte explicativa ("como funciona...") virou contexto; só o "mega prompt" virou instrução.
- Acrescentei o que o projeto exige e o texto não citava: manter a regra de 32 bits, o Ciclo de Sinergia, o save, os testes e o registro de prompts/relatórios.
- Paper-doll detalhado com ordem das camadas e âncora comum; clipes VICTORY/ROAR/LUNGE/FALL incluídos.
- Execução dividida em 6 fases verificáveis, em vez de uma entrega única.

## Decisões pendentes (perguntas ao usuário)
1. Direções top-down × referência lateral.
2. Escala: 480×270 com tiles 16 px ou 640×360 com tiles 32 px.
3. Novos ramos e magias substituem ou somam-se aos atuais.
4. Custo de geração de arte: começar pequeno, aprovando cada master still.
5. Entregar por fases, começando pela fase 1.

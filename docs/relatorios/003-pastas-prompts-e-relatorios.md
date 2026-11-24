---
numero: 003
data: 2026-11-24
prompt: ../prompts/003-pastas-prompts-e-relatorios.md
status: concluído
---

# Relatório 003 — Pastas de prompts e relatórios

## Pedido
Criar em `docs/` uma pasta para os prompts do usuário (salvos e organizados) e
outra para os relatórios de cada prompt.

## O que foi feito
- `docs/prompts/`: um arquivo por prompt (`NNN-tema.md`) com o texto **na íntegra, sem correções**, e o índice `README.md`.
- `docs/relatorios/`: um relatório por prompt com o mesmo número, e o índice `README.md`.
- Registrei retroativamente os prompts 001 (`/init`) e 002 (arquitetura do jogo), com os relatórios deles.
- `CLAUDE.md` ganhou a regra para que toda sessão futura continue registrando prompts e relatórios.
- Guardei a preferência na memória do Claude Code para valer também em conversas novas.

## Formato
- **Prompt**: cabeçalho (`numero`, `data`, `tema`, link do relatório) e o texto exato.
- **Relatório**: pedido, o que foi feito, decisões, verificação, bugs e pendências.

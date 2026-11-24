---
numero: 008
data: 2026-11-24
prompt: ../prompts/008-subir-para-git.md
status: concluído — enviado para github.com/RaulDuque/spritespreed
---

# Relatório 008 — Subir o projeto para o git

## Pedido
Versionar o projeto no git e enviá-lo ao GitHub.

## O que foi feito
- A pasta `spriteSpreed/` ainda não era um repositório git, e nenhum remoto estava configurado.
- Rodei `git init` com o branch `main` e fiz o commit inicial com tudo que o `.gitignore` permite. Ficaram de fora `build/`, `dist/` (o pacote Windows gerado), saves e as saídas locais do pixelfx.
- Verifiquei se havia arquivos de segredo (`.env`, chaves, credenciais) e não encontrei nenhum.

## Decisões
- `dist/` continua fora do git, porque é artefato de build. O pacote pode ser gerado de novo com `tools/empacotar_windows.py`.
- `docs/assets-user/` (cerca de 42 MB de referências de arte do usuário) entrou no commit. Nenhum arquivo passa de 100 MB, que é o limite do GitHub.

## Envio
- O `gh` não está instalado e não há token, então o usuário criou o repositório vazio `RaulDuque/spritespreed` no GitHub.
- Remoto `origin` = `git@github-andvarion:RaulDuque/spritespreed.git` (usa a chave SSH `~/.ssh/andvarion`). Fiz o push do `main` com upstream configurado.

## Pendências
- Nenhuma.

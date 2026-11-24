---
numero: 006
data: 2026-11-24
prompt: ../prompts/006-aprovacao-fase2.md
status: fase 2 concluída — aguardando avaliação antes da fase 3 (clima)
---

# Relatório 006 — Fase 2: grimório, árvore literal e proficiência

## Pedido
Fase 1 aprovada ("to gostando, pode seguir"). Fase 2: as 8 magias novas, a árvore
de habilidades **literal** (cada magia um galho), ramos Guerreiro/Arqueiro/Mago e
proficiência por arma e escola.

## Decisão de projeto
As 4 magias antigas **continuam**, cada uma também como galho. São 12 magias no
total, e o jogador escolhe 4 para os slots 1, 2, 3 e Q (o **grimório**). A Sede de
Sangue (Ciclo de Sinergia) vale para todas.

## O que foi feito
### Magias novas (com efeitos do pixelfx)
| Magia | Escola | Comportamento | Melhorias no galho |
|---|---|---|---|
| Bola de Fogo | Fogo | projétil que explode em área | +30% dano · +12 raio |
| Lança de Gelo | Gelo | atravessa inimigos e deixa lentos (45% da velocidade) | +1,5 s lentidão · +30% dano |
| Relâmpago em Cadeia | Raio | salta entre 4 inimigos (−15% por salto), atordoa rápido | +2 alvos · +30% dano |
| Escudo Arcano | Arcano | absorve 40 de dano por 8 s | +30 absorção · −25% recarga |
| Drenar Vida | Sangue | dano no inimigo mais perto da mira, cura 60% | +30% cura · +30% dano |
| Invocação de Familiar | Arcano | espírito orbita e atira por 20 s | +10 s · segundo familiar |
| Teleporte Curto | Arcano | reaparece até 90 na direção da mira, sem atravessar parede | +40 alcance · −30% recarga |
| Encantamento de Lâmina | Fogo | 10 s de golpes com +50% de dano e faíscas | +25% dano · +6 s |

15 efeitos novos do pixelfx (45 no total), gerados por `tools/gerar_vfx.mjs`.

### Árvore literal (`ui/MenuArvoreHabilidades`, tecla K)
- Tronco com raízes, três ramos grossos e **15 galhos curvos**: 3 do Guerreiro (Lâmina, Vigor, Encantamento), 3 do Arqueiro/Explorador (Pontaria, Teleporte, Familiar) e 9 do Mago em leque (um por magia).
- 44 nós ao longo dos galhos. Um nó exige o anterior do mesmo galho; nós aprendidos ganham folhas, e a madeira do galho ganha a cor da escola.
- Nó de magia mostra o ícone do próprio efeito; descrição embaixo com galho, custo e nível.
- **Grimório**: com o mouse sobre uma magia aprendida, as teclas 1/2/3/Q a colocam no slot. Magia aprendida entra sozinha num slot livre.
- Aba **Proficiências**: 8 armas e 6 escolas, com nível, barra e bônus.
- Os ids dos nós antigos foram mantidos, então saves antigos continuam válidos.

### Proficiência (`progressao/Proficiencia`, lógica pura)
- Armas sobem com acertos (golpes e projéteis); escolas sobem a cada conjuração (+3).
- Nível 1–10, XP para o próximo nível = 15 × nível. Arma: +4% de dano por nível. Escola: +4% de dano mágico e −2% de mana por nível.
- Aviso na tela ao subir ("Escola do Fogo: proficiência 3"). Salva no save.

### Outros
- `VFXManager::desenharIcone`: ícone centralizado e ajustado ao quadrado, para qualquer efeito.
- HUD: slots mostram a magia do grimório, a recarga e o custo com desconto, mais avisos de Escudo Arcano e Lâmina Encantada.
- Recarga é **por magia**: trocar de slot não zera a espera.

## Verificação
| Teste | Resultado |
|---|---|
| Testes de lógica (ASan/UBSan) | **707 verificações, 0 falhas**: novos `galhos` (um galho por magia, requisito implícito, galho inexistente recusado) e `proficiencia`; save com grimório e proficiências |
| Geometria da árvore | menor distância entre nós: 21,5 px (raio 7); tudo dentro do painel |
| Roteiro da árvore | aprender 11 nós pelo mouse, colocar 4 magias no grimório, aba de proficiências |
| Roteiros das magias (nave da catedral) | relâmpago matou cultista (tracker 1/5), bola de fogo explodiu, lança de gelo, familiar atirando, escudo absorveu 23 de dano, encantamento ativo, drenar curou até o máximo, teleporte |
| Regressão `fumaca.txt` e `equipamento.txt` | 14 e 13 capturas, sem erro |

## Bugs encontrados e corrigidos
- Nós da base dos galhos do Mago se sobrepunham → ramos mais afastados, galhos mais longos, nós da metade do galho até a ponta.
- Ícones de magias sumiam (cada efeito tem pivô diferente) → `desenharIcone`.
- A árvore reabria na última aba → sempre abre em "Árvore".
- `ColorLerp` não existe na raylib 5.0 → função local.

## Observações
- Com 60 de mana cabem cerca de 3 magias seguidas. Depois é preciso cortar para drenar essência (o ciclo de sinergia, como planejado).
- Teclado na árvore: ainda é só mouse para escolher o nó (as teclas trocam de aba e atribuem slots).

## Próximo passo (aguardando aprovação)
Fase 3: controlador global de clima e luz (Dia Ensolarado, Nublado/Chuvoso, Noite com Névoa, Tempestade com Raios, Nevasca, Brisa Costeira), com transições suaves e partículas do pixelfx.

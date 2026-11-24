---
numero: 002
data: 2026-11-24
tema: Arquitetura e código do jogo (Action RPG gótico 32 bits)
relatorio: ../relatorios/002-arquitetura-action-rpg-gotico.md
---

# Prompt 002 — Arquitetura completa do Action RPG gótico

Atue como um Arquiteto de Software Sênior e Game Designer especializado em C++. Crie a arquitetura completa, a estrutura de pastas (CMake) e o código-fonte modular para um Action RPG Gótico/Medieval Top-Down em Pixel Art, inspirado em Dungeons & Dragons e Bloodborne.

Requisitos Técnicos e de Arquitetura Obrigatórios:

Arquitetura Base (32-bit): Todo o projeto, configuração de compilação (CMakeLists.txt), bibliotecas vinculadas e tipos de dados devem ter como alvo obrigatório a arquitetura 32-bit (x86 / Win32), garantindo compatibilidade nativa com sistemas e pipelines de 32 bits.

Linguagem: C++ moderno (C++17), orientado a objetos, utilizando padrões de projeto como State Machine (Máquina de Estados), Observer (para quests/eventos) e gerenciamento seguro de memória (std::unique_ptr / std::shared_ptr).

Biblioteca Gráfica: Utilize SFML (versão 32-bit) ou Raylib (versão 32-bit) para renderização 2D, shaders de iluminação e carregamento de texturas.

Idioma: 100% em Português (PT-BR) (menus, diálogos, nomes de itens, habilidades e interface).

1. Pipeline de Arte, Sprites e Efeitos Visuais
Movimentação e Animação de Personagens (agent-sprite-forge):

Utilize o padrão de geração, recorte e organização de sprite sheets do repositório [https://github.com/0x0funky/agent-sprite-forge](https://github.com/0x0funky/agent-sprite-forge) como base para toda a movimentação e animação do protagonista e NPCs.

Implemente um leitor/parser de sprite sheets compatível com os metadados e grids gerados pelo agent-sprite-forge, cobrindo estados de ocioso (idle), caminhada/corrida em 4 ou 8 direções, ataques corpo a corpo, disparo à distância e dano.

Efeitos de Habilidades, Magias e Skills (pixelfx):

Utilize a biblioteca/pipeline [https://github.com/patch-gg/pixelfx](https://github.com/patch-gg/pixelfx) para gerar, renderizar e animar todos os efeitos visuais de combate, magias, trilhas de impacto, partículas e habilidades ativas/passivas da Skill Tree.

Ordenação e Iluminação Atmosférica (Estilo Bloodborne):

Ordenação de profundidade dinâmica (Y-Sorting baseado na coordenada Y das entidades).

Camada de escuridão/filtro azulado noturno sobre o cenário com luzes 2D dinâmicas: o jogador carrega uma lanterna na cintura (luz quente circular) e tochas nos cenários emitem luz oscilante.

2. Protagonista, Armas e Árvore de Habilidades
Personagem: Um jovem guerreiro mágico de cabelos prateados.

Arsenal Medieval Variado:

Corpo a Corpo: Suporte para múltiplos arquétipos de armas com alcances, velocidades e animações próprias: Espada, Adaga, Machado, Foice e Bastão.

Longa Distância: Arco e flecha, Besta e Bumerangue (que vai e volta até a posição do jogador).

Progressão e Skill Tree: Sistema de XP, níveis e uma Árvore de Habilidades (Skill Tree) interativa acessada via menu, permitindo desbloquear novos feitiços, melhorias de atributos e passivas (com efeitos visuais alimentados pelo pixelfx).

3. Sistema de Combate Único: "Ciclo de Sinergia Sangue-Arcano"
Implemente uma Máquina de Estados (PlayerStateMachine) que permita ao jogador alternar e combinar os três estilos em tempo real sem abrir menus:

Botão Esquerdo do Mouse (Lâmina): Executa um combo de 3 golpes rápidos. Cada acerto em um inimigo drena essência vital e regenera Mana (MP).

Botão Direito do Mouse (Armas à Distância): Mira direcional com o cursor e dispara (consome Vigor/Stamina). Segurar o botão carrega um disparo perfurante de alto dano.

Teclas 1, 2, 3 / Q (Magia Gótica): Conjura feitiços sombrios consumindo Mana (ex: Orbe de Sangue, Névoa Cortante) renderizados com efeitos do pixelfx.

Mecânica de Sinergia (Sede de Sangue): Gastar Mana conjurando uma magia aplica um bônus temporário de dano e velocidade no próximo ataque de Lâmina ou disparo à distância — incentivando o jogador a alternar constantemente entre cortar para ganhar mana, conjurar e finalizar à distância.

Espaço / Shift (Esquiva Sombria / Dash): Impulso rápido com janela de invulnerabilidade (i-frames) deixando rastros visuais (afterimages) e partículas geradas via pixelfx.

4. Mundo, Mapas por Tilesets, NPCs e Quests
Carregamento de Mapas: Sistema de mapa baseado em Tilesets (carregados via matriz/JSON/CSV) com camadas de chão, paredes com colisão AABB/circular e pontos de transição entre cenas.

Cenas / Áreas Jogáveis:

Menu Principal: Opções "Novo Jogo", "Continuar" (sistema de Save/Load em JSON) e "Sair".

Vila de Cinzavale (Área Segura): Casas góticas, tochas acesas, ferreiro, curandeira, vigia e a estrada para a área hostil.

Ruínas da Catedral (Área Hostil): Masmorra/ruínas sombrias com inimigos patrulhando (cultistas, carniçais com IA de perseguição e ataque) e itens coletáveis.

Sistema de Diálogo (DialogueSystem):

Caixa de diálogo em estilo pergaminho escuro na parte inferior da tela.

Deve renderizar obrigatoriamente o Retrato em Pixel Art (Portrait) do NPC à esquerda, o Nome do NPC, texto com efeito de digitação progressiva (typewriter) e opções de escolha navegáveis para aceitar missões ou conversar.

Sistema de Quests (QuestManager):

Estrutura de dados para missões com objetivos rastreáveis (MATAR_INIMIGO, COLETAR_ITEM, FALAR_NPC), estados (NAO_INICIADA, ATIVA, PRONTA_PARA_ENTREGAR, CONCLUIDA) e um HUD discreto (Quest Tracker) no canto da tela.

Quest Principal: "O Sussurro sob a Catedral" (investigar a corrupção, derrotar cultistas nas ruínas e reportar ao Vigia).

Side Quest 1: "Ferro Amaldiçoado" (dada pelo Ferreiro para coletar fragmentos nos carniçais).

Side Quest 2: "Cinzas da Memória" (dada pela Curandeira para recuperar ervas pálidas).

5. Entregáveis Esperados
Configuração de Build 32-bit: Arquivo CMakeLists.txt configurado explicitamente para compilação x86 (32-bit), incluindo o link com a biblioteca gráfica (32-bit) e a integração dos pipelines agent-sprite-forge e pixelfx.

Diagrama da Arquitetura e Árvore de Diretórios (src/, include/, assets/sprites/, assets/vfx/).

Código-fonte completo e comentado dos sistemas centrais (main.cpp, AnimationController adaptado ao agent-sprite-forge, VFXManager integrado ao pixelfx, PlayerStateMachine, QuestManager e DialogueSystem), pronto para compilar em ambiente 32-bit.

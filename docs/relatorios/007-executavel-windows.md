---
numero: 007
data: 2026-11-24
prompt: ../prompts/007-executavel-windows.md
status: pacote gerado — falta rodar no Windows 11 (não há Windows/Wine nesta máquina)
---

# Relatório 007 — Pasta com executável para testar no Windows 11

## Pedido
Gerar uma pasta com o executável e um arquivo para testar o jogo no Windows 11.

## O que foi feito
### Build Windows de 32 bits real
- Baixei e extraí o MinGW-w64 i686 (GCC 13, `-posix`) **no scratchpad**, sem instalar nada no sistema.
- Compilei pelo próprio `CMakeLists.txt` do projeto com `cmake/toolchains/mingw-i686.cmake`. É a **primeira compilação 32 bits real** do projeto: a trava do CMake confirmou `sizeof(void*) = 4, processador = i686`, o `static_assert` ficou ativo e passou, e não houve nenhum aviso no código do jogo.
- `Andvarion.exe`: **PE32, Intel i386, GUI**; depende só de DLLs do Windows (GDI32, KERNEL32, msvcrt, SHELL32, USER32, WINMM). OpenGL é carregado em tempo de execução. Ligação estática da libstdc++/libgcc/winpthread.

### Pasta e arquivo de teste
`dist/Andvarion-Win32/` (e `dist/Andvarion-Win32.zip`, 9,9 MB):
| Arquivo | Para quê |
|---|---|
| `Andvarion.exe` | o jogo |
| `TESTAR_NO_WINDOWS.bat` | menu: 1 jogar · 2 teste automático missão/combate/ruínas · 3 teste automático inventário/equipamento · 4 testes de lógica |
| `andv_testes.exe` | testes de lógica (console) |
| `roteiros/` | roteiros dos testes automáticos |
| `assets/` | arte, mapas, dados |
| `LEIA-ME.txt` | requisitos, controles, SmartScreen, onde ficam saves e capturas |

Os testes automáticos gravam as capturas em `capturas_<teste>\` e usam um save próprio, então o save do jogador não é tocado.

### Correções feitas para o Windows
1. **Caminhos com acento.** A raylib entrega o caminho do executável na página de código ANSI do Windows, mas o código lia caminhos como UTF-8 (`u8path`). Numa pasta como "Área de Trabalho", os sprites dos personagens não carregariam e o save podia falhar. Agora tudo usa a codificação nativa.
2. **Toolchain MinGW:** aceita os nomes `-posix`/`-win32` do Debian/Ubuntu e não cai mais no `gcc` do Linux por engano.
3. **`ANDV_SAVE`:** variável para trocar o arquivo de save (usada pelos testes).
4. **`andv_testes.exe`** procura `assets\` ao lado do executável quando o caminho do build não existe.

### Para gerar de novo
`tools/empacotar_windows.py <pasta-do-build>`: confere se os .exe são PE32 i386, monta a pasta, converte os textos para CRLF e gera o zip.

## Verificação
| Teste | Resultado |
|---|---|
| Build MinGW i686 (Release) | OK, 0 avisos no código do projeto |
| Cabeçalho dos .exe | PE32 Intel i386 (conferido também pelo script) |
| DLLs importadas | só do sistema |
| Regressão Linux após as correções (pasta com acento) | teste de equipamento: 13 capturas, nenhum sprite ausente |

**Não verificado:** rodar no Windows. Esta máquina não tem Windows nem Wine. É o mesmo código que roda no Linux, agora compilado de verdade para x86.

## O que pedir ao usuário
Rodar a opção **4** (testes de lógica) e depois a **2** no Windows 11, e mandar o resultado/capturas se algo falhar.

@echo off
chcp 65001 >nul
title Andvarion - Teste no Windows
cd /d "%~dp0"

:menu
cls
echo ==========================================================
echo    ANDVARION - O Sussurro sob a Catedral   (x86 / 32 bits)
echo ==========================================================
echo.
echo    1. Jogar
echo    2. Teste automatico: missao, combate e ruinas  (~25 s)
echo    3. Teste automatico: inventario e equipamento  (~20 s)
echo    4. Testes de logica (missoes, save, arvore, atlas...)
echo    5. Sair
echo.
echo    Nos testes automaticos o jogo se controla sozinho:
echo    nao mexa no teclado ate a janela fechar.
echo.
set "op="
set /p op="Escolha uma opcao e aperte Enter: "
if "%op%"=="1" goto jogar
if "%op%"=="2" call :roteiro fumaca & goto menu
if "%op%"=="3" call :roteiro equipamento & goto menu
if "%op%"=="4" goto logica
if "%op%"=="5" exit /b 0
goto menu

:jogar
start "" "%~dp0Andvarion.exe"
goto menu

:logica
echo.
"%~dp0andv_testes.exe"
if errorlevel 1 (echo. & echo  *** ALGUM TESTE FALHOU - envie esta tela ***) else (echo. & echo  Todos os testes passaram.)
echo.
pause
goto menu

:roteiro
rem Cada teste usa uma pasta e um save proprios: o seu save de jogo nao e tocado.
set "PASTA=%~dp0capturas_%1"
if not exist "%PASTA%" mkdir "%PASTA%"
del /q "%PASTA%\*.png" "%PASTA%\save_teste.json" 2>nul
pushd "%PASTA%"
set "ANDV_ROTEIRO=%~dp0roteiros\%1.txt"
set "ANDV_SAVE=%PASTA%\save_teste.json"
echo.
echo Rodando o teste "%1"... aguarde a janela do jogo fechar sozinha.
start /wait "" "%~dp0Andvarion.exe"
set "ANDV_ROTEIRO="
set "ANDV_SAVE="
popd
echo.
echo Pronto! As capturas de tela estao em:
echo   %PASTA%
start "" explorer "%PASTA%"
pause
exit /b 0

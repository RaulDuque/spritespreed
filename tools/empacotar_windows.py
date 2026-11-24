#!/usr/bin/env python3
"""
Monta a pasta de teste para Windows a partir de um build MinGW/MSVC de 32 bits.

    python3 tools/empacotar_windows.py <pasta-do-build> [saida]

Saída padrão: dist/Andvarion-Win32/ e dist/Andvarion-Win32.zip, com
Andvarion.exe, andv_testes.exe, assets/, roteiros de teste, TESTAR_NO_WINDOWS.bat
e LEIA-ME.txt (fins de linha CRLF, como o Windows espera).
"""
import os
import shutil
import struct
import sys
import zipfile

RAIZ = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))


def eh_pe32_i386(caminho):
    """Confere o cabeçalho PE: máquina 0x14c (Intel 386) = 32 bits."""
    with open(caminho, "rb") as f:
        dados = f.read(1024)
    if dados[:2] != b"MZ":
        return False
    pe = struct.unpack_from("<I", dados, 0x3C)[0]
    return dados[pe:pe + 4] == b"PE\0\0" and struct.unpack_from("<H", dados, pe + 4)[0] == 0x14C


def copiar_texto_crlf(origem, destino):
    with open(origem, "r", encoding="utf-8") as f:
        texto = f.read().replace("\r\n", "\n").replace("\n", "\r\n")
    with open(destino, "w", encoding="utf-8", newline="") as f:
        f.write(texto)


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    build = sys.argv[1]
    saida = sys.argv[2] if len(sys.argv) > 2 else os.path.join(RAIZ, "dist", "Andvarion-Win32")
    exes = {"andvarion.exe": "Andvarion.exe", "andv_testes.exe": "andv_testes.exe"}
    for origem in exes:
        caminho = os.path.join(build, origem)
        if not os.path.isfile(caminho):
            print(f"erro: {caminho} não existe (compile com o preset mingw-i686 ou win32-msvc)")
            return 1
        if not eh_pe32_i386(caminho):
            print(f"erro: {origem} não é um executável Windows de 32 bits (x86)")
            return 1

    if os.path.isdir(saida):
        shutil.rmtree(saida)
    os.makedirs(os.path.join(saida, "roteiros"))
    for origem, nome in exes.items():
        shutil.copy2(os.path.join(build, origem), os.path.join(saida, nome))
    shutil.copytree(os.path.join(RAIZ, "assets"), os.path.join(saida, "assets"))
    for r in ("fumaca.txt", "equipamento.txt"):
        copiar_texto_crlf(os.path.join(RAIZ, "tools", "roteiros", r), os.path.join(saida, "roteiros", r))
    copiar_texto_crlf(os.path.join(RAIZ, "tools", "windows", "TESTAR_NO_WINDOWS.bat"), os.path.join(saida, "TESTAR_NO_WINDOWS.bat"))
    copiar_texto_crlf(os.path.join(RAIZ, "tools", "windows", "LEIA-ME.txt"), os.path.join(saida, "LEIA-ME.txt"))

    zip_path = saida.rstrip("/\\") + ".zip"
    base = os.path.basename(saida.rstrip("/\\"))
    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as z:
        for pasta, _, arquivos in os.walk(saida):
            for a in sorted(arquivos):
                completo = os.path.join(pasta, a)
                z.write(completo, os.path.join(base, os.path.relpath(completo, saida)))
    tamanho = os.path.getsize(zip_path) / (1024 * 1024)
    print(f"pasta: {saida}")
    print(f"zip:   {zip_path} ({tamanho:.1f} MB)")
    return 0


if __name__ == "__main__":
    sys.exit(main())

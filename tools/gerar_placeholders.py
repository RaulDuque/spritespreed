#!/usr/bin/env python3
"""
Arte PROVISÓRIA desenhada por código (sem modelo de imagem), só para o jogo
rodar de ponta a ponta antes da arte final:

  assets/tilesets/gotico.png              tileset com tiles de 32 px (8 colunas)
  assets/sprites/protagonista/            CORPO BASE do herói (camada do paper-doll)
  assets/sprites/equip/<item>/            uma camada por equipamento (armaduras, capa, escudo, armas)
  assets/sprites/<npc|inimigo>/           NPCs e inimigos (camadas achatadas)
  assets/retratos/<npc>.png               retratos 48x48 para o diálogo

Todos os atlas seguem o formato do agent-sprite-forge (export_engine.py
--target aseprite-json): quadros 48x64, âncora dos pés em (24, 62), clipes
"<acao>_<direcao>". As camadas do herói compartilham a mesma Pose por quadro,
por isso encaixam pixel a pixel (contrato do PaperDoll).

A arte final substitui estes arquivos: gere o personagem com a skill
generate2dsprite/video2dsprite do agent-sprite-forge, exporte com
`export_engine.py --target aseprite-json` e copie a pasta aseprite/ para
assets/sprites/<personagem>/. Use os nomes de clipe "<acao>_<direcao>"
(ver docs/PIPELINE_ARTE.md). Nada no C++ muda.

Só biblioteca padrão (zlib + struct). Uso: python3 tools/gerar_placeholders.py
"""
import json
import math
import os
import random
import struct
import zlib

RAIZ = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "assets")


# ----------------------------------------------------------------------------- PNG
class Imagem:
    def __init__(self, w, h, cor=(0, 0, 0, 0)):
        self.w, self.h = w, h
        self.px = [list(cor) for _ in range(w * h)]

    def get(self, x, y):
        if 0 <= x < self.w and 0 <= y < self.h:
            return self.px[y * self.w + x]
        return [0, 0, 0, 0]

    def set(self, x, y, c):
        if 0 <= x < self.w and 0 <= y < self.h:
            if len(c) == 3:
                c = (*c, 255)
            self.px[y * self.w + x] = list(c)

    def rect(self, x0, y0, x1, y1, c):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                self.set(x, y, c)

    def colar(self, outra, ox, oy):
        for y in range(outra.h):
            for x in range(outra.w):
                p = outra.px[y * outra.w + x]
                if p[3]:
                    self.set(ox + x, oy + y, p)

    def salvar(self, caminho):
        os.makedirs(os.path.dirname(caminho), exist_ok=True)
        bruto = b"".join(b"\x00" + bytes(v for p in self.px[y * self.w:(y + 1) * self.w] for v in p) for y in range(self.h))

        def bloco(tipo, dados):
            return struct.pack(">I", len(dados)) + tipo + dados + struct.pack(">I", zlib.crc32(tipo + dados) & 0xFFFFFFFF)

        with open(caminho, "wb") as f:
            f.write(b"\x89PNG\r\n\x1a\n")
            f.write(bloco(b"IHDR", struct.pack(">IIBBBBB", self.w, self.h, 8, 6, 0, 0, 0)))
            f.write(bloco(b"IDAT", zlib.compress(bruto, 9)))
            f.write(bloco(b"IEND", b""))


def escurecer(c, k):
    return tuple(max(0, min(255, int(v * k))) for v in c[:3])


def contorno(img, cor=(12, 8, 14)):
    """Contorno de 1 px em volta dos pixels opacos (leitura em fundo escuro)."""
    marcar = []
    for y in range(img.h):
        for x in range(img.w):
            if img.get(x, y)[3]:
                continue
            if any(img.get(x + dx, y + dy)[3] for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                marcar.append((x, y))
    for x, y in marcar:
        img.set(x, y, cor)


# ----------------------------------------------------------------------------- TILESET
def tileset():
    random.seed(3)
    T = 16
    img = Imagem(8 * T, 5 * T)

    def tile(i):
        return (i % 8) * T, (i // 8) * T

    def ruido(i, base, var, prob=0.35):
        ox, oy = tile(i)
        for y in range(T):
            for x in range(T):
                c = base
                r = random.random()
                if r < prob:
                    c = escurecer(base, 1 - var)
                elif r < prob * 1.4:
                    c = escurecer(base, 1 + var * 0.6)
                img.set(ox + x, oy + y, c)

    def pedras(i, base, junta, tam=(5, 4)):
        ox, oy = tile(i)
        ruido(i, base, 0.12)
        w, h = tam
        for y in range(0, T, h):
            desloc = (y // h % 2) * (w // 2)
            for x in range(T):
                img.set(ox + x, oy + y, junta)
            for x in range(-desloc, T, w):
                for yy in range(y, min(T, y + h)):
                    img.set(ox + x, oy + yy, junta)

    # Linha 0: chãos
    ruido(0, (34, 46, 38), 0.25)                      # grama escura
    ruido(1, (38, 50, 40), 0.3, 0.45)
    for k in range(6):
        ox, oy = tile(1)
        x, y = random.randrange(T), random.randrange(2, T)
        img.set(ox + x, oy + y, (70, 80, 60)); img.set(ox + x, oy + y - 1, (60, 74, 54))
    ruido(2, (66, 54, 42), 0.2)                       # terra da estrada
    pedras(3, (70, 70, 78), (40, 40, 48))             # calçamento
    pedras(4, (64, 66, 74), (38, 38, 46), (4, 4))
    pedras(5, (54, 52, 60), (30, 28, 36), (8, 8))     # piso das ruínas
    pedras(6, (50, 48, 56), (28, 26, 34), (8, 8))     # piso rachado
    ox, oy = tile(6)
    x = 3
    for y in range(2, 14):
        x += random.choice((-1, 0, 1))
        img.set(ox + max(0, min(15, x)), oy + y, (20, 18, 24))
    ruido(7, (96, 22, 30), 0.15)                      # tapete vermelho
    ox, oy = tile(7)
    img.rect(ox, oy, ox + 1, oy + 15, (150, 120, 60)); img.rect(ox + 14, oy, ox + 15, oy + 15, (150, 120, 60))

    # Linha 1: paredes e casas
    pedras(8, (44, 42, 52), (26, 24, 32), (8, 8))     # topo de muro
    ox, oy = tile(8)
    img.rect(ox, oy + 14, ox + 15, oy + 15, (20, 18, 24))
    pedras(9, (72, 58, 54), (34, 26, 28), (6, 4))     # tijolos (face)
    ox, oy = tile(9)
    img.rect(ox, oy, ox + 15, oy + 1, (90, 76, 70))
    ruido(10, (52, 30, 36), 0.15)                     # telhado
    ox, oy = tile(10)
    for y in range(0, T, 4):
        img.rect(ox, oy + y, ox + 15, oy + y, (30, 16, 22))
        for x in range((y // 4 % 2) * 2, T, 4):
            img.set(ox + x, oy + y + 1, (70, 42, 46))
    ruido(11, (92, 78, 64), 0.12)                     # parede de enxaimel
    ox, oy = tile(11)
    img.rect(ox, oy, ox + 15, oy + 1, (40, 28, 22)); img.rect(ox, oy + 14, ox + 15, oy + 15, (40, 28, 22))
    img.rect(ox + 7, oy, ox + 8, oy + 15, (40, 28, 22))
    ox, oy = tile(12)                                 # porta
    img.colar(Imagem(1, 1), 0, 0)
    for y in range(T):
        for x in range(T):
            img.set(ox + x, oy + y, img.get(tile(11)[0] + x, tile(11)[1] + y))
    img.rect(ox + 3, oy + 2, ox + 12, oy + 15, (58, 36, 24)); img.rect(ox + 4, oy + 3, ox + 11, oy + 15, (78, 50, 32))
    img.set(ox + 10, oy + 9, (200, 170, 90))
    ox, oy = tile(13)                                 # janela acesa
    for y in range(T):
        for x in range(T):
            img.set(ox + x, oy + y, img.get(tile(11)[0] + x, tile(11)[1] + y))
    img.rect(ox + 3, oy + 3, ox + 12, oy + 11, (40, 28, 22)); img.rect(ox + 4, oy + 4, ox + 11, oy + 10, (255, 190, 90))
    img.rect(ox + 7, oy + 4, ox + 8, oy + 10, (40, 28, 22)); img.rect(ox + 4, oy + 7, ox + 11, oy + 7, (40, 28, 22))
    ruido(14, (60, 10, 18), 0.25)                     # poça de sangue
    img.rect(tile(15)[0], tile(15)[1], tile(15)[0] + 15, tile(15)[1] + 15, (6, 5, 10))  # abismo

    # Linha 2: decoração (transparente)
    def deco(i, desenhar):
        sub = Imagem(T, T)
        desenhar(sub)
        contorno(sub)
        img.colar(sub, *tile(i))

    deco(16, lambda s: (s.rect(1, 6, 14, 7, (90, 66, 44)), s.rect(1, 11, 14, 12, (90, 66, 44)),
                        s.rect(2, 3, 3, 15, (110, 80, 52)), s.rect(12, 3, 13, 15, (110, 80, 52))))
    deco(17, lambda s: (s.rect(4, 3, 11, 15, (110, 110, 118)), s.rect(5, 2, 10, 2, (110, 110, 118)),
                        s.rect(7, 5, 8, 10, (60, 60, 70)), s.rect(5, 7, 10, 7, (60, 60, 70)), s.rect(4, 13, 11, 15, (80, 80, 90))))
    deco(18, lambda s: (s.rect(2, 4, 13, 15, (120, 84, 50)), s.rect(2, 4, 13, 5, (150, 110, 70)),
                        s.rect(7, 4, 8, 15, (80, 56, 34)), s.rect(2, 10, 13, 10, (80, 56, 34))))
    deco(19, lambda s: (s.rect(4, 3, 11, 15, (110, 70, 40)), s.rect(3, 6, 12, 12, (110, 70, 40)),
                        s.rect(3, 5, 12, 5, (70, 70, 80)), s.rect(3, 12, 12, 12, (70, 70, 80)), s.rect(5, 3, 10, 3, (140, 96, 56))))
    deco(20, lambda s: (s.rect(3, 11, 9, 12, (220, 214, 196)), s.rect(10, 9, 12, 11, (220, 214, 196)),
                        s.set(11, 10, (40, 30, 30)), s.rect(5, 13, 12, 13, (190, 184, 170))))
    deco(21, lambda s: (s.rect(2, 9, 8, 15, (84, 80, 90)), s.rect(7, 6, 13, 15, (70, 66, 76)), s.rect(4, 12, 10, 15, (96, 92, 100))))
    deco(22, lambda s: (s.rect(2, 6, 13, 9, (70, 72, 82)), s.rect(0, 6, 3, 7, (70, 72, 82)), s.rect(6, 10, 10, 15, (50, 52, 60)),
                        s.rect(3, 6, 12, 6, (130, 134, 150))))
    deco(23, lambda s: (s.rect(1, 6, 14, 15, (70, 60, 70)), s.rect(1, 5, 14, 6, (140, 24, 34)), s.rect(6, 1, 9, 5, (200, 190, 170)),
                        s.rect(7, 0, 8, 2, (255, 200, 120)), s.rect(3, 9, 12, 9, (120, 20, 30))))

    # Linha 3 (metade de cima de objetos altos) e linha 4 (base)
    def arvore_topo(s):
        for (x, y) in [(7, 15), (7, 14), (7, 13), (6, 12), (5, 11), (4, 10), (8, 12), (9, 11), (10, 10), (11, 9), (7, 12), (7, 11),
                       (7, 10), (6, 9), (5, 8), (8, 9), (9, 8), (3, 9), (12, 8), (7, 9), (7, 8), (6, 7), (8, 6), (10, 7), (4, 6)]:
            s.set(x, y, (46, 36, 34))
    deco(24, arvore_topo)
    deco(25, lambda s: (s.rect(4, 4, 11, 15, (96, 92, 104)), s.rect(3, 2, 12, 4, (120, 116, 128)), s.rect(5, 5, 5, 15, (70, 66, 80))))
    deco(26, lambda s: (s.rect(7, 6, 8, 15, (40, 40, 48)), s.rect(5, 2, 10, 6, (40, 40, 48)), s.rect(6, 3, 9, 5, (255, 200, 110))))
    deco(27, lambda s: (s.rect(5, 2, 10, 8, (130, 130, 140)), s.rect(4, 8, 11, 15, (120, 120, 130)), s.rect(6, 4, 6, 4, (40, 40, 50)),
                        s.rect(9, 4, 9, 4, (40, 40, 50)), s.rect(2, 9, 3, 13, (120, 120, 130)), s.rect(12, 9, 13, 13, (120, 120, 130))))
    deco(29, lambda s: [s.set(x, y, (60 + (x * 7) % 30, 80, 56)) for x in range(3, 13) for y in range(8, 15)
                        if (x - 8) ** 2 + (y - 12) ** 2 < 22 and (x + y) % 3])
    deco(30, lambda s: (s.rect(4, 11, 6, 12, (210, 200, 230)), s.rect(5, 13, 5, 15, (180, 170, 160)),
                        s.rect(9, 9, 12, 10, (170, 150, 220)), s.rect(10, 11, 11, 15, (180, 170, 160))))
    deco(31, lambda s: (s.rect(7, 9, 8, 15, (220, 210, 190)), s.rect(7, 7, 8, 8, (255, 200, 100)), s.set(7, 6, (255, 240, 200))))
    deco(32, lambda s: (s.rect(6, 0, 9, 15, (50, 40, 36)), s.rect(4, 13, 11, 15, (40, 32, 30)), s.rect(7, 0, 7, 12, (66, 54, 48))))
    deco(33, lambda s: (s.rect(4, 0, 11, 13, (96, 92, 104)), s.rect(3, 12, 12, 15, (120, 116, 128)), s.rect(5, 0, 5, 12, (70, 66, 80)),
                        s.rect(9, 4, 9, 7, (60, 56, 70))))
    deco(34, lambda s: (s.rect(7, 0, 8, 15, (40, 40, 48)), s.rect(5, 13, 10, 15, (40, 40, 48))))
    deco(35, lambda s: (s.rect(5, 0, 10, 10, (120, 120, 130)), s.rect(3, 10, 12, 15, (90, 90, 100)), s.rect(4, 11, 11, 11, (130, 130, 140))))
    deco(36, lambda s: (s.rect(1, 8, 14, 10, (110, 76, 46)), s.rect(2, 11, 3, 14, (80, 56, 34)), s.rect(12, 11, 13, 14, (80, 56, 34))))
    deco(37, lambda s: (s.rect(2, 5, 13, 15, (90, 90, 100)), s.rect(4, 6, 11, 11, (10, 12, 30)), s.rect(2, 4, 13, 5, (120, 120, 130)),
                        s.rect(2, 1, 3, 5, (70, 50, 40)), s.rect(12, 1, 13, 5, (70, 50, 40)), s.rect(2, 1, 13, 1, (70, 50, 40))))
    ox, oy = tile(38)                                 # grade da cripta (parede)
    pedras(38, (44, 42, 52), (26, 24, 32), (8, 8))
    for x in range(1, T, 3):
        img.rect(ox + x, oy, ox + x, oy + 15, (20, 20, 24))
    ox, oy = tile(39)
    for y in range(0, T, 4):
        img.rect(ox, oy + y, ox + 15, oy + y + 2, escurecer((60, 58, 66), 1 - y / 24))
    ampliar(img, 2).salvar(os.path.join(RAIZ, "tilesets", "gotico.png"))   # 32 px por tile




def ampliar(img, k):
    """Amplia por vizinho mais próximo (pixel art)."""
    out = Imagem(img.w * k, img.h * k)
    for y in range(img.h):
        for x in range(img.w):
            p = img.px[y * img.w + x]
            if p[3]:
                out.rect(x * k, y * k, x * k + k - 1, y * k + k - 1, p)
    return out


# ----------------------------------------------------------------------------- PRIMITIVAS
def linha(img, x0, y0, x1, y1, cor, esp=1):
    """Linha com espessura (Bresenham + pincel quadrado)."""
    x0, y0, x1, y1 = int(round(x0)), int(round(y0)), int(round(x1)), int(round(y1))
    dx, dy = abs(x1 - x0), -abs(y1 - y0)
    sx, sy = (1 if x0 < x1 else -1), (1 if y0 < y1 else -1)
    err = dx + dy
    r0, r1 = -(esp // 2), (esp - 1) // 2
    while True:
        img.rect(x0 + r0, y0 + r0, x0 + r1, y0 + r1, cor)
        if x0 == x1 and y0 == y1:
            break
        e2 = 2 * err
        if e2 >= dy:
            err += dy
            x0 += sx
        if e2 <= dx:
            err += dx
            y0 += sy


def disco(img, cx, cy, rx, ry, cor):
    for y in range(int(cy - ry - 1), int(cy + ry + 2)):
        for x in range(int(cx - rx - 1), int(cx + rx + 2)):
            if ((x + 0.5 - cx) / max(rx, 0.5)) ** 2 + ((y + 0.5 - cy) / max(ry, 0.5)) ** 2 <= 1:
                img.set(x, y, cor)


def poligono(img, pontos, cor):
    """Preenche um polígono convexo ou côncavo simples (scanline)."""
    ys = [p[1] for p in pontos]
    for y in range(int(min(ys)), int(max(ys)) + 1):
        cy = y + 0.5
        xs = []
        for i in range(len(pontos)):
            (x0, y0), (x1, y1) = pontos[i], pontos[(i + 1) % len(pontos)]
            if (y0 <= cy < y1) or (y1 <= cy < y0):
                xs.append(x0 + (cy - y0) * (x1 - x0) / (y1 - y0))
        xs.sort()
        for a, b in zip(xs[::2], xs[1::2]):
            for x in range(int(round(a)), int(round(b))):
                img.set(x, y, cor)


def girar(img, graus, px, py):
    """Gira a imagem em torno do pivô (amostragem inversa, vizinho mais próximo)."""
    out = Imagem(img.w, img.h)
    a = math.radians(graus)
    c, s = math.cos(a), math.sin(a)
    for y in range(img.h):
        for x in range(img.w):
            dx, dy = x + 0.5 - px, y + 0.5 - py
            sx, sy = c * dx + s * dy + px, -s * dx + c * dy + py
            p = img.get(int(math.floor(sx)), int(math.floor(sy)))
            if p[3]:
                out.set(x, y, p)
    return out


def contorno(img, cor=(12, 8, 14)):
    marcar = []
    for y in range(img.h):
        for x in range(img.w):
            if img.get(x, y)[3]:
                continue
            if any(img.get(x + dx, y + dy)[3] for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                marcar.append((x, y))
    for x, y in marcar:
        img.set(x, y, cor)


# ----------------------------------------------------------------------------- POSE
# Quadro 48x64; pés (âncora) em (24, 62). Figura ~55 px (estilo chibi das referências).
L, A = 48, 64
ANCORA = (24.0, 62.0)


class Pose:
    """Tudo que as camadas precisam para desenhar o MESMO quadro: corpo, roupa e arma
    leem esta pose, por isso ficam sincronizadas sem conhecer umas às outras."""

    def __init__(self, direcao, acao, f, n):
        self.d, self.acao, self.f, self.n = direcao, acao, f, n
        fase = f / max(1, n)
        self.dx = 0          # inclinação horizontal (para frente = +)
        self.dy = 0          # sobe/desce o corpo inteiro
        self.passo = 0.0     # -1..1 ciclo das pernas
        self.angulo = 30.0   # direção da arma, relativa à frente (graus)
        self.bracoFrente = (0, 0)   # deslocamento da mão da arma
        self.bracoTras = (0, 0)     # deslocamento da outra mão
        self.levanta = False        # braço de trás erguido (conjurar)
        self.mirando = False
        if acao == "ocioso":
            self.dy = 1 if f in (2, 3) else 0
        elif acao == "andar":
            self.passo = math.sin(fase * 2 * math.pi)
            self.dy = -1 if f % 3 == 1 else 0
        elif acao.startswith("ataque"):
            # Preparação para trás/cima, golpe atravessando, final baixo.
            curvas = {"ataque1": [-130, -60, 50, 70], "ataque2": [100, 40, -70, -90], "ataque3": [-175, -120, 60, 85]}
            self.angulo = curvas[acao][min(f, 3)]
            self.dx = [-1, 1, 3, 2][min(f, 3)] + (2 if acao == "ataque3" and f >= 2 else 0)
            self.dy = 1 if (acao == "ataque3" and f >= 2) else 0
            self.bracoFrente = [(-2, -4), (1, -2), (4, 1), (3, 2)][min(f, 3)]
        elif acao in ("mirar", "disparo"):
            self.mirando = True
            self.angulo = 0
            self.bracoFrente = (4, -2)
            self.bracoTras = (5, -2)
            if acao == "disparo":
                self.dx = [-1, -2, 0][min(f, 2)]
        elif acao == "conjurar":
            self.levanta = True
            self.dy = -1 if f >= 1 else 0
            self.angulo = 70
        elif acao == "esquiva":
            self.dx, self.dy = 3, 2
            self.angulo = 150
        elif acao == "dano":
            self.dx, self.dy = -2, 0
            self.angulo = 110
        elif acao == "vitoria":
            self.angulo = -90 if self.d != "cima" else 90
            self.bracoFrente = (0, -9)
            self.dy = -1 if f % 2 else 0

    # Ângulo absoluto (tela, y para baixo) da frente do personagem.
    def frente(self):
        return {"direita": 0.0, "baixo": 90.0, "cima": -90.0}[self.d]

    def angulo_arma(self):
        a = self.frente() + self.angulo
        # De frente/de costas a lâmina varre para o lado da mão, não para dentro do corpo.
        if self.d == "baixo":
            a = 90 - self.angulo * 0.8
        elif self.d == "cima":
            a = -90 + self.angulo * 0.8
        return a

    def ox(self):
        return self.dx if self.d == "direita" else 0

    def oy(self):
        return self.dy + (abs(self.dx) // 2 if self.d != "direita" else 0)

    def mao_arma(self):
        """Posição da mão que segura a arma (px do quadro)."""
        bx, by = self.bracoFrente
        if self.d == "direita":
            return (29 + self.ox() + bx, 42 + self.oy() + by)
        if self.d == "baixo":
            return (33 + bx // 2, 43 + self.oy() + by)
        return (14 - bx // 2, 43 + self.oy() + by)       # de costas: mão direita aparece à esquerda

    def mao_escudo(self):
        bx, by = self.bracoTras
        if self.levanta:
            return {"direita": (22 + self.ox(), 26 + self.oy()), "baixo": (13, 27 + self.oy()), "cima": (34, 27 + self.oy())}[self.d]
        if self.d == "direita":
            return (19 + self.ox() + bx, 42 + self.oy() + by)
        if self.d == "baixo":
            return (14 - bx // 3, 43 + self.oy() + by)
        return (33, 43 + self.oy())


# ----------------------------------------------------------------------------- CAMADAS
def pele_e_cabelo_padrao():
    return dict(pele=(236, 204, 184), pele2=(206, 168, 150), cabelo=(226, 230, 240), cabelo2=(168, 176, 196),
                roupa=(58, 56, 66), roupa2=(40, 38, 48), calca=(42, 38, 46), bota=(70, 46, 34), olhos=(40, 40, 64),
                estilo="longo", curvado=False)


def camada_corpo(img, p, pal):
    """Corpo base: pernas, botas, camisa simples, braços, cabeça e cabelo."""
    ox, oy = p.ox(), p.oy() + (3 if pal["curvado"] else 0)
    lado = p.d == "direita"
    # --- pernas e botas ---
    if lado:
        s = p.passo
        for k, (cx, desl) in enumerate(((22, s), (26, -s))):
            x = cx + ox + int(round(desl * 3))
            levanta = int(max(0.0, desl) * 2)
            cor = pal["calca"] if k == 1 else escurecer(pal["calca"], 0.8)
            img.rect(x - 2, 45 + oy, x + 1, 56 + oy - levanta, cor)
            img.rect(x - 2, 56 + oy - levanta, x + 3, 61 - levanta, pal["bota"])
    else:
        s = p.passo
        for cx, lev in ((21, max(0.0, s)), (27, max(0.0, -s))):
            l = int(lev * 2)
            img.rect(cx - 2, 45 + oy, cx + 1, 56 + oy - l, pal["calca"])
            img.rect(cx - 2, 56 + oy - l, cx + 2, 61 - l, pal["bota"])
    # --- tronco ---
    tx0, tx1 = (19, 29) if lado else (17, 30)
    img.rect(tx0 + ox, 29 + oy, tx1 + ox, 46 + oy, pal["roupa"])
    img.rect(tx0 + ox, 29 + oy, tx0 + 1 + ox, 46 + oy, pal["roupa2"])
    img.rect(tx0 + ox, 43 + oy, tx1 + ox, 44 + oy, escurecer(pal["roupa2"], 0.7))   # cinto
    # --- braços (tras primeiro, mais escuro) ---
    mx, my = p.mao_escudo()
    ombro_t = (20 + ox, 31 + oy) if lado else ((15, 31 + oy) if p.d == "baixo" else (32, 31 + oy))
    linha(img, ombro_t[0], ombro_t[1], mx, my, escurecer(pal["roupa"], 0.75), 4)
    img.rect(mx - 1, my - 1, mx + 1, my + 1, pal["pele2"])
    ax, ay = p.mao_arma()
    ombro_f = (27 + ox, 31 + oy) if lado else ((32, 31 + oy) if p.d == "baixo" else (15, 31 + oy))
    linha(img, ombro_f[0], ombro_f[1], ax, ay, pal["roupa"], 4)
    img.rect(ax - 1, ay - 1, ax + 1, ay + 1, pal["pele"])
    # --- cabeça (grande, estilo chibi) ---
    hx0, hx1 = (17, 31) if lado else (15, 32)
    hy = 11 + oy
    if pal["curvado"]:
        hx0, hx1 = hx0 + 3, hx1 + 3
    img.rect(hx0 + ox, hy + 2, hx1 + ox, hy + 17, pal["pele"])
    img.rect(hx0 + 1 + ox, hy + 1, hx1 - 1 + ox, hy + 18, pal["pele"])
    estilo = pal["estilo"]
    c1, c2 = pal["cabelo"], pal["cabelo2"]
    if estilo == "longo":
        img.rect(hx0 - 1 + ox, hy - 1, hx1 + 1 + ox, hy + 5, c1)          # topo
        img.rect(hx0 + 1 + ox, hy - 3, hx1 - 1 + ox, hy - 1, c1)
        if p.d == "cima":
            img.rect(hx0 - 1 + ox, hy - 1, hx1 + 1 + ox, hy + 24, c1)     # cabelo cobre as costas
            for x in range(hx0 + ox, hx1 + 1 + ox, 3):
                img.rect(x, hy + 6, x, hy + 23, c2)
        elif lado:
            img.rect(hx0 - 2 + ox, hy - 1, hx0 + 6 + ox, hy + 25, c1)     # mechas para trás
            img.rect(hx0 - 1 + ox, hy + 8, hx0 + 2 + ox, hy + 24, c2)
            img.rect(hx1 - 5 + ox, hy + 3, hx1 + 1 + ox, hy + 6, c1)      # franja
        else:
            img.rect(hx0 - 2 + ox, hy + 2, hx0 + 1 + ox, hy + 25, c1)     # laterais longas
            img.rect(hx1 - 1 + ox, hy + 2, hx1 + 2 + ox, hy + 25, c1)
            img.rect(hx0 - 1 + ox, hy + 10, hx0 + ox, hy + 24, c2)
            img.rect(hx1 + ox, hy + 10, hx1 + 1 + ox, hy + 24, c2)
            for x in range(hx0 + 2 + ox, hx1 - 1 + ox, 3):                # franja repicada
                img.rect(x, hy + 5, x + 1, hy + 7, c1)
    elif estilo == "curto":
        img.rect(hx0 - 1 + ox, hy - 1, hx1 + 1 + ox, hy + 5, c1)
        if p.d == "cima":
            img.rect(hx0 - 1 + ox, hy - 1, hx1 + 1 + ox, hy + 14, c1)
        if pal.get("barba") and p.d != "cima":
            img.rect(hx0 + 2 + ox, hy + 13, hx1 - 2 + ox, hy + 19, c2)
    elif estilo == "careca":
        img.rect(hx0 + ox, hy + 2, hx1 + ox, hy + 3, escurecer(pal["pele"], 0.85))
    elif estilo == "capuz":
        cor = pal["capuz"]
        img.rect(hx0 - 2 + ox, hy - 2, hx1 + 2 + ox, hy + 20, cor)
        img.rect(hx0 + ox, hy - 4, hx1 + ox, hy - 2, cor)
        if pal.get("ponta"):
            img.rect((hx0 + hx1) // 2 - 1 + ox, hy - 8, (hx0 + hx1) // 2 + 1 + ox, hy - 4, cor)
        if p.d != "cima":
            face0 = hx0 + 2 + (3 if lado else 0)
            img.rect(face0 + ox, hy + 4, hx1 - 2 + ox, hy + 18, pal["pele"])
    elif estilo == "elmo":
        aco, aco2 = (150, 156, 170), (100, 106, 120)
        img.rect(hx0 - 1 + ox, hy - 2, hx1 + 1 + ox, hy + 11, aco)
        img.rect(hx0 - 1 + ox, hy + 7, hx1 + 1 + ox, hy + 8, aco2)
        img.rect((hx0 + hx1) // 2 - 1 + ox, hy - 6, (hx0 + hx1) // 2 + 1 + ox, hy - 2, (170, 30, 40))
    # --- olhos ---
    if p.d != "cima":
        o = pal["olhos"]
        if lado:
            img.rect(hx1 - 3 + ox, hy + 10, hx1 - 2 + ox, hy + 12, o)
        else:
            img.rect(hx0 + 4 + ox, hy + 10, hx0 + 5 + ox, hy + 12, o)
            img.rect(hx1 - 5 + ox, hy + 10, hx1 - 4 + ox, hy + 12, o)


def camada_roupa(img, p, estilo, c1, c2, detalhe):
    """Armadura/túnica por cima do tronco e dos braços."""
    ox, oy = p.ox(), p.oy()
    lado = p.d == "direita"
    tx0, tx1 = (18, 30) if lado else (16, 31)
    if estilo == "sobretudo":
        img.rect(tx0 + ox, 28 + oy, tx1 + ox, 47 + oy, c1)
        # abas até o joelho, abertas na frente (forro carmesim)
        poligono(img, [(tx0 + ox, 46 + oy), (tx1 + ox + 1, 46 + oy), (tx1 + ox + 3, 54 + oy), (tx0 + ox - 2, 54 + oy)], c1)
        if p.d == "baixo":
            img.rect(23, 33 + oy, 24, 54 + oy, detalhe)                   # forro na abertura
            for y in range(33, 45, 4):
                img.set(21, y + oy, (190, 196, 210))                      # botões de prata
                img.set(26, y + oy, (190, 196, 210))
        elif lado:
            img.rect(tx0 + ox - 2, 50 + oy, tx0 + ox, 54 + oy, detalhe)
        img.rect(tx0 + ox, 27 + oy, tx1 + ox, 29 + oy, c2)                # gola alta
    elif estilo == "cavaleiro":
        img.rect(tx0 + ox, 29 + oy, tx1 + ox, 45 + oy, c1)
        img.rect(tx0 + 2 + ox, 31 + oy, tx1 - 2 + ox, 33 + oy, (200, 206, 220))   # brilho do peitoral
        img.rect(tx0 + ox, 44 + oy, tx1 + ox, 45 + oy, detalhe)                    # cinto dourado
        for k in range(3):                                                         # escarcelas
            img.rect(tx0 + 1 + k * 5 + ox, 46 + oy, tx0 + 4 + k * 5 + ox, 50 + oy, c2)
        # ombreiras grandes
        if lado:
            disco(img, 25 + ox, 31 + oy, 5, 4, c1)
        else:
            disco(img, tx0 - 1, 31 + oy, 4, 4, c1)
            disco(img, tx1 + 1, 31 + oy, 4, 4, c1)
        # grevas
        for cx in ((21, 27) if not lado else (22 + ox, 26 + ox)):
            img.rect(cx - 2, 51 + oy, cx + 1, 56 + oy, c2)
        # manoplas
        ax, ay = p.mao_arma()
        img.rect(ax - 2, ay - 2, ax + 1, ay + 1, c2)
    elif estilo == "tunica":
        poligono(img, [(tx0 + ox, 28 + oy), (tx1 + ox + 1, 28 + oy), (tx1 + ox + 4, 59 + oy), (tx0 + ox - 3, 59 + oy)], c1)
        img.rect(tx0 + ox - 3, 57 + oy, tx1 + ox + 4, 58 + oy, detalhe)   # barra dourada
        img.rect(tx0 + ox, 43 + oy, tx1 + ox, 44 + oy, detalhe)           # faixa
        if p.d == "baixo":
            img.rect(23, 29 + oy, 24, 56 + oy, detalhe)
            for y in range(34, 54, 6):
                img.set(21, y + oy, (220, 200, 255))                      # runas
                img.set(26, y + 3 + oy, (220, 200, 255))
        img.rect(tx0 + ox, 28 + oy, tx1 + ox, 29 + oy, c2)
    elif estilo == "avental":
        img.rect(tx0 + 2 + ox, 33 + oy, tx1 - 2 + ox, 53 + oy, c1)
        img.rect(tx0 + 2 + ox, 33 + oy, tx1 - 2 + ox, 34 + oy, c2)
    elif estilo == "trapos":
        for k in range(tx0, tx1 + 1, 2):
            img.rect(k + ox, 44 + oy, k + ox, 47 + oy + (k % 3), c1)
        img.rect(tx0 + ox, 38 + oy, tx1 + ox, 44 + oy, c2)
    # mangas por cima dos braços
    if estilo in ("sobretudo", "tunica", "cavaleiro"):
        ax, ay = p.mao_arma()
        ombro_f = (27 + ox, 31 + oy) if lado else ((32, 31 + oy) if p.d == "baixo" else (15, 31 + oy))
        manga = c1 if estilo != "cavaleiro" else c2
        meio = ((ombro_f[0] + ax) // 2, (ombro_f[1] + ay) // 2)
        linha(img, ombro_f[0], ombro_f[1], meio[0], meio[1], manga, 5 if estilo == "tunica" else 4)


def camada_capa(img, p, c1, c2):
    ox, oy = p.ox(), p.oy()
    balanco = int(round(p.passo * 2)) + (2 if p.acao in ("esquiva", "andar") else 0)
    if p.d == "cima":
        poligono(img, [(16, 29 + oy), (32, 29 + oy), (35, 58 + oy), (13, 58 + oy)], c1)       # cobre as costas
        for x in (19, 24, 29):
            img.rect(x, 33 + oy, x, 57 + oy, c2)
    elif p.d == "direita":
        poligono(img, [(20 + ox, 29 + oy), (26 + ox, 29 + oy), (22 + ox, 57 + oy), (12 + ox - balanco, 56 + oy)], c1)
        img.rect(14 + ox - balanco, 50 + oy, 18 + ox, 55 + oy, c2)
    else:
        poligono(img, [(15, 30 + oy), (33, 30 + oy), (36, 58 + oy), (12, 58 + oy)], c1)       # aparece nas bordas
        img.rect(12, 55 + oy, 36, 58 + oy, c2)


def desenhar_arma(img, tipo, mao, ang, p):
    hx, hy = mao
    a = math.radians(ang)
    ux, uy = math.cos(a), math.sin(a)
    px, py = -uy, ux                       # perpendicular
    aco, aco2, madeira, ouro = (214, 222, 236), (130, 138, 156), (110, 74, 44), (210, 172, 80)

    def ponto(t, s=0.0):
        return (hx + ux * t + px * s, hy + uy * t + py * s)

    if tipo in ("espada", "adaga", "lanca"):
        comp = {"espada": 17, "adaga": 8, "lanca": 24}[tipo]
        linha(img, *ponto(-3), *ponto(0), madeira, 2)                                # cabo
        linha(img, *ponto(1, -3), *ponto(1, 3), ouro if tipo != "lanca" else madeira, 2)  # guarda
        linha(img, *ponto(2), *ponto(comp), aco, 2 if tipo != "lanca" else 1)
        if tipo == "lanca":
            poligono(img, [ponto(comp - 1, -3), ponto(comp + 5), ponto(comp - 1, 3)], aco)
        else:
            linha(img, *ponto(3, 1), *ponto(comp - 1, 1), aco2, 1)
    elif tipo == "machado":
        linha(img, *ponto(-4), *ponto(13), madeira, 2)
        poligono(img, [ponto(8, 0), ponto(9, 7), ponto(14, 8), ponto(14, 0)], aco)
    elif tipo == "foice":
        linha(img, *ponto(-6), *ponto(18), (70, 52, 40), 2)
        curva = [ponto(18 - k * 0.5, k * 1.4) for k in range(9)]
        for (x0, y0), (x1, y1) in zip(curva, curva[1:]):
            linha(img, x0, y0, x1, y1, aco, 2)
    elif tipo == "bastao":
        linha(img, *ponto(-8), *ponto(16), madeira, 2)
        cx, cy = ponto(18)
        disco(img, cx, cy, 2.5, 2.5, (150, 90, 230))
        img.set(int(cx), int(cy) - 1, (230, 210, 255))
    elif tipo == "martelo":
        linha(img, *ponto(-3), *ponto(10), madeira, 2)
        poligono(img, [ponto(9, -3), ponto(9, 3), ponto(13, 3), ponto(13, -3)], (90, 94, 104))
    elif tipo == "garras":
        for s in (-2, 0, 2):
            linha(img, *ponto(0, s), *ponto(5, s * 1.4), (230, 226, 200), 1)
    elif tipo == "arco":
        # Arco perpendicular à mira; mirando, a corda vai até a mão de trás.
        arco = [ponto(2 + 3 * math.sin(k / 8 * math.pi), (k - 4) * 2.4) for k in range(9)]
        for (x0, y0), (x1, y1) in zip(arco, arco[1:]):
            linha(img, x0, y0, x1, y1, madeira, 2)
        topo, base = arco[0], arco[-1]
        corda = ponto(-3 if p.mirando else 2)
        linha(img, *topo, *corda, (230, 230, 220), 1)
        linha(img, *corda, *base, (230, 230, 220), 1)
    elif tipo == "besta":
        linha(img, *ponto(-4), *ponto(9), madeira, 3)
        arco = [ponto(9, -7), ponto(11, 0), ponto(9, 7)]
        linha(img, *arco[0], *arco[1], aco2, 2)
        linha(img, *arco[1], *arco[2], aco2, 2)
        linha(img, *arco[0], *ponto(6), (230, 230, 220), 1)
        linha(img, *arco[2], *ponto(6), (230, 230, 220), 1)
    elif tipo == "bumerangue":
        linha(img, *ponto(0, 0), *ponto(6, -5), (226, 216, 190), 3)
        linha(img, *ponto(0, 0), *ponto(6, 5), (226, 216, 190), 3)


def camada_arma(img, p, tipo):
    ang = p.angulo_arma()
    if tipo in ("arco", "besta", "bumerangue") and not p.mirando:
        ang = p.frente() + 70 if p.d != "baixo" else 100   # carregada apontando para baixo
    desenhar_arma(img, tipo, p.mao_arma(), ang, p)


def camada_escudo(img, p):
    mx, my = p.mao_escudo()
    if p.d == "baixo":
        disco(img, mx, my - 2, 6, 7, (150, 156, 170))
        disco(img, mx, my - 2, 4.5, 5.5, (110, 40, 46))
        disco(img, mx, my - 2, 2, 2, (210, 172, 80))                      # leão (medalhão)
    elif p.d == "direita":
        disco(img, mx - 1, my - 2, 2.5, 7, (120, 126, 140))
        img.rect(int(mx) - 2, int(my) - 8, int(mx) - 1, int(my) + 4, (210, 172, 80))
    else:
        disco(img, mx, my - 2, 6, 7, (90, 70, 52))                        # parte de trás (madeira)
        img.rect(int(mx) - 1, int(my) - 8, int(mx), int(my) + 4, (60, 46, 34))


# ----------------------------------------------------------------------------- CATÁLOGO DE CAMADAS
def cor_roupa(estilo):
    return {
        "sobretudo": ((34, 30, 40), (22, 20, 28), (140, 26, 40)),
        "cavaleiro": ((150, 156, 170), (100, 106, 120), (210, 172, 80)),
        "tunica": ((96, 56, 150), (66, 38, 108), (210, 180, 90)),
    }[estilo]


def fabrica(desenho):
    """Transforma uma função (img, pose) numa camada com contorno."""
    def f(pose):
        img = Imagem(L, A)
        desenho(img, pose)
        contorno(img)
        return img
    return f


CAMADAS_EQUIP = {
    "sobretudo_gotico": fabrica(lambda img, p: camada_roupa(img, p, "sobretudo", *cor_roupa("sobretudo"))),
    "armadura_cavaleiro": fabrica(lambda img, p: camada_roupa(img, p, "cavaleiro", *cor_roupa("cavaleiro"))),
    "tunica_mago": fabrica(lambda img, p: camada_roupa(img, p, "tunica", *cor_roupa("tunica"))),
    "capa_carmesim": fabrica(lambda img, p: camada_capa(img, p, (122, 22, 34), (84, 12, 22))),
    "escudo_leao": fabrica(camada_escudo),
    "espada_prata": fabrica(lambda img, p: camada_arma(img, p, "espada")),
    "adagas_gemeas": fabrica(lambda img, p: camada_arma(img, p, "adaga")),
    "machado_carrasco": fabrica(lambda img, p: camada_arma(img, p, "machado")),
    "foice_ceifador": fabrica(lambda img, p: camada_arma(img, p, "foice")),
    "bastao_arcano": fabrica(lambda img, p: camada_arma(img, p, "bastao")),
    "arco_teixo": fabrica(lambda img, p: camada_arma(img, p, "arco")),
    "besta_cacador": fabrica(lambda img, p: camada_arma(img, p, "besta")),
    "bumerangue_osso": fabrica(lambda img, p: camada_arma(img, p, "bumerangue")),
}


def personagem_composto(pal, roupa=None, capa=None, arma=None, escudo=False):
    """NPCs e inimigos: as mesmas camadas, achatadas num atlas só."""
    def f(pose):
        img = Imagem(L, A)
        partes = []
        if capa and pose.d != "cima":
            partes.append(lambda i: camada_capa(i, pose, *capa))
        if arma and pose.d == "cima":
            partes.append(lambda i: camada_arma(i, pose, arma))
        partes.append(lambda i: camada_corpo(i, pose, pal))
        if roupa:
            partes.append(lambda i: camada_roupa(i, pose, *roupa))
        if capa and pose.d == "cima":
            partes.append(lambda i: camada_capa(i, pose, *capa))
        if escudo:
            partes.append(lambda i: camada_escudo(i, pose))
        if arma and pose.d != "cima":
            partes.append(lambda i: camada_arma(i, pose, arma))
        for desenhar_parte in partes:
            camada = Imagem(L, A)
            desenhar_parte(camada)
            contorno(camada)
            img.colar(camada, 0, 0)
        return img
    return f


def pal(**k):
    base = pele_e_cabelo_padrao()
    base.update(k)
    return base


NPCS_E_INIMIGOS = {
    "ferreiro": personagem_composto(pal(pele=(204, 150, 112), pele2=(170, 120, 90), cabelo=(86, 56, 38), cabelo2=(64, 42, 28),
                                        estilo="curto", barba=True, roupa=(120, 84, 60), roupa2=(90, 62, 44)),
                                    roupa=("avental", (96, 64, 40), (70, 46, 30), None), arma="martelo"),
    "curandeira": personagem_composto(pal(pele=(232, 212, 198), cabelo=(60, 44, 40), estilo="capuz", capuz=(196, 190, 176),
                                          roupa=(186, 180, 166), roupa2=(140, 134, 122)),
                                      roupa=("tunica", (196, 190, 176), (150, 144, 132), (130, 40, 50)), arma="bastao"),
    "vigia": personagem_composto(pal(pele=(210, 170, 140), estilo="elmo"), roupa=("cavaleiro", *cor_roupa("cavaleiro")),
                                 capa=((90, 40, 30), (60, 26, 20)), arma="lanca"),
    "cultista": personagem_composto(pal(pele=(22, 12, 16), pele2=(16, 8, 12), olhos=(255, 90, 90), estilo="capuz", ponta=True,
                                        capuz=(120, 20, 32), roupa=(110, 18, 30), roupa2=(80, 12, 22), calca=(70, 10, 18)),
                                    roupa=("tunica", (120, 20, 32), (84, 12, 22), (210, 170, 80))),
    "carnical": personagem_composto(pal(pele=(128, 140, 110), pele2=(100, 110, 86), olhos=(250, 230, 90), estilo="careca",
                                        curvado=True, roupa=(128, 140, 110), roupa2=(100, 110, 86), calca=(70, 62, 52),
                                        bota=(128, 140, 110)),
                                    roupa=("trapos", (80, 70, 60), (60, 52, 44), None), arma="garras"),
}

# Clipes: nome -> (quadros, duração ms, loop, eventos {quadro: nome})
CLIPES_HEROI = {
    "ocioso": (4, 220, True, {}),
    "andar": (6, 100, True, {1: "step_l", 4: "step_r"}),
    "ataque1": (4, 70, False, {2: "hit"}),
    "ataque2": (4, 70, False, {2: "hit"}),
    "ataque3": (4, 100, False, {2: "hit"}),
    "mirar": (2, 200, True, {}),
    "disparo": (3, 70, False, {1: "release"}),
    "conjurar": (4, 70, False, {2: "release"}),
    "esquiva": (3, 70, False, {}),
    "dano": (2, 110, False, {}),
    "vitoria": (4, 160, True, {}),
}
CLIPES_NPC = {"ocioso": (4, 260, True, {})}
CLIPES_INIMIGO = {
    "ocioso": (4, 220, True, {}),
    "andar": (6, 110, True, {}),
    "ataque1": (4, 90, False, {2: "hit"}),
    "conjurar": (4, 110, False, {2: "release"}),
    "dano": (2, 110, False, {}),
}
# "esquerda" NÃO é desenhada: o AnimationController espelha "direita" em torno da âncora.
DIRECOES = ("baixo", "cima", "direita")
ANGULOS_MORTE = [0, 25, 55, 80, 90]


def exportar(pasta, nome, desenho, clipes, com_morte):
    """Atlas no formato do agent-sprite-forge (export_engine.py write_aseprite)."""
    quadros, tags, animacoes = [], [], {}

    def adicionar_clipe(nome_clipe, imgs, ms, loop, eventos):
        inicio = len(quadros)
        quadros.extend((im, ms) for im in imgs)
        tag = {"name": nome_clipe, "from": inicio, "to": len(quadros) - 1, "direction": "forward", "color": "#000000ff"}
        if not loop:
            tag["repeat"] = "1"
        if eventos:
            tag["data"] = json.dumps({"events": [{"name": ev, "frame": inicio + q, "at_ms": q * ms} for q, ev in eventos.items()]},
                                     separators=(",", ":"))
        tags.append(tag)
        animacoes[nome_clipe] = [str(i) for i in range(inicio, len(quadros))]

    for acao, (n, ms, loop, eventos) in clipes.items():
        for d in DIRECOES:
            adicionar_clipe(f"{acao}_{d}", [desenho(Pose(d, acao, f, n)) for f in range(n)], ms, loop, eventos)
    if com_morte:
        base = Pose("direita", "dano", 1, 2)
        adicionar_clipe("morte", [girar(desenho(base), -g, *ANCORA) for g in ANGULOS_MORTE], 120, False, {})

    pad, colunas = 2, 12
    linhas = math.ceil(len(quadros) / colunas)
    atlas = Imagem(colunas * (L + pad), linhas * (A + pad))
    frames = []
    for i, (im, ms) in enumerate(quadros):
        x, y = (i % colunas) * (L + pad), (i // colunas) * (A + pad)
        atlas.colar(im, x, y)
        frames.append({"filename": str(i), "frame": {"x": x, "y": y, "w": L, "h": A}, "rotated": False, "trimmed": False,
                       "spriteSourceSize": {"x": 0, "y": 0, "w": L, "h": A}, "sourceSize": {"w": L, "h": A}, "duration": ms})
    destino = os.path.join(RAIZ, pasta)
    os.makedirs(destino, exist_ok=True)
    atlas.salvar(os.path.join(destino, f"{nome}.png"))
    doc = {
        "frames": frames,
        "meta": {
            "app": "agent-sprite-forge export_engine.py (placeholder: tools/gerar_placeholders.py)", "version": "placeholder-2",
            "image": f"{nome}.png", "format": "RGBA8888", "size": {"w": atlas.w, "h": atlas.h}, "scale": "1",
            "frameTags": tags,
            "layers": [{"name": "sprite", "opacity": 255, "blendMode": "normal"}],
            "slices": [{"name": "anchor", "color": "#0000ffff", "data": json.dumps({"anchor_px": list(ANCORA)}, separators=(",", ":")),
                        "keys": [{"frame": 0, "bounds": {"x": 0, "y": 0, "w": L, "h": A}, "pivot": {"x": 24, "y": 62}}]}],
        },
        "animations": animacoes,
    }
    with open(os.path.join(destino, f"{nome}.json"), "w", encoding="utf-8") as f:
        json.dump(doc, f, indent=1)
    return len(quadros)


# ----------------------------------------------------------------------------- RETRATOS
PALETAS_RETRATO = {
    "protagonista": dict(cabelo=(222, 226, 236), cabelo2=(160, 168, 186), pele=(236, 204, 184), roupa=(34, 30, 40),
                         roupa2=(22, 20, 28), detalhe=(156, 28, 40)),
    "ferreiro": dict(cabelo=(80, 52, 36), cabelo2=(60, 40, 28), pele=(204, 150, 112), roupa=(96, 64, 40), roupa2=(70, 46, 30),
                     detalhe=(140, 96, 60)),
    "curandeira": dict(cabelo=(206, 200, 186), cabelo2=(160, 154, 140), pele=(230, 210, 196), roupa=(190, 184, 170),
                       roupa2=(140, 134, 122), detalhe=(120, 40, 50), capuz=True),
    "vigia": dict(cabelo=(140, 146, 158), cabelo2=(100, 104, 116), pele=(210, 170, 140), roupa=(86, 46, 34), roupa2=(60, 32, 24),
                  detalhe=(160, 160, 170), elmo=True),
}


def retrato(nome):
    p = PALETAS_RETRATO[nome]
    base = Imagem(24, 24)
    fundo = escurecer(p["roupa"], 0.35)
    for y in range(24):
        for x in range(24):
            d = math.hypot(x - 12, y - 10) / 16
            base.set(x, y, escurecer(fundo, 1.2 - d * 0.6))
    base.rect(3, 18, 20, 23, p["roupa"])
    base.rect(3, 18, 5, 23, p["roupa2"])
    base.rect(9, 15, 14, 18, p["pele"])
    for y in range(4, 17):
        for x in range(6, 18):
            if ((x - 11.5) / 5.6) ** 2 + ((y - 10.5) / 6.5) ** 2 <= 1:
                base.set(x, y, p["pele"])
    if p.get("capuz"):
        for y in range(1, 20):
            for x in range(3, 21):
                dentro = ((x - 11.5) / 8.5) ** 2 + ((y - 10) / 9.5) ** 2 <= 1
                rosto = ((x - 11.5) / 5.2) ** 2 + ((y - 11.5) / 6) ** 2 <= 1
                if dentro and not rosto:
                    base.set(x, y, p["roupa"])
    elif p.get("elmo"):
        base.rect(5, 2, 18, 8, p["cabelo"])
        base.rect(5, 6, 18, 6, p["cabelo2"])
        base.rect(11, 0, 12, 2, (170, 30, 40))
    else:
        for y in range(2, 10):
            for x in range(5, 19):
                if ((x - 11.5) / 7) ** 2 + ((y - 7) / 5) ** 2 <= 1:
                    base.set(x, y, p["cabelo"])
        base.rect(5, 8, 6, 19 if nome == "protagonista" else 15, p["cabelo2"])
        base.rect(17, 8, 18, 19 if nome == "protagonista" else 15, p["cabelo2"])
        if nome == "ferreiro":
            base.rect(8, 14, 15, 17, p["cabelo"])
    olho = (30, 26, 40)
    base.set(9, 11, olho); base.set(14, 11, olho)
    base.set(9, 10, escurecer(p["pele"], 0.7)); base.set(14, 10, escurecer(p["pele"], 0.7))
    base.rect(10, 14, 13, 14, escurecer(p["pele"], 0.6))
    if not p.get("capuz") and not p.get("elmo"):
        base.rect(7, 19, 16, 19, p["detalhe"])
    ampliar(base, 2).salvar(os.path.join(RAIZ, "retratos", f"{nome}.png"))


def main():
    tileset()
    # O tileset é desenhado em 16 px e ampliado para 32 px (tamanhoTileArte = 32 nos mapas).
    caminho = os.path.join(RAIZ, "tilesets", "gotico.png")
    total = exportar("sprites/protagonista", "protagonista", fabrica(lambda img, p: camada_corpo(img, p, pele_e_cabelo_padrao())),
                     CLIPES_HEROI, True)
    for item, desenho in CAMADAS_EQUIP.items():
        total += exportar(f"sprites/equip/{item}", item, desenho, CLIPES_HEROI, True)
    for npc in ("ferreiro", "curandeira", "vigia"):
        total += exportar(f"sprites/{npc}", npc, NPCS_E_INIMIGOS[npc], CLIPES_NPC, False)
        retrato(npc)
    retrato("protagonista")
    for inimigo in ("cultista", "carnical"):
        total += exportar(f"sprites/{inimigo}", inimigo, NPCS_E_INIMIGOS[inimigo], CLIPES_INIMIGO, True)
    print(f"placeholders gerados em {os.path.normpath(RAIZ)} ({total} quadros, {len(CAMADAS_EQUIP) + 1} camadas do herói)")
    _ = caminho


if __name__ == "__main__":
    main()

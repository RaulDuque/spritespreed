#!/usr/bin/env node
// Gera as sprite sheets de VFX do jogo a partir do pixelfx (third_party/pixelfx).
//
//   node tools/gerar_vfx.mjs                 todas as receitas de tools/vfx_receitas.json
//   node tools/gerar_vfx.mjs orbe_sangue     só as chaves informadas
//
// Saída: assets/vfx/<chave>.png + <chave>.json (atlas estilo Aseprite com meta.pixelfx: fps, loop,
// frameSize, pivot, act) e assets/vfx/indice.json, que o VFXManager (C++) lê na inicialização.

import { readFileSync, writeFileSync, mkdirSync, existsSync } from "node:fs";
import { join, dirname } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";

const RAIZ = join(dirname(fileURLToPath(import.meta.url)), "..");
const PIXELFX = join(RAIZ, "third_party", "pixelfx");
const SAIDA = join(RAIZ, "assets", "vfx");
const FPS_PADRAO = 15; // o jogo roda pixel art a 60 Hz, mas 15 fps de efeito mantém as sheets pequenas

const { loadEffect, renderFrames, spriteSheet, save } = await import(pathToFileURL(join(PIXELFX, "tools", "lib", "fxlib.mjs")).href);
const { PAL } = await import(pathToFileURL(join(PIXELFX, "src", "core", "color.js")).href);

const receitas = JSON.parse(readFileSync(join(RAIZ, "tools", "vfx_receitas.json"), "utf8")).efeitos;
const filtro = process.argv.slice(2);
const escolhidas = filtro.length ? receitas.filter((r) => filtro.includes(r.chave)) : receitas;
if (!escolhidas.length) {
  console.error(`nenhuma receita encontrada para: ${filtro.join(", ")}`);
  process.exit(2);
}

// Uma paleta pode vir pelo nome de um ramp PAL ("blood") ou como lista de cores hex.
function resolverOpcoes(opcoes = {}) {
  const o = { ...opcoes };
  if (typeof o.palette === "string") {
    if (!PAL[o.palette]) throw new Error(`paleta desconhecida: ${o.palette}`);
    o.palette = PAL[o.palette];
  }
  return o;
}

mkdirSync(SAIDA, { recursive: true });
const caminhoIndice = join(SAIDA, "indice.json");
const indice = existsSync(caminhoIndice) ? JSON.parse(readFileSync(caminhoIndice, "utf8")) : { efeitos: {} };
let falhas = 0;

for (const r of escolhidas) {
  try {
    const fx = await loadEffect(r.id);
    const quadros = renderFrames(fx, { bg: "none", actor: false, fps: r.fps ?? FPS_PADRAO, options: resolverOpcoes(r.opcoes) });
    const sheet = spriteSheet(fx, quadros, r.chave);
    save(join(SAIDA, `${r.chave}.png`), sheet.png);
    save(join(SAIDA, `${r.chave}.json`), JSON.stringify(sheet.json, null, 1));
    indice.efeitos[r.chave] = { id: r.id, json: `${r.chave}.json`, quadros: quadros.n, fps: quadros.fps, loop: !!fx.meta.loop };
    console.log(`${r.chave.padEnd(26)} <- ${r.id.padEnd(28)} ${String(quadros.n).padStart(4)} quadros ${quadros.w}x${quadros.h}`);
  } catch (e) {
    falhas++;
    console.error(`${r.chave}: FALHOU: ${e.stack ?? e}`);
  }
}

writeFileSync(caminhoIndice, JSON.stringify(indice, null, 1));
process.exit(falhas ? 1 : 0);

// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
//
// Checks of the interactive manual (web/manual). Run from the repo root:  node web/test_manual.mjs
// - data.js / nav.js / manual.js load without a browser; nav.js is what tools/gen_manual.py makes from the firmware
// - every control on the FM-1 drawing (8 knobs, 14 buttons, 27 keys) has an entry; every combo names real controls
// - every text in Spanish and English; every screen the page names is in shots/ (a real 240 x 240 PNG)
// - the search: Spanish and English, accents ignored, filtered by section, the right command first
// - the page in Chromium (Playwright, when installed): no error, the drawing drawn, a hover fills the panel, the search
//   filters, the English toggle, no horizontal scroll at 390 px; skipped without Playwright
import { execFileSync } from "node:child_process";
import { existsSync, readFileSync, readdirSync } from "node:fs";
import { createRequire } from "node:module";
import { join } from "node:path";
import vm from "node:vm";

let failed = 0;
const ok = (cond, what) => { console.log(`${what.padEnd(72)} ${cond ? "ok" : "FAIL"}`); if (!cond) failed++; };
const HERE = new URL(".", import.meta.url).pathname, DIR = join(HERE, "manual"), ROOT = join(HERE, "..");

/* the scripts as the page loads them, in one context without a document */
const ctx = { console };
ctx.window = ctx;
vm.createContext(ctx);
const TOUR_FILES = ["tour.js", "tour.es.js", "tour.en.js", "tour.pt.js", "tour.fr.js", "tour.ja.js", "tour.zh.js"];
for (const f of ["data.js", "nav.js", ...TOUR_FILES, "manual.js"]) vm.runInContext(readFileSync(join(DIR, f), "utf8"), ctx, { filename: f });
const M = ctx.JMAN, N = ctx.JNAV, A = ctx.JMANUAL;
ok(M && N && A, "manual: data.js, nav.js and manual.js load (no DOM)");

/* nav.js is the firmware's: tools/gen_manual.py --check */
try {
  execFileSync("python3", [join(ROOT, "tools/gen_manual.py"), "--check"], { stdio: "pipe" });
  ok(true, "manual: nav.js matches the firmware chains, every named screen in shots/");
} catch (e) { console.log(String(e.stdout || "")); ok(false, "manual: nav.js matches the firmware chains, every named screen in shots/"); }
ok(N.sections.join() === "HOME,EDIT,SEQ,FX,GLO,ENV,LFO,SCL,ARP,SAVE", "manual: the ten sections of the firmware");
const fwv = /FELUCCA_VERSION "(v[^"]+)"/.exec(readFileSync(join(ROOT, "firmware/src/felucca.c"), "utf8"))[1];
ok(N.version === fwv || /^V\d/.test(N.version), `manual: the map is the firmware's version (${N.version}: this tree's ${fwv} or the V line's, JIANT_FW)`);

/* the tutorial (tour.js): every step in the six languages, what it asks to touch is on the drawing, every keycap in
   its words a control, every question lands on a step, every screen in shots/ */
{
  const T = ctx.JTOUR, X = ctx.JTOUR_TEXT, ids = new Set([...A.CONTROL_IDS, ...A.KEYS.map((k) => k.name)]);
  ok(T && T.flat.length > 60 && T.CHAPTERS.length >= 12, `tutorial: ${T.flat.length} steps in ${T.CHAPTERS.length} chapters`);
  for (const l of T.LANGS) {
    const t = X[l] || {};
    const miss = [...T.flat.filter((st) => !(t.steps && t.steps[st.key] && t.steps[st.key].t && t.steps[st.key].d)).map((st) => st.key),
                  ...T.CHAPTERS.filter((c) => !(t.chapters && t.chapters[c.id] && t.chapters[c.id].t)).map((c) => c.id),
                  ...T.QUESTIONS.filter((q) => !(t.questions && t.questions[q.id] && t.questions[q.id].q)).map((q) => q.id),
                  ...Object.keys(X.en.ui).filter((k) => !(t.ui && t.ui[k]))];
    ok(!miss.length, `tutorial: every text in ${l}` + (miss.length ? " (missing " + miss.slice(0, 6).join(" ") + ")" : ""));
    if (l !== "es" && l !== "en") {
      const pm = Object.keys(A.UI.en).filter((k) => !(t.page && t.page[k]));
      ok(!pm.length, `tutorial: the page's own words in ${l}` + (pm.length ? " (missing " + pm.slice(0, 6).join(" ") + ")" : ""));
    }
    const badcap = [];
    for (const [k, v] of Object.entries(t.steps || {})) for (const m of String(v.d).matchAll(/\[([^\]]+)\]/g)) if (!ids.has(m[1].replace("−", "-"))) badcap.push(k + ":" + m[1]);
    ok(!badcap.length, `tutorial: every [keycap] in ${l} is a control` + (badcap.length ? " (" + badcap.slice(0, 4).join(" ") + ")" : ""));
  }
  const badt = T.flat.filter((st) => T.targets(st).some((x) => !ids.has(x))).map((st) => st.key);
  ok(!badt.length, "tutorial: what each step asks to touch is on the drawing" + (badt.length ? " (" + badt.join(" ") + ")" : ""));
  ok(T.QUESTIONS.every((q) => T.indexOf(q.go) >= 0), "tutorial: every question lands on a step");
  const noshot = T.flat.filter((st) => st.shot && !existsSync(join(DIR, "shots", st.shot + ".png"))).map((st) => st.shot);
  ok(!noshot.length, "tutorial: every screen it shows is in shots/" + (noshot.length ? " (" + noshot.join(" ") + ")" : ""));
  ok(A.LANGS.join() === "es,en,pt,fr,ja,zh" && A.uiOf("ja").lang === "ja" && A.uiOf("fr").h2 !== undefined || A.uiOf("fr").r_h2 !== A.UI.en.r_h2,
     "tutorial: the page in six languages (pt fr ja zh from tour.<lang>.js)");
}

/* every control on the drawing has an entry */
ok(A.CONTROL_IDS.length === 22 && A.KEYS.length === 27, "manual: 8 knobs + 14 buttons and 27 keys drawn");
ok(A.KEYS.filter((k) => k.black).length === 11 && A.KEYS[0].name === "F3" && A.KEYS[26].name === "G5", "manual: keys F3..G5, 11 black");
const missing = A.CONTROL_IDS.filter((id) => { const c = M.controls[id]; return !c || !c.alone || !c.alone[0] || !c.alone[1]; });
ok(!missing.length, "manual: every knob and button has its text, in both languages" + (missing.length ? " (" + missing + ")" : ""));
ok(Object.keys(M.controls).every((id) => A.CONTROL_IDS.includes(id)), "manual: no entry for a control the drawing lacks");
ok(A.KEYS.every((k) => A.keyLayers(k.name, "es").length > 0) && M.keyAlone[0] && M.keyAlone[1], "manual: every key has its text and at least one layer");
const firmwareButtons = /B_NAME\[NB\] = \{([^}]*)\}/.exec(readFileSync(join(ROOT, "firmware/src/panel.c"), "utf8"))[1].match(/"([^"]+)"/g).map((s) => s.slice(1, -1));
ok(firmwareButtons.every((b) => A.CONTROL_IDS.includes(b)), "manual: the buttons are panel.c's (FX SCL ENV LFO EDIT GLO HOME SAVE ARP SEQ PLAY REC OCT±)");
const keyNames = new Set(A.KEYS.map((k) => k.name));
const tokOk = (t) => A.CONTROL_IDS.includes(t) || keyNames.has(t) || t === "STEP";
const combos = [...M.commands.map((c) => c.k), ...Object.values(M.controls).flatMap((c) => (c.combos || []).map((x) => x.k))];
const bad = combos.filter((k) => !A.tokens(k).every(tokOk));
ok(!bad.length, `manual: every combo (${combos.length}) names real controls` + (bad.length ? " (" + bad + ")" : ""));
for (const [id, ly] of Object.entries(M.layers))
  if (ly.keys) ok(Object.keys(ly.keys).every((k) => keyNames.has(k)), `manual: layer ${id}: its keys exist`);

/* both languages everywhere */
const pairs = [];
const walk = (v) => { if (Array.isArray(v) && v.length === 2 && v.every((x) => typeof x === "string")) pairs.push(v); else if (v && typeof v === "object") Object.values(v).forEach(walk); };
walk({ c: M.commands, r: M.recipes, n: M.news, s: M.sections, k: M.controls });
ok(pairs.length > 300 && pairs.every(([es, en]) => es.trim() && en.trim()), `manual: ${pairs.length} texts, each in Spanish and English`);
ok((M.news[0].v === fwv.slice(1) || /^V\d/.test(M.news[0].v)) && M.news[M.news.length - 1].v === "0.8.0", "manual: what's new from the current version back to 0.8.0");
const ids = M.sections.map((s) => s.id);
ok([...M.commands, ...M.recipes].every((e) => ids.includes(e.s)), "manual: every command and recipe in a known section");
for (const s of ["home", "edit", "seq", "glo", "fx", "scl", "mod", "save", "menu", "drum", "gen", "euclid", "punch", "song"])
  if (!M.commands.some((c) => c.s === s)) ok(false, `manual: section ${s} has commands`);
ok(["live", "unq", "locks", "clear", "auto", "song", "gen", "euclid", "punch", "liveP", "learn", "usb", "backup", "cpu", "trouble"]
   .every((id) => M.recipes.some((r) => r.id === id)), "manual: the task recipes asked for are all there");

/* the screens: every one named is a 240 x 240 PNG in shots/, none left over */
const shots = A.allShots();
const pngOk = (n) => { const f = join(DIR, "shots", n + ".png"); if (!existsSync(f)) return false; const b = readFileSync(f);
  return b.readUInt32BE(0) === 0x89504e47 && b.readUInt32BE(16) === 240 && b.readUInt32BE(20) === 240; };
const noShot = shots.filter((n) => !pngOk(n));
ok(!noShot.length, `manual: ${shots.length} screens linked, each a 240 x 240 PNG in shots/` + (noShot.length ? " (" + noShot + ")" : ""));
const extra = readdirSync(join(DIR, "shots")).filter((f) => !shots.includes(f.replace(/\.png$/, "")));
ok(!extra.length, "manual: no screen in shots/ the page does not use" + (extra.length ? " (" + extra + ")" : ""));
ok(N.chains.flatMap((c) => c.pages).filter((p) => p.shot).length >= 80, "manual: the map's pages open their screen (80 or more)");

/* the page: its scripts and anchors */
const html = readFileSync(join(DIR, "index.html"), "utf8");
for (const id of ["fm1", "info", "q", "secs", "cmds", "recipes", "map", "news", "emubox", "ctlsel", "layers"])
  ok(html.includes(`id="${id}"`), `manual: index.html has #${id}`);
ok(["data.js", "nav.js", "manual.js"].every((f) => html.includes(`src="${f}"`)), "manual: index.html loads its three scripts");
ok(!/<script[^>]+src="https?:/.test(html) && (html.match(/https:\/\/fonts\.googleapis\.com/g) || []).length <= 2,
   "manual: no external script, only the landing's Google Fonts");
ok(!/JIANT Stems|Probe|probe/.test(readFileSync(join(DIR, "data.js"), "utf8") + html), "manual: no word of the Stems app beyond 'separate product'");
ok(readFileSync(join(HERE, "landing/index.html"), "utf8").includes('href="manual/"'), "manual: the landing page links it");
ok(readFileSync(join(ROOT, "README.md"), "utf8").includes("juanjiant-bit.github.io/JIANT/manual/"), "manual: the README links it");
ok(/make_site\.py manual build\/pages/.test(readFileSync(join(ROOT, ".github/workflows/emulator.yml"), "utf8")), "manual: the workflow publishes it");

/* the search */
const first = (q, l, s) => { const r = A.search(q, l, s); return r[0] && r[0].k; };
ok(first("borrar paso", "es") === "SEQ+G5", "search: 'borrar paso' -> SEQ + G5 first");
ok(first("clear step", "en") === "SEQ+G5", "search: 'clear step' -> SEQ + G5 first");
ok(first("deshacer", "es") === "GLO+G4" && first("undo", "en") === "GLO+G4", "search: deshacer / undo -> GLO + G4");
ok(A.search("CUANTIZAR", "es").some((c) => c.k === "KNOB 4" && /REC Q/.test(c.t[0])), "search: case and accents ignored (CUANTIZAR -> REC Q)");
ok(A.search("micro", "es").some((c) => /MICRO/.test(c.t[0])), "search: MICRO found");
ok(A.search("midi learn", "en").length >= 1 && A.search("midi learn", "en", "menu").every((c) => c.s === "menu"), "search: MIDI LEARN, filtered to MENU");
ok(A.search("", "es", "seq").length === M.commands.filter((c) => c.s === "seq").length, "search: a section alone lists all its commands");
ok(A.search("zzzqqq", "es").length === 0, "search: nonsense finds nothing");
ok(A.search("grabar", "en").length > 0, "search: a Spanish word from the English page still finds");
ok(A.search("stems", "es", "all", M.recipes).some((r) => r.id === "usb"), "search: the recipes too (stems -> USB modes)");
ok(A.emuHint("SEQ+G5", "es") === "Emulador: mantené «0» + «O»" && A.emuHint("GLO+G4", "en") === "Emulator: hold «6» + «.»",
   "manual: the emulator's keys for a combo (SEQ = 0, G5 = O, GLO = 6, G4 = .)");

/* the page in Chromium, when Playwright is there */
let chromium = null;
try { chromium = createRequire(import.meta.url)("playwright").chromium; } catch (e) {
  try { chromium = createRequire("/opt/node-tools/node_modules/")("playwright").chromium; } catch (e2) { /* none */ }
}
if (!chromium) console.log("manual: page in Chromium (no Playwright)".padEnd(72) + " skip");
else {
  const exe = process.env.CHROMIUM || "/opt/pw-browsers/chromium-1194/chrome-linux/chrome";
  const b = await chromium.launch(existsSync(exe) ? { executablePath: exe } : {});
  for (const [w, h] of [[1280, 860], [390, 844]]) {
    const p = await b.newPage({ viewport: { width: w, height: h } });
    const errs = [];
    p.on("pageerror", (e) => errs.push(e.message));
    p.on("requestfailed", (r) => { if (!/fonts\.g/.test(r.url())) errs.push("failed " + r.url()); });
    await p.route(/fonts\.(googleapis|gstatic)\.com/, (r) => r.abort());      /* offline: the page must not need them */
    /* the tutorial, on by default: its card, a step done by touching what it lights, a question, the languages */
    await p.goto("file://" + join(DIR, "index.html") + "?lang=es");
    ok(await p.locator("#tour").isVisible() && /Bienvenida/i.test(await p.locator("#info").innerText()), `page ${w}px: the tutorial on by default`);
    await p.locator("[data-tgo='1']").click();
    await p.locator('#fm1 .ctl[data-ctl="SELECT"]').click();
    ok(await p.locator(".tok").isVisible(), `page ${w}px: tutorial: SELECT touched -> the step done`);
    await p.locator("details.tqs summary").click();
    await p.locator('[data-tq="gen.undo"]').click();
    await p.locator('#fm1 .ctl[data-ctl="GLO"]').click();
    await p.locator('#fm1 .ctl[data-ctl="G4"]').click();
    ok(await p.locator(".tok").isVisible(), `page ${w}px: tutorial: a question, then GLO + G4 -> done`);
    const titles = [];
    for (const l of ["pt", "fr", "ja", "zh", "en"]) { await p.locator(`.lang button[data-lang="${l}"]`).click(); titles.push(await p.locator(".ttitle").innerText()); }
    ok(new Set(titles).size === 5 && titles.every(Boolean), `page ${w}px: tutorial in PT FR JA ZH EN (${titles.join(" | ")})`);
    await p.locator('[data-tmode="off"]').click();
    ok(!(await p.locator(".ttitle").count()), `page ${w}px: Free explore leaves the tutorial`);
    await p.goto("file://" + join(DIR, "index.html") + "?lang=es&tour=off");
    const n = await p.locator("#fm1 .ctl").count();
    ok(n === 49, `page ${w}px: the drawing has its 49 controls (${n})`);
    await p.locator('#fm1 .ctl[data-ctl="SEQ"]').click();
    ok(/SEQ TOOLS/.test(await p.locator("#info").innerText()), `page ${w}px: SEQ touched -> its entry in the panel`);
    await p.locator('#layers button[data-layer="FX"]').click();
    ok((await p.locator('#fm1 .ctl[data-ctl="F3"] .klay').textContent()) === "RPT 1/8", `page ${w}px: the FX layer on the keys (F3 RPT 1/8)`);
    await p.fill("#q", "borrar paso");
    ok(/CLR STEP/.test(await p.locator("#cmds .cmd").first().innerText()), `page ${w}px: the search filters (borrar paso -> CLR STEP)`);
    await p.locator('.pg[data-ce="CE_SEQ"]', { hasText: /\dTRACK$/ }).first().click();
    ok(/SEQ · \w+ · TRACK/.test(await p.locator("#pagebox").innerText()), `page ${w}px: a page of the map opens its screen and knobs`);
    const imgsOk = await p.evaluate(() => Promise.all([...document.images].filter((i) => i.src.includes("/shots/")).map((i) =>
      i.complete ? i.naturalWidth === 240 : new Promise((r) => { i.onload = () => r(i.naturalWidth === 240); i.onerror = () => r(false); }))).then((a) => a.every(Boolean)));
    ok(imgsOk, `page ${w}px: the screens shown load`);
    await p.locator('.lang button[data-lang="en"]').click();
    ok((await p.locator("#ref h2").innerText()) === "How do I…?", `page ${w}px: English`);
    ok(await p.evaluate(() => document.documentElement.scrollWidth <= innerWidth), `page ${w}px: no horizontal scroll`);
    ok(!errs.length, `page ${w}px: no error` + (errs.length ? " (" + errs.join("; ") + ")" : ""));
    await p.close();
  }
  await b.close();
}

console.log(failed ? `\n${failed} FAILED` : "\nall ok");
process.exit(failed ? 1 : 0);

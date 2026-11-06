// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
// The interactive manual's TUTORIAL (on by default): a guided walk through the whole of JIANT on the drawn FM-1.
// Each step names what to touch ("do": a control, or a combination "A+B" = hold A and press / turn B, as data.js's
// combos) and the screen it leads to (shot: a render in shots/). The page lights the controls to touch; touching them
// in order is the step done. The words live in tour.<lang>.js (window.JTOUR_TEXT[lang]: es en pt fr ja zh), keyed by
// the ids below: chapters {t, d}, steps {t, d}, questions {q}, and the page's own words (ui). In a text, [X] is drawn
// as the control X's keycap. The logic part (JTOUR) runs without a DOM, for web/test_manual.mjs.
(function (G) {
"use strict";

const CHAPTERS = [
  { id: "welcome", steps: [
    { id: "hello", shot: 'home' },
    { id: "how", do: "SELECT", shot: 'home' },
    { id: "emu", shot: 'home' } ] },
  { id: "panel", steps: [
    { id: "master", do: "MASTER", shot: 'home' },
    { id: "algorithm", do: "ALGORITHM", shot: 'home' },
    { id: "presets", do: "PRESETS", shot: 'save_presets' },
    { id: "select", do: "SELECT", shot: 'lente_bar' },
    { id: "knobs", do: "KNOB 1", shot: 'home_being' },
    { id: "buttons", do: "SEQ", shot: 'lente_pattern' },
    { id: "keys", do: "C4", shot: 'home' },
    { id: "oct", do: "OCT+", shot: 'home' } ] },
  { id: "sound", steps: [
    { id: "home", do: "HOME", shot: 'home' },
    { id: "track", do: "ALGORITHM", shot: 'home' },
    { id: "browse", do: "PRESETS", shot: 'save_presets' },
    { id: "play", do: "E4", shot: 'home' },
    { id: "shape", do: "KNOB 2", shot: 'home_being' },
    { id: "dice", do: "EDIT+SELECT", shot: 'dlg_dice_sound' },
    { id: "keep", do: "OCT+", shot: 'home_being' },
    { id: "jump", do: "EDIT+PRESETS", shot: 'save_presets' } ] },
  { id: "nav", steps: [
    { id: "rule1", do: "SELECT", shot: 'fx_track_sends' },
    { id: "rule2", do: "FX", shot: 'fx_buses' },
    { id: "rule3", do: "PRESETS", shot: 'phrases' },
    { id: "rule4", do: "KNOB 1", shot: 'lente_bar' },
    { id: "rule5", do: "GLO", layer: "GLO", shot: 'layer_glo_active' },
    { id: "rule6", shot: 'fx_effect_delay2' },
    { id: "menu", do: "HOME", shot: 'menu' } ] },
  { id: "gen", steps: [
    { id: "open", do: "HOME+SELECT", shot: 'gen_explore' },
    { id: "make", do: "OCT+", shot: 'gen' },
    { id: "play", do: "PLAY", shot: 'home' },
    { id: "again", do: "OCT+", shot: 'gen' },
    { id: "parts", do: "SELECT", shot: 'gen_parts' },
    { id: "undo", do: "GLO+G4", layer: "GLO", shot: 'layer_glo_active' } ] },
  { id: "edit", steps: [
    { id: "engines", do: "EDIT", shot: 'edit_analog' },
    { id: "pages", do: "SELECT", shot: 'ed_filter' },
    { id: "focus", do: "EDIT", shot: 'ed_focus' },
    { id: "fm6", shot: 'edit_fm6' },
    { id: "drum", shot: 'drumx' },
    { id: "init", do: "EDIT+F3", layer: "EDIT", shot: 'layer_edit_active' },
    { id: "store", do: "SAVE", shot: 'save_sound' } ] },
  { id: "seq", steps: [
    { id: "roll", do: "SEQ", shot: 'lente_pattern' },
    { id: "steprec", do: "REC", shot: 'lente_step_note' },
    { id: "write", do: "C4", shot: 'roll_chords' },
    { id: "length", do: "KNOB 3", shot: 'roll_ties' },
    { id: "clear", do: "SEQ+G5", layer: "SEQ", shot: 'layer_seq_tools' },
    { id: "live", do: "REC", shot: 'countin' },
    { id: "lock", do: "STEP+KNOB 2", shot: 'roll_lock_held' },
    { id: "grid", shot: 'drum' },
    { id: "patterns", do: "PRESETS", shot: 'phrases' },
    { id: "tools", do: "SEQ", layer: "SEQ", shot: 'layer_seq_tools' } ] },
  { id: "euclid", steps: [
    { id: "go", do: "SEQ", shot: 'euclid' },
    { id: "density", do: "KNOB 1", shot: 'euclid' },
    { id: "lane", do: "KNOB 4", shot: 'euclid' },
    { id: "modes", do: "SELECT", shot: 'euclid_modes' },
    { id: "vary", do: "OCT+", shot: 'euclid' },
    { id: "mutate", do: "SELECT", shot: 'euclid_modes' },
    { id: "melodic", do: "SCL", shot: 'scl' } ] },
  { id: "mix", steps: [
    { id: "glo", do: "GLO", shot: 'mixer' },
    { id: "mutes", do: "GLO+F#3", layer: "GLO", shot: 'layer_glo_active' },
    { id: "solo", do: "GLO+F3", layer: "GLO", shot: 'layer_glo_active' },
    { id: "tempo", do: "GLO+SELECT", shot: 'global' },
    { id: "fx", do: "FX", shot: 'fx_buses' },
    { id: "punch", do: "FX+F3", layer: "FX", shot: 'perform_held' },
    { id: "mano", do: "GLO", shot: 'live_mano' } ] },
  { id: "mod", steps: [
    { id: "env", do: "ENV", shot: 'env' },
    { id: "lfo", do: "LFO", shot: 'lfo' },
    { id: "matrix", do: "LFO", shot: 'mod' },
    { id: "macros", do: "LFO+KNOB 1", layer: "LFO", shot: 'layer_macro' },
    { id: "scale", do: "SCL", shot: 'scl' },
    { id: "arp", do: "ARP", shot: 'key_arp' } ] },
  { id: "song", steps: [
    { id: "idea", shot: 'save_timeline' },
    { id: "sections", do: "SAVE", shot: 'save_project' },
    { id: "storea", do: "OCT+", shot: 'name_typing' },
    { id: "storeb", do: "KNOB 1", shot: 'save_project' },
    { id: "arrange", do: "SELECT", shot: 'save_arrange' },
    { id: "timeline", do: "SAVE", shot: 'save_timeline' },
    { id: "live", do: "SAVE+F3", layer: "SAVE", shot: 'layer_song' },
    { id: "projects", shot: 'save_library' } ] },
  { id: "connect", steps: [
    { id: "menu", do: "HOME", shot: 'menu' },
    { id: "tracks", do: "GLO", shot: 'system' },
    { id: "usb", shot: 'menu_row' },
    { id: "input", shot: 'home' },
    { id: "bt", shot: 'menu' },
    { id: "editor", shot: 'about' } ] },
  { id: "next", steps: [
    { id: "cheat", shot: 'home' },
    { id: "search", shot: 'home' },
    { id: "done", shot: 'home' } ] }
];

// the questions: where each one lands (chapter.step)
const QUESTIONS = [
  { id: "q_sound", go: "sound.browse" }, { id: "q_lost", go: "nav.rule1" }, { id: "q_undo", go: "gen.undo" },
  { id: "q_song", go: "gen.open" }, { id: "q_melody", go: "seq.steprec" }, { id: "q_beat", go: "euclid.go" },
  { id: "q_save", go: "song.sections" }, { id: "q_chain", go: "song.arrange" }, { id: "q_mute", go: "mix.mutes" },
  { id: "q_live", go: "mix.punch" }, { id: "q_tempo", go: "mix.tempo" }, { id: "q_daw", go: "connect.usb" },
  { id: "q_tracks", go: "connect.tracks" }, { id: "q_engine", go: "edit.engines" }
];

const LANGS = ["es", "en", "pt", "fr", "ja", "zh"];
const TXT = () => G.JTOUR_TEXT || {};
/* a text of the tour in lang (English, then Spanish, when a language lacks it) */
function tx(lang, kind, id, field) {
  for (const l of [lang, "en", "es"]) {
    const t = TXT()[l], v = t && t[kind] && t[kind][id];
    if (v != null) return field ? (v[field] != null ? v[field] : "") : v;
  }
  return "";
}
const flat = [];
CHAPTERS.forEach((c, ci) => c.steps.forEach((s, si) => flat.push({ ...s, ch: c.id, ci, si, key: c.id + "." + s.id })));
const indexOf = (key) => flat.findIndex((s) => s.key === key);
/* the controls a step names, in order: "SEQ+G5" -> SEQ, G5; "STEP+KNOB 2" -> KNOB 2 (a step held is a key of the grid) */
function targets(step) {
  if (!step || !step.do) return [];
  return (String(step.do).match(/OCT[+-]|[^+]+/g) || []).map((x) => x.trim()).filter((x) => x && x !== "STEP");
}

G.JTOUR = { CHAPTERS, QUESTIONS, LANGS, flat, indexOf, targets, tx };
})(typeof window !== "undefined" ? window : globalThis);

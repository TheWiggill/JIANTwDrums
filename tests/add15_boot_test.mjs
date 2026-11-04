import fs from 'node:fs';
import assert from 'node:assert/strict';
const bytes=fs.readFileSync(process.argv[2]);
async function boot(saved){
 const {instance}=await WebAssembly.instantiate(bytes,{}),e=instance.exports;
 e._initialize();e.web_nor_erase();
 if(saved)new Uint8Array(e.memory.buffer,e.web_nor(),e.web_nor_size()).set(saved);
 e.web_seed(12345);e.web_boot();return e;
}
function render(e,ms){
 let peak=0;
 for(let n=Math.round(ms*44.1);n>0;n-=128){const frames=Math.min(128,n);e.web_render(frames);
  for(const x of new Float32Array(e.memory.buffer,e.web_out_l(),frames)){assert.ok(Number.isFinite(x));peak=Math.max(peak,Math.abs(x));}}
 return peak;
}
const e=await boot();render(e,5000);assert.ok(e.web_screen_draws()>0,'fresh boot did not draw');
e.web_gen(0,12345,64,0);render(e,100);e.web_play(1);assert.ok(render(e,3000)>0.00001,'generated scene silent');
e.web_play(0);render(e,100);
// Save an actual section explicitly; an idle wait alone does not guarantee autosave,
// which intentionally waits until all voices have finished their releases.
const length=e.web_bk_len(0);assert.ok(length>0,'generated scene does not fit a section');
assert.equal(e.web_bk_put(40,length),0,'section flash save failed');
assert.ok(e.web_flash_writes_count()>0,'section was not written');
const saved=new Uint8Array(e.memory.buffer,e.web_nor(),e.web_nor_size()).slice();
const resumed=await boot(saved);render(resumed,5000);assert.ok(resumed.web_screen_draws()>0,'saved-session boot did not draw');
const stored=resumed.web_bk_len(40);assert.equal(stored,length,'saved section missing after boot');
assert.equal(resumed.web_bk_put(0,stored),0,'saved section cannot be restored');
render(resumed,100);resumed.web_play(1);assert.ok(render(resumed,3000)>0.00001,'saved scene silent');
console.log('PASS ADD15 WASM: fresh boot, generated audio, explicit section flash save, reboot and stored-project playback');

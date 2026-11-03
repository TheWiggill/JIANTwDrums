"""Real Chromium: ADD selection, keyboard audio, edition guard and storage isolation."""
from pathlib import Path
from functools import partial
from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
from threading import Thread
import shutil,sys
from playwright.sync_api import sync_playwright

root=Path(sys.argv[1]).resolve()
server=ThreadingHTTPServer(('127.0.0.1',0),partial(SimpleHTTPRequestHandler,directory=str(root)))
Thread(target=server.serve_forever,daemon=True).start()
try:
    with sync_playwright() as p:
        browser=p.chromium.launch(executable_path=shutil.which('google-chrome') or shutil.which('chromium'),args=['--no-sandbox','--autoplay-policy=no-user-gesture-required'])
        page=browser.new_page(viewport={'width':412,'height':900});errors=[];requests=[]
        page.on('pageerror',lambda e:errors.append(str(e)));page.on('request',lambda r:requests.append(r.url))
        page.add_init_script("""localStorage.setItem('felucca-emu-flash','legacy-sample-bank-preserved');
        const Original=window.AudioWorkletNode;
        window.AudioWorkletNode=class extends Original {constructor(...a){super(...a);window.__node=this;window.__audioErrors=[];
          this.addEventListener('processorerror',()=>__audioErrors.push('processorerror'));
          this.port.addEventListener('message',e=>{if(e.data.type==='error')__audioErrors.push(e.data.message);if(e.data.type==='diagnostics')window.__edition=e.data;});this.port.start();}};""")
        page.goto(f'http://127.0.0.1:{server.server_port}/')
        assert 'V10' in page.locator('.version').inner_text()
        assert page.locator('#sample-bank-controls').count()==0
        page.click('#go');page.wait_for_function("document.querySelector('#go').textContent==='On'",timeout=30000)
        page.wait_for_timeout(4200)
        # Tap SAVE (V10.2: TIMELINE), then SELECT to the chain's last page, PRESETS; KNOB 2 selects the engine.
        page.locator('[role="button"][aria-label="SAVE"]').click(delay=100)
        page.wait_for_timeout(150)
        page.locator('[role="slider"][aria-label="SELECT"]').focus()
        for _ in range(6):
            page.keyboard.press('ArrowRight');page.wait_for_timeout(150)
        page.locator('[role="slider"][aria-label="KNOB 2"]').focus()
        page.keyboard.press('ArrowLeft');page.wait_for_timeout(150)   # (V10.54: INPUT is the last engine, ADD before it)
        page.keyboard.press('ArrowLeft');page.wait_for_timeout(150)
        page.locator('[role="slider"][aria-label="KNOB 2"]').evaluate('(el)=>el.blur()')
        page.evaluate("__node.port.postMessage({type:'diagnostics'})")
        page.wait_for_function('window.__edition && __edition.additive===true && __edition.engine===13',timeout=5000)
        page.evaluate("""() => {const a=__node.context.createAnalyser();a.fftSize=512;__node.connect(a);window.__analyser=a;}""")
        page.keyboard.down('b');peak=0
        for _ in range(12):
            page.wait_for_timeout(20)
            peak=max(peak,page.evaluate("() => {const x=new Float32Array(512);__analyser.getFloatTimeDomainData(x);return Math.max(...x.map(Math.abs));}"))
        page.keyboard.up('b');assert peak>0.00001,peak
        assert page.evaluate("localStorage.getItem('felucca-emu-flash')")=='legacy-sample-bank-preserved'
        assert not any('factory-' in x for x in requests),requests
        assert not errors,errors
        assert not page.evaluate('__audioErrors'),page.evaluate('__audioErrors')
        print('PASS Chromium ADD15: actual ADD engine 13, physical keyboard audio peak',peak,'legacy SAMPLE storage preserved; no sample-bank requests',flush=True)
        browser.close()
finally:
    server.shutdown()

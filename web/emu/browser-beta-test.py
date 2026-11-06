"""Real Chromium, the beta emulator: it loads, turns on, the keyboard sounds, no page or audio errors, the version label."""
from pathlib import Path
from functools import partial
from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
from threading import Thread
import shutil,sys
from playwright.sync_api import sync_playwright

root=Path(sys.argv[1]).resolve();want=sys.argv[2] if len(sys.argv)>2 else 'V10'
server=ThreadingHTTPServer(('127.0.0.1',0),partial(SimpleHTTPRequestHandler,directory=str(root)))
Thread(target=server.serve_forever,daemon=True).start()
try:
    with sync_playwright() as p:
        browser=p.chromium.launch(executable_path=shutil.which('google-chrome') or shutil.which('chromium'),args=['--no-sandbox','--autoplay-policy=no-user-gesture-required'])
        page=browser.new_page(viewport={'width':412,'height':900});errors=[]
        page.on('pageerror',lambda e:errors.append(str(e)))
        page.add_init_script("""const Original=window.AudioWorkletNode;
        window.AudioWorkletNode=class extends Original {constructor(...a){super(...a);window.__node=this;window.__audioErrors=[];
          this.addEventListener('processorerror',()=>__audioErrors.push('processorerror'));
          this.port.addEventListener('message',e=>{if(e.data.type==='error')__audioErrors.push(e.data.message);});this.port.start();}};""")
        page.goto(f'http://127.0.0.1:{server.server_port}/')
        label=page.locator('.version').inner_text()
        assert want in label,(label,want)
        page.click('#go');page.wait_for_function("document.querySelector('#go').textContent==='On'",timeout=30000)
        page.wait_for_timeout(4200)
        page.evaluate("""() => {const a=__node.context.createAnalyser();a.fftSize=512;__node.connect(a);window.__analyser=a;}""")
        page.keyboard.down('b');peak=0
        for _ in range(15):
            page.wait_for_timeout(20)
            peak=max(peak,page.evaluate("() => {const x=new Float32Array(512);__analyser.getFloatTimeDomainData(x);return Math.max(...x.map(Math.abs));}"))
        page.keyboard.up('b');assert peak>0.00001,peak
        assert not errors,errors
        assert not page.evaluate('__audioErrors'),page.evaluate('__audioErrors')
        print('PASS Chromium beta emulator',label,'keyboard audio peak',peak,flush=True)
        browser.close()
finally:
    server.shutdown()

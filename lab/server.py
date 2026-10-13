"""FM1 Mod Lab. Local source editing only; no device or arbitrary command endpoints."""
import ast
import argparse
import base64
import difflib
import hashlib
import importlib.util
import json
import os
import re
from pathlib import Path
import secrets
import shutil
import subprocess
import sys
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import parse_qs, urlparse
import zipfile
from build_emulator import source_hash
from lab_paths import UPSTREAM_REF, configured_path, prepare_baseline

HERE = Path(__file__).resolve().parent
BASE = HERE.parent
WORK = configured_path('FM1_SOURCE_ROOT', BASE)
DATA = configured_path('FM1_LAB_DATA', WORK / '.acultur8-lab')
ORIGINAL = None
BASELINE = {}
REX_SDK = configured_path('FM1_REX_SDK')
REX_HELPER = configured_path('FM1_REX_HELPER')
STATIC = HERE / 'static'
TOKEN = secrets.token_urlsafe(32)
LOCK = threading.RLock()
BUILD_LOCK = threading.Lock()
BUILD = {'running': False, 'error': None}
EDITABLE = {'.c', '.h', '.S', '.ld', '.py', '.sh', '.js', '.mjs', '.html', '.css', '.json', '.md', '.txt', '.x0x', '.svg', '.yml'}
MAX_FILE = 1024 * 1024
MODULES = [
    ('Interface', 'LCD pages, labels, palettes and controls', 'firmware/src/app/ui.c', 'violet'),
    ('909 drums', 'Modelled voices, sampled cymbals and kit parameters', 'firmware/src/dsp/drum909.c', 'amber'),
    ('808 drums', 'Analogue drum synthesis and voice switches', 'firmware/src/dsp/drum808.c', 'amber'),
    ('303 acid', 'Bass synthesis, filter, envelopes and slide', 'firmware/src/dsp/bass303.c', 'lime'),
    ('Break player', 'Loop slices, playback and generators', 'firmware/src/dsp/breaks.c', 'cyan'),
    ('Sequencer', 'Clock, swing, probability and step playback', 'firmware/src/seq/sequencer.c', 'violet'),
    ('FX + master', 'Reverb, tape delay, compressor and output', 'firmware/src/dsp/master.c', 'cyan'),
    ('Project storage', 'Defaults, migrations, save and readback', 'firmware/src/app/project.c', 'lime'),
]
TARGETS = [
    ('FM1-01', 'Establish the baseline', 'Compare the archive, working copy and source map; retain a known-good source checkpoint.', 'review', 'foundation'),
    ('FM1-02', 'Bring up the host simulator', 'Set up Linux/WSL, Python + Pillow and a host C compiler; build and run getting_started.x0x.', 'todo', 'toolchain'),
    ('FM1-03', 'Make the first UI mod', 'Change one label or palette in ui.c; verify screenshots and navigation in the host simulator.', 'todo', 'interface'),
    ('FM1-04', 'Explore sound defaults', 'Adjust one engine default; compare rendered audio against baseline and inspect clipping.', 'todo', 'sound'),
    ('FM1-05', 'Validate patterns and saves', 'Run sequencer, undo, storage and migration scenarios before altering any persisted layout.', 'todo', 'quality'),
    ('FM1-06', 'Establish firmware builds', 'Install the JieLi pi32v2 toolchain and pinned AC79 SDK; pass image/RAM budgets and produce x0x.fwsc.', 'todo', 'toolchain'),
    ('FM1-07', 'Design X0X parameter editing', 'Define a versioned X0X protocol from engine.h. Felucca SET/DUMP is not implemented in this fork.', 'todo', 'protocol'),
    ('FM1-08', 'Prepare sample workflow', 'Validate WAV-to-slot conversion offline; review ownership and slot capacity before device upload.', 'todo', 'samples'),
    ('FM1-09', 'Document device recovery', 'Identify the physical FM1, installed firmware, stock recovery image and recovery method before installation.', 'todo', 'hardware'),
    ('FM1-10', 'Accept a device release', 'After host and target checks pass, separately approve installation and check audio, MIDI, controls and save/reboot.', 'todo', 'hardware'),
    ('FM1-11', 'Per-sound high-pass and low-pass filters', 'Independent HPF/LPF per 808/909 sound, 303 part and break part; bypass preserves baseline audio; smooth cutoff changes, bounded output and worst-case CPU checks pass.', 'todo', 'sound'),
    ('FM1-12', 'Per-sound boost and ceiling limiter', 'Start with one 909 kick, then extend to individual sounds; verify ceiling, gain reduction, release, aligned timing, effect sends and unchanged bypass sound.', 'todo', 'sound'),
    ('FM1-13', 'Paged arrangement blocks and transitions', 'Show pattern step pages separately from arrangement blocks; edit block order, repeat count and quantized transitions; verify internal/external clock, save/load and playback while editing.', 'todo', 'sequencer'),
    ('FM1-14', 'Key-aware chord progression lane', 'Sequence degrees 1-7 with scale-derived triad qualities and chord durations; verify major/minor tables, progression loops and saved-project migration. Pentatonic mode must not invent seven diatonic degrees.', 'todo', 'harmony'),
    ('FM1-15', 'Chord-following bass generation', 'Both 303 generators and mutation obey the active chord in strict mode; preserve rhythmic seed, accent/slide controls and octave limits; verify held notes, transitions and MIDI pitch agreement.', 'todo', 'harmony'),
    ('FM1-16', 'External MIDI pad accompaniment', 'Progression triggers triads on a dedicated configurable MIDI channel; release the actual previous notes on changes, stop and reset; verify ties, repeated chords, transport and channel isolation.', 'todo', 'accompaniment'),
    ('FM1-17', 'Measure a minimal internal pad prototype', 'Optional later experiment: three simple voices, smooth envelopes and voice cap; compare worst-case CPU, RAM, dropouts and guard activity with dense existing parts before enabling a production pad.', 'todo', 'accompaniment'),
]

def digest(data):
    return hashlib.sha256(data).hexdigest()

def atomic(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_name(path.name + '.tmp-' + secrets.token_hex(4))
    tmp.write_bytes(data)
    os.replace(tmp, path)

def record(event, **kwargs):
    with (DATA / 'history.jsonl').open('a', encoding='utf-8') as f:
        f.write(json.dumps({'at': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()), 'event': event, **kwargs}) + '\n')

def safe_file(root, name):
    if not isinstance(name, str) or not name or '\\' in name or ':' in name:
        raise ValueError('Select a valid source file.')
    relative = Path(name)
    if relative.is_absolute() or '..' in relative.parts or any(p.startswith('.') for p in relative.parts):
        raise ValueError('Path is outside the source workspace.')
    path = (root / relative).resolve()
    if not path.is_relative_to(root.resolve()) or not path.is_file():
        raise ValueError('Source file not found.')
    if root.resolve() == WORK.resolve() and not source_visible(path):
        raise ValueError('This path is outside the editable source inventory.')
    return path

def source_visible(path):
    """Do not expose local data, Git metadata, dependencies or configured SDKs."""
    resolved = path.resolve()
    if not resolved.is_relative_to(WORK.resolve()):
        return False
    if any(p.startswith('.') or p in {'build', '__pycache__', 'node_modules'} for p in path.relative_to(WORK).parts):
        return False
    private = [DATA, ORIGINAL, REX_SDK, REX_HELPER]
    return not any(p and (resolved == p.resolve() or resolved.is_relative_to(p.resolve())) for p in private)

def file_record(name):
    path = safe_file(WORK, name)
    if path.suffix not in EDITABLE or path.stat().st_size > MAX_FILE:
        raise ValueError('This asset is read-only. Select a text source file.')
    data = path.read_bytes()
    original = (ORIGINAL / name).resolve()
    if not original.is_relative_to(ORIGINAL.resolve()):
        raise ValueError('Baseline path is outside the source workspace.')
    old = original.read_bytes() if original.is_file() else b''
    text = data.decode('utf-8')
    diff = ''.join(difflib.unified_diff(old.decode('utf-8').splitlines(True), text.splitlines(True), fromfile='baseline/' + name, tofile='working/' + name))
    backups = []
    folder = DATA / 'backups' / digest(name.encode())[:24]
    if folder.exists():
        backups = [{'id': p.name, 'bytes': p.stat().st_size} for p in sorted(folder.glob('*.bak'), reverse=True)]
    return {'path': name, 'content': text, 'sha': digest(data), 'changed': data != old, 'diff': diff, 'backups': backups}

def files():
    result = []
    for path in sorted(WORK.rglob('*')):
        if not path.is_file() or not source_visible(path):
            continue
        name = path.relative_to(WORK).as_posix()
        original = ORIGINAL / name
        result.append({'path': name, 'bytes': path.stat().st_size, 'editable': path.suffix in EDITABLE and path.stat().st_size <= MAX_FILE,
                       'changed': not original.exists() or digest(path.read_bytes()) != digest(original.read_bytes())})
    return result

def readiness():
    tc = Path(os.environ.get('JIELI_TOOLCHAIN', str(Path.home() / '.jieli/toolchain')))
    sdk = Path(os.environ.get('AC79_SDK', str(Path.home() / 'fw-AC79_AIoT_SDK')))
    return [
        {'name': 'Python', 'ready': True, 'detail': sys.version.split()[0]},
        {'name': 'Pillow', 'ready': importlib.util.find_spec('PIL') is not None, 'detail': 'Font and sample generation'},
        {'name': 'Node.js', 'ready': bool(shutil.which('node')), 'detail': 'JavaScript checks and web tests'},
        {'name': 'Host C compiler', 'ready': bool(shutil.which('cc') or shutil.which('gcc') or shutil.which('clang')), 'detail': shutil.which('cc') or shutil.which('gcc') or shutil.which('clang') or 'Not detected on server PATH; WSL tools need a separate check'},
        {'name': 'Linux build environment', 'ready': sys.platform.startswith('linux'), 'detail': 'Windows host detected; use a verified WSL/Linux setup' if os.name == 'nt' else sys.platform},
        {'name': 'JieLi pi32v2', 'ready': (tc / 'pi32v2/bin/clang').is_file(), 'detail': 'JIELI_TOOLCHAIN or ~/.jieli/toolchain'},
        {'name': 'AC79 SDK', 'ready': all((sdk / 'cpu/wl82/tools' / p).is_file() for p in ['uboot.boot', 'cfg_tool.bin', 'cfg/eq_cfg_hw.bin']), 'detail': 'Pinned SDK files; hashes still require upstream build verification'},
        {'name': 'Compiled emulator', 'ready': all((WORK / 'build/emu' / p).is_file() for p in ['index.html', 'worklet.js', 'x0x.wasm']), 'detail': 'Emscripten build outputs build/emu/x0x.wasm with HTML and worklet.js'},
    ]

def state():
    listing = files()
    return {'files': listing, 'modules': [{'name': a, 'description': b, 'path': c, 'color': d} for a,b,c,d in MODULES],
            'targets': json.loads((DATA / 'targets.json').read_text('utf-8')), 'readiness': readiness(),
            'history': [json.loads(line) for line in (DATA / 'history.jsonl').read_text('utf-8').splitlines()][-30:][::-1],
            'checks': json.loads((DATA / 'checks.json').read_text('utf-8')) if (DATA / 'checks.json').exists() else None,
            'archive_sha': BASELINE.get('archive_sha256', ''), 'baseline_commit': BASELINE.get('commit', UPSTREAM_REF), 'source_root': str(WORK),
            'stats': {'files': len(listing), 'changed': sum(f['changed'] for f in listing), 'parts': 5, 'steps': 64}, 'emulator': emulator_state(), 'rex_ready': rex_ready()}

def rex_ready():
    return bool(sys.platform == 'win32' and sys.maxsize > 2**32 and REX_HELPER and REX_HELPER.is_file()
                and REX_SDK and (REX_SDK / 'Win/x64/Deployment/REX Shared Library.dll').is_file())

def rex_job(identifier):
    if not re.fullmatch(r'[0-9a-f]{20}', identifier):
        raise ValueError('Invalid conversion identifier')
    path = DATA / 'rex-imports' / identifier / 'job.json'
    if not path.is_file():
        raise ValueError('Conversion not found')
    return json.loads(path.read_text('utf-8'))

def start_rex_conversion(name, encoded):
    if not isinstance(name,str) or len(name)>120 or '/' in name or '\\' in name or ':' in name or Path(name).suffix.lower() not in {'.rex','.rx2','.rcy'}:
        raise ValueError('Choose a .rex, .rx2 or .rcy file with a valid filename.')
    sdk = REX_SDK
    if not rex_ready():
        raise ValueError('Native REX conversion needs 64-bit Windows Python and separately configured FM1_REX_HELPER and FM1_REX_SDK paths. WAV and slice-marker import is available without them. See the FAQ.')
    if not isinstance(encoded,str):
        raise ValueError('REX upload must contain base64 file data.')
    try:
        raw = base64.b64decode(encoded, validate=True)
    except (ValueError,TypeError) as error:
        raise ValueError('Invalid base64 REX upload.') from error
    if not 0<len(raw)<=16*1024*1024:
        raise ValueError('REX input must be nonempty and at most 16 MiB.')
    name=re.sub(r'[^A-Za-z0-9_.-]','_',name)
    identifier=secrets.token_hex(10);folder=DATA/'rex-imports'/identifier;folder.mkdir(parents=True)
    source=folder/name;source.write_bytes(raw)
    job={'id':identifier,'status':'running','name':name,'error':None}
    atomic(folder/'job.json',json.dumps(job).encode())
    def convert():
        try:
            helper=REX_HELPER
            proc=subprocess.run([sys.executable,str(helper),str(source),'--output',str(folder/'bundle'),'--sdk',str(sdk)],capture_output=True,text=True,timeout=120)
            if proc.returncode:
                job.update(status='failed',error=(proc.stderr or proc.stdout)[-3000:])
            else:
                marker_name=source.stem+'_markers.json';manifest=json.loads((folder/'bundle'/marker_name).read_text('utf-8'))
                if not isinstance(manifest.get('audioFile'), str) or Path(manifest['audioFile']).suffix.lower() != '.wav':
                    raise ValueError('The external converter did not return a WAV slice bank.')
                for output_name in [manifest['audioFile'], marker_name, source.stem+'_markers.csv', 'REX-SDK-LICENSE.txt', 'ATTRIBUTION.txt']:
                    safe_file(folder/'bundle', output_name)
                root='/rex/'+identifier+'/'
                job.update(status='done',sliceCount=len(manifest['slices']),bpm=manifest['bpm'],loopDuration=manifest['loopDuration'],audioUrl=root+manifest['audioFile'],markersUrl=root+marker_name,csvUrl=root+source.stem+'_markers.csv',licenseUrl=root+'REX-SDK-LICENSE.txt',attributionUrl=root+'ATTRIBUTION.txt')
            with LOCK:
                record('REX conversion',detail=name,status=job['status'])
        except (OSError,ValueError,KeyError,TypeError,subprocess.TimeoutExpired) as e:
            job.update(status='failed',error=str(e))
        finally:
            atomic(folder/'job.json',json.dumps(job,indent=2).encode())
    threading.Thread(target=convert,daemon=True).start()
    return {'id':identifier,'status':'running'}

def emulator_state():
    output = WORK / 'build/emu'
    ready = all((output / p).is_file() for p in ['index.html', 'emu.js', 'emu.css', 'worklet.js', 'x0x.wasm', 'manifest.json'])
    manifest = json.loads((output / 'manifest.json').read_text('utf-8')) if ready else None
    log = (DATA / 'emulator-build.log').read_text('utf-8', errors='replace')[-14000:] if (DATA / 'emulator-build.log').exists() else ''
    return {**BUILD, 'ready': ready, 'stale': bool(ready and (manifest['source_sha256'] != source_hash(WORK) or manifest['wasm_sha256'] != digest((output / 'x0x.wasm').read_bytes()))), 'manifest': manifest, 'log': log}

def start_emulator_build():
    if not BUILD_LOCK.acquire(blocking=False):
        return
    BUILD.update(running=True, error=None)
    def work():
        try:
            with (DATA / 'emulator-build.log').open('w', encoding='utf-8') as log:
                result = subprocess.run([sys.executable, str(HERE / 'build_emulator.py'), '--source-root', str(WORK)], cwd=HERE, stdout=log, stderr=subprocess.STDOUT, timeout=900)
            if result.returncode:
                BUILD['error'] = 'Build failed. See compiler output below.'
            with LOCK:
                record('emulator build', detail='Failed' if result.returncode else 'Compiled working firmware to WebAssembly')
        except (OSError, subprocess.TimeoutExpired) as e:
            BUILD['error'] = str(e)
        finally:
            BUILD['running'] = False
            BUILD_LOCK.release()
    threading.Thread(target=work, daemon=True).start()

def checks():
    results = []
    for path in WORK.rglob('*.py'):
        if not source_visible(path):
            continue
        try:
            ast.parse(path.read_text('utf-8'), filename=str(path))
        except (SyntaxError, UnicodeError) as e:
            results.append({'name': path.relative_to(WORK).as_posix(), 'status': 'fail', 'detail': str(e)})
    count = sum(1 for path in WORK.rglob('*.py') if source_visible(path))
    results.insert(0, {'name': 'Python syntax', 'status': 'fail' if results else 'pass', 'detail': f'{count} Python files parsed; no code executed'})
    results.append({'name': 'Source baseline', 'status': 'pass', 'detail': f'{len(files())} files inventoried; {sum(f["changed"] for f in files())} modified'})
    results.append({'name': 'Felucca web test compatibility', 'status': 'blocked' if not (WORK / 'web/editor.html').exists() else 'review', 'detail': 'web/test_web.mjs references editor.html and firmware/src/params.c, absent from this X0X archive'})
    results.append({'name': 'Firmware / host build', 'status': 'not tested', 'detail': 'Requires a verified Linux toolchain, SDK and simulator. Syntax checks do not prove firmware correctness.'})
    result = {'at': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()), 'results': results}
    atomic(DATA / 'checks.json', json.dumps(result, indent=2).encode())
    record('checks', detail='Offline source checks completed')
    return result

class Handler(BaseHTTPRequestHandler):
    def log_message(self, *args):
        pass

    def send(self, data, content_type='application/json', status=200):
        if isinstance(data, (dict, list)):
            data = json.dumps(data).encode()
        elif isinstance(data, str):
            data = data.encode()
        self.send_response(status)
        self.send_header('Content-Type', content_type)
        self.send_header('Content-Length', str(len(data)))
        self.send_header('Cache-Control', 'no-store')
        self.send_header('X-Content-Type-Options', 'nosniff')
        emulator = urlparse(self.path).path.startswith('/emu/')
        self.send_header('X-Frame-Options', 'SAMEORIGIN' if emulator else 'DENY')
        csp =  "default-src 'self'; script-src 'self' 'wasm-unsafe-eval'; style-src 'self' 'unsafe-inline'; font-src 'self'; img-src 'self' data:; connect-src 'self'; worker-src 'self'; object-src 'none'; frame-ancestors 'self'" if emulator else "default-src 'self'; script-src 'self'; style-src 'self'; img-src 'self' data:; connect-src 'self'; frame-src 'self'; object-src 'none'; frame-ancestors 'none'"
        if urlparse(self.path).path == '/faq':
            css = re.search(r'<style>(.*?)</style>', (STATIC/'faq.html').read_text('utf-8'), re.S).group(1)
            style_hash = base64.b64encode(hashlib.sha256(css.encode()).digest()).decode()
            csp = csp.replace("style-src 'self'", "style-src 'self' 'sha256-"+style_hash+"'")
        self.send_header('Content-Security-Policy', csp)
        self.end_headers()
        self.wfile.write(data)

    def valid_host(self):
        return self.headers.get('Host') in {f'127.0.0.1:{self.server.server_port}', f'localhost:{self.server.server_port}'}

    def do_GET(self):
        if not self.valid_host():
            return self.send({'error': 'Loopback host required'}, status=403)
        u = urlparse(self.path)
        try:
            with LOCK:
                if u.path == '/api/state':
                    return self.send(state())
                if u.path == '/api/emulator':
                    return self.send(emulator_state())
                if u.path == '/api/rex/status':
                    return self.send(rex_job(parse_qs(u.query).get('id',[''])[0]))
                if u.path == '/api/session':
                    return self.send({'token': TOKEN})
                if u.path == '/api/file':
                    return self.send(file_record(parse_qs(u.query).get('path', [''])[0]))
                routes = {'/': ('index.html', 'text/html; charset=utf-8'), '/app.js': ('app.js', 'text/javascript; charset=utf-8'), '/style.css': ('style.css', 'text/css; charset=utf-8'), '/faq': ('faq.html', 'text/html; charset=utf-8'), '/faq.css': ('faq.css', 'text/css; charset=utf-8')}
                if u.path in routes:
                    name, mime = routes[u.path]
                    return self.send((STATIC / name).read_bytes(), mime)
                if u.path.startswith('/rex/'):
                    parts=u.path.split('/')
                    if len(parts)!=4: return self.send({'error':'Not found'},status=404)
                    job=rex_job(parts[2]);name=parts[3]
                    allowed=[job.get(k,'').rsplit('/',1)[-1] for k in ['audioUrl','markersUrl','csvUrl','licenseUrl','attributionUrl']]
                    if job['status']!='done' or name not in allowed or not name:
                        return self.send({'error':'Not found'},status=404)
                    path=safe_file(DATA/'rex-imports'/parts[2]/'bundle',name)
                    mime='audio/wav' if name.endswith('.wav') else 'application/json' if name.endswith('.json') else 'text/csv' if name.endswith('.csv') else 'text/plain; charset=utf-8'
                    return self.send(path.read_bytes(),mime)
                if u.path.startswith('/emu/'):
                    name = u.path.removeprefix('/emu/') or 'index.html'
                    allowed = {'index.html': 'text/html; charset=utf-8', 'emu.css': 'text/css', 'emu.js': 'text/javascript', 'slice-import.js': 'text/javascript', 'slice-ui.js': 'text/javascript', 'variant-ui.js': 'text/javascript', 'variant-ui.css': 'text/css', 'arrangement-ui.js': 'text/javascript', 'arrangement-ui.css': 'text/css', 'bass-ui.js': 'text/javascript', 'bass-ui.css': 'text/css', 'pad-ui.js': 'text/javascript', 'pad-ui.css': 'text/css', 'lane-ui.js': 'text/javascript', 'lane-ui.css': 'text/css', 'midi-router.mjs': 'text/javascript', 'midi-worklet.mjs': 'text/javascript', 'worklet.js': 'text/javascript', 'x0x.wasm': 'application/wasm', 'BarlowSemiCondensed-Medium.ttf': 'font/ttf', 'manifest.json': 'application/json'}
                    if name == 'manual.txt':
                        return self.send((WORK / 'docs/MANUAL.md').read_bytes(), 'text/plain; charset=utf-8')
                    if name not in allowed or not (WORK / 'build/emu' / name).is_file():
                        return self.send({'error': 'Emulator not built or asset not found'}, status=404)
                    return self.send((WORK / 'build/emu' / name).read_bytes(), allowed[name])
                if u.path.startswith('/exports/'):
                    path = safe_file(DATA / 'exports', u.path.removeprefix('/exports/'))
                    return self.send(path.read_bytes(), 'application/zip')
                return self.send({'error': 'Not found'}, status=404)
        except (ValueError, UnicodeError) as e:
            self.send({'error': str(e)}, status=400)

    def do_POST(self):
        origin = self.headers.get('Origin')
        origins = {f'http://127.0.0.1:{self.server.server_port}', f'http://localhost:{self.server.server_port}'}
        if not self.valid_host() or (origin and origin not in origins) or self.headers.get('X-Lab-Token') != TOKEN:
            return self.send({'error': 'Local session required'}, status=403)
        try:
            size = int(self.headers.get('Content-Length', '0'))
            route = urlparse(self.path).path
            if not 0 < size <= (24*1024*1024 if route=='/api/rex/convert' else 2*MAX_FILE):
                raise ValueError('Invalid request size')
            body = json.loads(self.rfile.read(size))
            with LOCK:
                if route == '/api/rex/convert':
                    return self.send(start_rex_conversion(body['name'],body['data']))
                if route in {'/api/save', '/api/restore'}:
                    if BUILD['running']:
                        return self.send({'error': 'Wait for the emulator build to finish before editing source.'}, status=409)
                    name = body['path']
                    current = file_record(name)
                    if body.get('sha') != current['sha']:
                        return self.send({'error': 'File changed since opening. Reopen it before saving.'}, status=409)
                    folder = DATA / 'backups' / digest(name.encode())[:24]
                    if route == '/api/restore':
                        backup = body['backup']
                        if backup not in [b['id'] for b in current['backups']]:
                            raise ValueError('Backup not found')
                        content = (folder / backup).read_bytes()
                    else:
                        content = body['content'].encode('utf-8')
                    if len(content) > MAX_FILE:
                        raise ValueError('File exceeds editing limit')
                    if content == safe_file(WORK, name).read_bytes():
                        return self.send(current)
                    backup_id = time.strftime('%Y%m%d-%H%M%S', time.gmtime()) + '-' + secrets.token_hex(4) + '.bak'
                    atomic(folder / backup_id, safe_file(WORK, name).read_bytes())
                    atomic(safe_file(WORK, name), content)
                    record('restore' if route.endswith('restore') else 'save', path=name, backup=backup_id, before=current['sha'], after=digest(content))
                    return self.send(file_record(name))
                if route == '/api/targets':
                    targets = json.loads((DATA / 'targets.json').read_text('utf-8'))
                    target = next((t for t in targets if t['id'] == body['id']), None)
                    if not target or body.get('status') not in {'todo', 'doing', 'review', 'blocked', 'done'}:
                        raise ValueError('Invalid target')
                    notes = body.get('notes', '')
                    if not isinstance(notes, str) or len(notes) > 6000:
                        raise ValueError('Notes too long')
                    if body['status'] == 'done' and not notes.strip():
                        raise ValueError('Record acceptance evidence before marking done.')
                    target.update(status=body['status'], notes=notes)
                    atomic(DATA / 'targets.json', json.dumps(targets, indent=2).encode())
                    record('target', detail=target['title'], status=target['status'])
                    return self.send({'ok': True})
                if route == '/api/checks':
                    return self.send(checks())
                if route == '/api/emulator/build':
                    start_emulator_build()
                    return self.send({'running': True})
                if route == '/api/export':
                    name = 'fm1-x0x-source-' + time.strftime('%Y%m%d-%H%M%S', time.gmtime()) + '-' + secrets.token_hex(3) + '.zip'
                    output = DATA / 'exports' / name
                    output.parent.mkdir(exist_ok=True)
                    with zipfile.ZipFile(output, 'w', zipfile.ZIP_DEFLATED) as z:
                        for f in files():
                            z.write(WORK / f['path'], 'fm1-x0x/' + f['path'])
                        z.writestr('MOD-LAB-CHANGES.json', json.dumps({'archive_sha256': state()['archive_sha'], 'changes': [f['path'] for f in files() if f['changed']], 'targets': state()['targets'], 'checks': state()['checks']}, indent=2))
                    record('export', detail=name)
                    return self.send({'url': '/exports/' + name, 'name': name})
                return self.send({'error': 'Not found'}, status=404)
        except (ValueError, KeyError, TypeError, UnicodeError) as e:
            self.send({'error': str(e)}, status=400)

def initialize(baseline_ref=UPSTREAM_REF):
    global ORIGINAL, BASELINE
    if not (WORK / 'firmware/src').is_dir() or not (WORK / 'web/emu').is_dir():
        raise SystemExit('Expected an FM1 Git checkout with firmware/src and web/emu. Set --source-root if necessary.')
    if WORK.resolve().is_relative_to(DATA.resolve()):
        raise SystemExit('Runtime data must be a separate directory, not the source root or its parent.')
    DATA.mkdir(parents=True, exist_ok=True)
    ORIGINAL, BASELINE = prepare_baseline(WORK, DATA, baseline_ref)
    if not (DATA / 'targets.json').exists():
        seed = HERE / 'seeds/targets.json'
        content = seed.read_bytes() if seed.is_file() else json.dumps([{'id': i, 'title': title, 'acceptance': acceptance, 'status': status, 'area': area, 'notes': ''} for i,title,acceptance,status,area in TARGETS], indent=2).encode()
        atomic(DATA / 'targets.json', content)
    (DATA / 'history.jsonl').touch(exist_ok=True)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('port', nargs='?', type=int, default=8097)
    parser.add_argument('--source-root', type=Path, default=WORK)
    parser.add_argument('--data-dir', type=Path, default=None)
    parser.add_argument('--rex-helper', type=Path, default=REX_HELPER)
    parser.add_argument('--rex-sdk', type=Path, default=REX_SDK)
    args = parser.parse_args()
    WORK = args.source_root.expanduser().resolve()
    DATA = (args.data_dir or configured_path('FM1_LAB_DATA') or WORK / '.acultur8-lab').expanduser().resolve()
    REX_HELPER = args.rex_helper.expanduser().resolve() if args.rex_helper else None
    REX_SDK = args.rex_sdk.expanduser().resolve() if args.rex_sdk else None
    initialize()
    port = args.port
    server = ThreadingHTTPServer(('127.0.0.1', port), Handler)
    print(f'FM1 Mod Lab: http://127.0.0.1:{port}/', flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        server.server_close()

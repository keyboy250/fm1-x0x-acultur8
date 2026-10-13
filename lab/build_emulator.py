"""Build the shared firmware emulator from a portable checkout. No device operations."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import time
from lab_paths import configured_path

HERE = Path(__file__).resolve().parent
WORK = configured_path('FM1_SOURCE_ROOT', HERE.parent)
IMAGE = os.environ.get('FM1_BUILDER_IMAGE', 'acultur8-fm1-emu-builder:local')
OUTPUT = WORK / 'build/emu'

def run(args):
    print('Running: ' + ' '.join(args), flush=True)
    subprocess.run(args, check=True)


def docker_identity():
    # Linux bind mounts retain the checkout owner's UID/GID. With capabilities
    # dropped, container root cannot write another user's non-world-writable tree.
    return ['--user', f'{os.getuid()}:{os.getgid()}'] if hasattr(os, 'getuid') else []

def source_hash(root=WORK):
    root = Path(root).resolve()
    h = hashlib.sha256()
    paths = []
    for folder in ['firmware', 'host', 'tools', 'assets', 'web/emu']:
        paths += [p for p in (root / folder).rglob('*') if p.is_file() and '__pycache__' not in p.parts]
    for p in sorted(paths, key=lambda p: p.relative_to(root).as_posix()):
        h.update(p.relative_to(root).as_posix().encode() + b'\0' + p.read_bytes())
    # Packaging changes affect the built browser even when firmware is unchanged.
    for name in ['build_emulator.py', 'emulator.Dockerfile', 'lab_paths.py']:
        p = root / 'lab' / name
        if p.is_file():
            h.update(('lab/' + name).encode() + b'\0' + p.read_bytes())
    return h.hexdigest()

def package_page(work=None, output=None, home_url='/', storage_namespace='acultur8-fm1-lab'):
    work = Path(work or WORK).resolve()
    output = Path(output or work / 'build/emu').resolve()
    output.mkdir(parents=True, exist_ok=True)
    if not re.fullmatch(r'[a-zA-Z0-9_-]+', storage_namespace):
        raise ValueError('Storage namespace must contain only letters, numbers, hyphens and underscores.')
    if not re.fullmatch(r'[a-zA-Z0-9_./#-]+', home_url):
        raise ValueError('Home URL must be a local path.')
    html = (work / 'web/emu/index.html').read_text('utf-8')
    html = re.sub(r'<link[^>]+fonts.googleapis.com[^>]+>\s*', '', html)
    css = re.search(r'<style>(.*?)</style>', html, re.S).group(1)
    js = re.search(r'<script>(.*?)</script>', html, re.S).group(1)
    js = js.replace('"x0x-emu-store"', '"' + storage_namespace + '-store-v1"')
    js = js.replace('"x0x-emu-slots"', '"' + storage_namespace + '-slots-v1"')
    js = js.replace('indexedDB.open("x0x-emu",', 'indexedDB.open("' + storage_namespace + '-loops-v1",')
    js = js.replace('if (m.type === "ready") res();', 'if (m.type === "ready") res();\n        window.dispatchEvent(new CustomEvent("fm1-marked", {detail:m}));')
    # Keep the original else-if chain intact: dispatch must happen before its first if.
    js = js.replace('if (m.type === "ready") res();\n        window.dispatchEvent(new CustomEvent("fm1-marked", {detail:m}));\n        else if', 'window.dispatchEvent(new CustomEvent("fm1-marked", {detail:m}));\n        if (m.type === "ready") res();\n        else if')
    js = js.replace('syncButtons(); syncKeys();\n  } catch', 'syncButtons(); syncKeys();\n    window.dispatchEvent(new Event("fm1-ready"));\n  } catch')
    js = js.replace('function send(m) { if (node) node.port.postMessage(m); }',
        'let uiInputTicket = 0;\nwindow.addEventListener("fm1-ready", () => { uiInputTicket = 0; });\nfunction send(m) { if (node) { if (m.type === "buttons") { '
        'm = {...m, inputTicket: uiInputTicket = ((uiInputTicket + 1) >>> 0) || 1}; '
        'window.dispatchEvent(new CustomEvent("fm1-input", {detail:m})); } node.port.postMessage(m); } }')
    js += '\nwindow.fm1SliceSend = send;\n'
    html = re.sub(r'<style>.*?</style>', '<link rel="stylesheet" href="emu.css">', html, flags=re.S)
    html = re.sub(r'<script>.*?</script>', '<script src="emu.js"></script>', html, flags=re.S)
    panels = '\n'.join((work / 'web/emu' / name).read_text('utf-8') for name in ['variant-panel.html', 'arrangement-panel.html', 'bass-panel.html', 'pad-panel.html', 'slice-panel.html'])
    html = html.replace('<section class="breaks"', panels + '\n<section class="breaks"', 1)
    html = html.replace('</head>', '<link rel="stylesheet" href="variant-ui.css">\n<link rel="stylesheet" href="arrangement-ui.css">\n<link rel="stylesheet" href="bass-ui.css">\n<link rel="stylesheet" href="pad-ui.css">\n</head>')
    html = html.replace('</body>', '<script type="module" src="slice-ui.js"></script>\n<script type="module" src="variant-ui.js"></script>\n<script type="module" src="arrangement-ui.js"></script>\n<script type="module" src="bass-ui.js"></script>\n<script type="module" src="pad-ui.js"></script>\n</body>')
    html = html.replace('<title>X0X', '<title>Acultur8 Mod Lab · X0X')
    html = html.replace('href="../"', 'href="' + home_url + '" target="_top"')
    html = html.replace('<a href="../manual/">Manual</a>', '<a href="manual.txt" target="_blank">Manual</a>')
    html = html.replace('<a href="../install/">Install</a>', '')
    html = html.replace('in the browser: the same code the FM-1 runs', 'Acultur8 working firmware · browser simulation · no device connection')
    css = '@font-face{font-family:Barlow;src:url(BarlowSemiCondensed-Medium.ttf)}\n' + css + '\n.help{grid-template-columns:repeat(auto-fit,minmax(min(100%,16rem),1fr))}main{min-width:0;overflow-wrap:anywhere}header{flex-wrap:wrap}.slot{min-width:0}\n'
    css += '\n.slice-lab{margin:24px 0;padding:18px;border:1px solid #45413a;border-radius:10px;background:#17191d}.slice-files{display:flex;flex-wrap:wrap;gap:12px}.slice-files label{display:grid;gap:6px;min-width:0;flex:1}.slice-files input{max-width:100%;color:#bbb}.slice-lab p{font-size:13px;color:#a6aab1}.slice-lab .slice-error{color:#f0a29d}.slice-lab canvas{display:block;width:100%;height:140px;background:#0b0e12;border-radius:6px}#slice-list{display:grid;grid-template-columns:repeat(auto-fit,minmax(min(100%,230px),1fr));gap:6px;margin:12px 0}#slice-list button{font:11px monospace;text-align:left;padding:9px;background:#23262b;color:#ddd;border:1px solid #444;border-radius:5px}#slice-list .slice-active{border-color:#f0a043;color:#f0a043}.slice-lab button:disabled{opacity:.4}.slice-lab details{font-size:12px}.slice-lab input[type=range]{width:100px}.slice-lab summary{cursor:pointer}\n'
    css += '#slice-export-json{display:block;box-sizing:border-box;width:100%;background:#0b0e12;color:#ddd;font:11px monospace;padding:12px;border:1px solid #555;border-radius:5px}\n'
    css += '.slice-files label{grid-template-columns:minmax(0,1fr)}.slice-files input{min-width:0;width:100%;box-sizing:border-box}\n'
    (output / 'index.html').write_text(html, encoding='utf-8', newline='\n')
    (output / 'emu.css').write_text(css, encoding='utf-8', newline='\n')
    (output / 'emu.js').write_text(js, encoding='utf-8', newline='\n')
    shutil.copy2(work / 'assets/fonts/BarlowSemiCondensed-Medium.ttf', output)
    for name in ['slice-import.js', 'slice-ui.js', 'variant-ui.js', 'variant-ui.css', 'arrangement-ui.js', 'arrangement-ui.css', 'bass-ui.js', 'bass-ui.css', 'pad-ui.js', 'pad-ui.css', 'midi-router.mjs', 'midi-worklet.mjs']:
        shutil.copy2(work / 'web/emu' / name, output)
    if storage_namespace != 'acultur8-fm1-lab':
        slice_ui = output / 'slice-ui.js'
        slice_ui.write_text(slice_ui.read_text('utf-8').replace('acultur8-fm1-marked-imports-v1', storage_namespace + '-marked-imports-v1'), encoding='utf-8', newline='\n')
    shutil.copy2(work / 'docs/MANUAL.md', output / 'manual.txt')

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path, default=WORK)
    parser.add_argument('--output', type=Path, help='Copy the packaged build to this directory.')
    parser.add_argument('--image', default=IMAGE)
    parser.add_argument('--bootstrap', action='store_true', help='Build the local Docker builder from the digest-pinned Dockerfile (network required).')
    parser.add_argument('--native', action='store_true', help='Use emcc, a host C compiler and Pillow already installed on this Linux host.')
    parser.add_argument('--package-only', action='store_true', help='Refresh browser assets around an existing build; does not compile firmware.')
    args = parser.parse_args()
    work = args.source_root.expanduser().resolve()
    output = work / 'build/emu'
    if not (work / 'web/emu/build.sh').is_file():
        parser.error('Source root must contain web/emu/build.sh.')
    if args.native and args.bootstrap:
        parser.error('--bootstrap builds Docker; omit --native when bootstrapping.')
    if args.package_only:
        if not (output / 'x0x.wasm').is_file():
            parser.error('--package-only requires an existing build/emu/x0x.wasm.')
        package_page(work, output)
        if args.output and args.output.resolve() != output:
            shutil.copytree(output, args.output, dirs_exist_ok=True)
        print('Browser assets packaged; existing compilation manifest retained for stale-build detection.')
        return
    initial = source_hash(work)
    if not args.native:
        if args.bootstrap:
            # Only the Dockerfile is sent as build context, never local data or SDKs.
            with tempfile.TemporaryDirectory(prefix='fm1-builder-') as context:
                shutil.copy2(HERE / 'emulator.Dockerfile', Path(context) / 'Dockerfile')
                run(['docker', 'build', '--tag', args.image, context])
        try:
            run(['docker', 'image', 'inspect', args.image, '--format', '{{.Id}}'])
        except subprocess.CalledProcessError:
            parser.exit(1, 'Builder image unavailable. Run this command with --bootstrap once, then rebuild offline.\n')
    # Always regenerate headers: changes to assets, fonts or sound defaults must not leave stale output.
    script = '''set -e
mkdir -p build/gen build/host build/emu
python3 tools/gen_font.py build/gen/felucca_font.h
python3 tools/gen_drum_samples.py build/gen/x0x_drum_samples.h
sh tools/gen_builtin_break.sh build/gen
BINARYEN_CORES=1 sh web/emu/build.sh
X0X_HOST_DEFS=-D_POSIX_C_SOURCE=200809L sh host/build_host.sh
'''
    script_path = work / 'build/build-emulator-container.sh'
    script_path.parent.mkdir(parents=True, exist_ok=True)
    script_path.write_text(script, encoding='utf-8', newline='\n')
    if args.native:
        subprocess.run(['sh', str(script_path)], cwd=work, check=True)
        compiler = subprocess.check_output(['emcc', '--version'], text=True).splitlines()[0]
        image_id = None
    else:
        run(['docker', 'run', '--rm', '--network=none', '--cap-drop=ALL', '--security-opt=no-new-privileges', *docker_identity(), '-v', str(work) + ':/src', '-w', '/src', args.image, 'sh', '/src/build/build-emulator-container.sh'])
        compiler = subprocess.check_output(['docker', 'run', '--rm', '--network=none', '--cap-drop=ALL', '--security-opt=no-new-privileges', args.image, 'emcc', '--version'], text=True).splitlines()[0]
        image_id = subprocess.check_output(['docker', 'image', 'inspect', args.image, '--format', '{{.Id}}'], text=True).strip()
    if source_hash(work) != initial:
        raise RuntimeError('Source changed during build. Rebuild before accepting the emulator.')
    package_page(work, output)
    manifest = {'built_at': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()), 'source_sha256': initial, 'wasm_sha256': hashlib.sha256((output / 'x0x.wasm').read_bytes()).hexdigest(), 'wasm_bytes': (output / 'x0x.wasm').stat().st_size, 'compiler': compiler, 'image_id': image_id}
    (output / 'manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    if args.output and args.output.resolve() != output:
        shutil.copytree(output, args.output, dirs_exist_ok=True)
    print(json.dumps(manifest, indent=2), flush=True)

if __name__ == '__main__':
    main()

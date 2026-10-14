"""Browser packaging smoke checks without a compiler, audio device or decoder."""
from pathlib import Path
import hashlib
import shutil
import subprocess
import tempfile
import unittest

from build_emulator import WORK, package_page, source_hash


class BuildPackagingTest(unittest.TestCase):
    def test_local_package_keeps_working_storage_and_audio_bridge(self):
        with tempfile.TemporaryDirectory(prefix='fm1-package-test-') as folder:
            output = Path(folder)
            package_page(WORK, output)
            page = (output / 'index.html').read_text('utf-8')
            script = (output / 'emu.js').read_text('utf-8')
            self.assertNotIn('fonts.googleapis.com', page)
            self.assertNotIn('<style>', page)
            self.assertNotIn('<script>', page)
            self.assertIn('src="slice-ui.js"', page)
            self.assertEqual(page.count('id="variant-title"'), 1)
            self.assertEqual(page.count('src="variant-ui.js"'), 1)
            self.assertIn('href="variant-ui.css"', page)
            self.assertEqual(page.count('id="arrangement-title"'), 1)
            self.assertEqual(page.count('src="arrangement-ui.js"'), 1)
            self.assertIn('href="arrangement-ui.css"', page)
            self.assertEqual(page.count('id="pad-title"'), 1)
            self.assertEqual(page.count('src="pad-ui.js"'), 1)
            self.assertIn('href="pad-ui.css"', page)
            self.assertEqual(page.count('id="lane-title"'), 1)
            self.assertEqual(page.count('src="lane-ui.js"'), 1)
            self.assertIn('href="lane-ui.css"', page)
            self.assertEqual(page.count('id="cc-input-title"'), 1)
            self.assertEqual(page.count('src="cc-input-ui.js"'), 1)
            self.assertIn('href="cc-input-ui.css"', page)
            for name in ('variant-ui.js', 'variant-ui.css', 'arrangement-ui.js', 'arrangement-ui.css',
                         'bass-ui.js', 'bass-ui.css', 'pad-ui.js', 'pad-ui.css', 'lane-ui.js', 'lane-ui.css',
                         'cc-input-ui.js', 'cc-input-ui.css', 'cc-input-router.mjs', 'cc-input-worklet.mjs', 'midi-router.mjs', 'midi-worklet.mjs',
                         'slice-control.mjs', 'slice-worklet.mjs'):
                self.assertEqual((output / name).read_bytes(), (WORK / 'web/emu' / name).read_bytes())
            self.assertIn('href="manual.txt"', page)
            self.assertIn('window.fm1SliceSend = send', script)
            self.assertIn('new Event("fm1-ready")', script)
            self.assertIn('new CustomEvent("fm1-marked"', script)
            self.assertIn('acultur8-fm1-lab-store-v1', script)
            self.assertIn("from './slice-control.mjs'", (output / 'slice-ui.js').read_text('utf-8'))
            self.assertIn('acultur8-fm1-marked-imports-v1', (output / 'slice-control.mjs').read_text('utf-8'))
            # The compiler stage copies worklet.js; package_page supplies its dependencies.
            self.assertIn("from './slice-worklet.mjs'", (WORK / 'web/emu/worklet.js').read_text('utf-8'))
            self.assertEqual((output / 'manual.txt').read_bytes(), (WORK / 'docs/MANUAL.md').read_bytes())
            if shutil.which('node'):
                subprocess.run(['node', '--check', str(output / 'emu.js')], check=True)
                # Execute the generated input boundary: intent precedes posting,
                # wrap skips the reserved zero, and engine restart resets tickets.
                test = r'''
const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const js=fs.readFileSync(process.argv[1],'utf8');
const part=js.slice(js.indexOf('let uiInputTicket ='),js.indexOf('function syncButtons()'));
const events=[],handlers={},posted=[];
const context=vm.createContext({node:{port:{postMessage:m=>{assert.equal(events.at(-1).detail.inputTicket,m.inputTicket);posted.push(m);}}},
  window:{addEventListener:(n,f)=>handlers[n]=f,dispatchEvent:e=>events.push(e)},
  CustomEvent:class{constructor(type,options){this.type=type;this.detail=options.detail;}}});
vm.runInContext(part,context);vm.runInContext('uiInputTicket=0xfffffffe; send({type:"buttons",mask:1024}); send({type:"buttons",mask:0});',context);
assert.equal(posted[0].inputTicket,0xffffffff);assert.equal(posted[1].inputTicket,1);
handlers['fm1-ready']();vm.runInContext('send({type:"buttons",mask:0})',context);assert.equal(posted[2].inputTicket,1);
'''
                subprocess.run(['node', '-e', test, str(output / 'emu.js')], check=True)

    def test_static_namespace_and_relative_home(self):
        with tempfile.TemporaryDirectory(prefix='fm1-package-test-') as folder:
            output = Path(folder)
            package_page(WORK, output, home_url='../', storage_namespace='acultur8-fm1-alpha')
            self.assertIn('href="../" target="_top"', (output / 'index.html').read_text('utf-8'))
            self.assertIn('src="variant-ui.js"', (output / 'index.html').read_text('utf-8'))
            self.assertIn('src="arrangement-ui.js"', (output / 'index.html').read_text('utf-8'))
            self.assertEqual((output / 'variant-ui.js').read_bytes(), (WORK / 'web/emu/variant-ui.js').read_bytes())
            script = (output / 'emu.js').read_text('utf-8')
            self.assertIn('acultur8-fm1-alpha-store-v1', script)
            self.assertNotIn('acultur8-fm1-lab-store-v1', script)
            service = (output / 'slice-control.mjs').read_text('utf-8')
            self.assertIn('acultur8-fm1-alpha-marked-imports-v1', service)
            self.assertNotIn('acultur8-fm1-marked-imports-v1', service)
            self.assertIn("from './slice-control.mjs'", (output / 'slice-ui.js').read_text('utf-8'))

    def test_source_fingerprint_uses_portable_case_sensitive_order(self):
        with tempfile.TemporaryDirectory(prefix='fm1-hash-order-') as folder:
            root = Path(folder)
            (root / 'firmware').mkdir()
            entries = [('firmware/Z.c', b'last upper'), ('firmware/b.c', b'lower'), ('firmware/A.c', b'first upper')]
            for name, content in entries:
                (root / name).write_bytes(content)
            expected = hashlib.sha256()
            for name, content in sorted(entries):
                expected.update(name.encode() + b'\0' + content)
            self.assertEqual(source_hash(root), expected.hexdigest())

    def test_source_hash_ignores_runtime_but_tracks_packager(self):
        with tempfile.TemporaryDirectory(prefix='fm1-hash-test-') as folder:
            root = Path(folder)
            (root / 'lab').mkdir()
            source = root / 'lab/build_emulator.py'
            source.write_text('# package one\n', encoding='utf-8')
            initial = source_hash(root)
            (root / '.acultur8-lab').mkdir()
            (root / '.acultur8-lab/private.rx2').write_bytes(b'private loop')
            self.assertEqual(initial, source_hash(root))
            source.write_text('# package two\n', encoding='utf-8')
            self.assertNotEqual(initial, source_hash(root))

    def test_source_hash_tracks_cc_input_modules(self):
        with tempfile.TemporaryDirectory(prefix='fm1-cc-hash-test-') as folder:
            root = Path(folder)
            (root / 'web/emu').mkdir(parents=True)
            for name in ('cc-input-router.mjs', 'cc-input-worklet.mjs', 'cc-input-ui.js', 'cc-input-ui.css', 'cc-input-panel.html',
                         'slice-control.mjs', 'slice-worklet.mjs'):
                before = source_hash(root)
                (root / 'web/emu' / name).write_text('original', encoding='utf-8')
                added = source_hash(root)
                self.assertNotEqual(before, added, name)
                (root / 'web/emu' / name).write_text('changed', encoding='utf-8')
                self.assertNotEqual(added, source_hash(root), name)


if __name__ == '__main__':
    unittest.main(verbosity=2)

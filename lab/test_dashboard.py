"""Integration checks in a disposable workspace, never the user's working source."""
import importlib.util
import base64
import hashlib
import re
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import threading
import time
import unittest
from unittest.mock import patch
from urllib.error import HTTPError
from urllib.request import Request, urlopen
import zipfile

spec = importlib.util.spec_from_file_location('lab', Path(__file__).with_name('server.py'))
lab = importlib.util.module_from_spec(spec)
spec.loader.exec_module(lab)

class DashboardTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory(prefix='fm1-dashboard-test-')
        lab.BASE = Path(cls.temp.name)
        lab.WORK = lab.BASE / 'clone'
        # A real clean clone proves this no longer relies on the author's archive folders.
        repository = Path(__file__).resolve().parent.parent
        subprocess.run(['git', 'clone', '--quiet', '--no-hardlinks', '--local', str(repository), str(lab.WORK)], check=True)
        lab.DATA = lab.WORK / '.acultur8-lab'
        lab.REX_HELPER = lab.REX_SDK = None
        lab.initialize()
        cls.server = lab.ThreadingHTTPServer(('127.0.0.1', 0), lab.Handler)
        cls.thread = threading.Thread(target=cls.server.serve_forever, daemon=True)
        cls.thread.start()
        cls.url = f'http://127.0.0.1:{cls.server.server_port}'

    @classmethod
    def tearDownClass(cls):
        cls.server.shutdown()
        cls.server.server_close()
        cls.temp.cleanup()

    def request(self, path, body=None, headers=None):
        h = {'X-Lab-Token': lab.TOKEN, 'Content-Type': 'application/json'}
        h.update(headers or {})
        request = Request(self.url + path, data=json.dumps(body).encode() if body is not None else None, headers=h)
        try:
            with urlopen(request) as response:
                data = response.read()
                return response.status, json.loads(data) if 'application/json' in response.headers['Content-Type'] else data
        except HTTPError as e:
            return e.code, json.loads(e.read())

    def test_faq_standalone_style_allowed_by_csp(self):
        with urlopen(self.url+'/faq') as response:
            # HTML parsing normalizes CRLF before computing inline style hashes.
            page=response.read().decode('utf-8').replace('\r\n','\n')
            policy=response.headers['Content-Security-Policy']
        css=re.search(r'<style>(.*?)</style>',page,re.S).group(1)
        encoded=base64.b64encode(hashlib.sha256(css.encode()).digest()).decode()
        self.assertIn("'sha256-"+encoded+"'",policy)
        self.assertNotIn("'unsafe-inline'",policy)
        self.assertEqual(self.request('/faq.css')[1].decode('utf-8').replace('\r\n','\n'),css)
        self.assertIn('Play with FM1',page)

    def test_rex_boundaries_and_downloads(self):
        self.assertEqual(self.request('/api/rex/status?id=../source')[0],400)
        self.assertEqual(self.request('/api/rex/convert', {'name':'../loop.rx2','data':'AA=='})[0],400)
        self.assertEqual(self.request('/api/rex/convert', {'name':'loop.rx2','data':'AA=='}, {'X-Lab-Token':'wrong'})[0],403)
        identifier='a'*20
        folder=lab.DATA/'rex-imports'/identifier
        (folder/'bundle').mkdir(parents=True)
        (folder/'bundle/loop_slices.wav').write_bytes(b'wav fixture')
        (folder/'private.rx2').write_bytes(b'private source')
        (folder/'job.json').write_text(json.dumps({'id':identifier,'status':'done','audioUrl':f'/rex/{identifier}/loop_slices.wav'}))
        self.assertEqual(self.request(f'/rex/{identifier}/loop_slices.wav'),(200,b'wav fixture'))
        self.assertEqual(self.request(f'/rex/{identifier}/private.rx2')[0],404)
        self.assertEqual(self.request(f'/rex/{identifier}/job.json')[0],404)
        self.assertEqual(self.request('/faq')[0],200)

    def test_configured_external_converter_contract(self):
        # Synthetic helper exercises only process/manifest integration. No decoder or REX audio is used.
        helper = lab.BASE / 'external helper fixture.py'
        helper.write_text('''import argparse,json
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('source');p.add_argument('--output');p.add_argument('--sdk');a=p.parse_args()
out=Path(a.output);out.mkdir();stem=Path(a.source).stem
(out/(stem+'_slices.wav')).write_bytes(b'RIFF synthetic fixture')
(out/(stem+'_markers.json')).write_text(json.dumps({'audioFile':stem+'_slices.wav','slices':[{'time':0}],'bpm':120,'loopDuration':2}))
for name in [stem+'_markers.csv','REX-SDK-LICENSE.txt','ATTRIBUTION.txt']:(out/name).write_text('Synthetic integration fixture')
''', encoding='utf-8')
        old_helper, old_sdk = lab.REX_HELPER, lab.REX_SDK
        lab.REX_HELPER, lab.REX_SDK = helper, lab.BASE
        try:
            with patch.object(lab, 'rex_ready', return_value=True):
                status, started = self.request('/api/rex/convert', {'name':'synthetic.rx2','data':'AA=='})
            self.assertEqual(status, 200)
            deadline = time.monotonic() + 10
            job = {'status': 'running'}
            while job['status'] == 'running' and time.monotonic() < deadline:
                job = self.request('/api/rex/status?id=' + started['id'])[1]
                time.sleep(0.03)
            self.assertEqual(job['status'], 'done', job)
            self.assertEqual(job['bpm'], 120)
            self.assertEqual(self.request(job['audioUrl']), (200, b'RIFF synthetic fixture'))
            self.assertEqual(self.request('/rex/' + job['id'] + '/synthetic.rx2')[0], 404)
        finally:
            lab.REX_HELPER, lab.REX_SDK = old_helper, old_sdk
            helper.unlink()

    def test_save_conflict_and_restore_exact_bytes(self):
        name = 'firmware/src/app/x0x.h'
        baseline = (lab.ORIGINAL / name).read_bytes()
        original_working_bytes = (lab.WORK / name).read_bytes()
        status, before = self.request('/api/file?path=' + name)
        self.assertEqual(status, 200)
        status, after = self.request('/api/save', {'path': name, 'content': before['content'] + '\n/* mod lab test */\n', 'sha': before['sha']})
        self.assertEqual(status, 200)
        self.assertTrue(after['changed'])
        self.assertIn('mod lab test', after['diff'])
        self.assertEqual(self.request('/api/save', {'path': name, 'content': '', 'sha': before['sha']})[0], 409)
        status, restored = self.request('/api/restore', {'path': name, 'sha': after['sha'], 'backup': after['backups'][0]['id']})
        self.assertEqual(status, 200)
        self.assertEqual(restored['changed'], before['changed'])
        self.assertEqual((lab.WORK / name).read_bytes(), original_working_bytes)
        self.assertEqual((lab.ORIGINAL / name).read_bytes(), baseline)
        self.assertEqual(len(restored['backups']), 2)

    def test_path_and_asset_boundaries(self):
        for name in ['../README.md', '/README.md', 'C:/Windows/system.ini', 'firmware\\src\\app\\x0x.h', 'assets/909/hh.wav']:
            self.assertEqual(self.request('/api/file?path=' + name)[0], 400, name)
        self.assertEqual(self.request('/server.py')[0], 404)
        self.assertEqual(self.request('/api/save', {'path': 'README.md', 'sha': '', 'content': 'bad'}, {'X-Lab-Token': 'wrong'})[0], 403)
        self.assertEqual(self.request('/api/checks', {}, {'Origin': 'https://foreign.example'})[0], 403)
        self.assertEqual(self.request('/api/state', headers={'Host': 'foreign.example'})[0], 403)

    def test_targets_and_evidence(self):
        self.assertEqual(self.request('/api/targets', {'id': 'FM1-03', 'status': 'done', 'notes': ''})[0], 400)
        self.assertEqual(self.request('/api/targets', {'id': 'FM1-03', 'status': 'review', 'notes': 'Simulator acceptance pending'})[0], 200)
        state = self.request('/api/state')[1]
        self.assertEqual(next(t for t in state['targets'] if t['id'] == 'FM1-03')['notes'], 'Simulator acceptance pending')

    def test_portable_baseline_and_optional_decoder(self):
        status, state = self.request('/api/state')
        self.assertEqual(status, 200)
        self.assertEqual(state['baseline_commit'], lab.UPSTREAM_REF)
        self.assertRegex(state['archive_sha'], r'^[0-9a-f]{64}$')
        self.assertFalse(state['rex_ready'])
        self.assertEqual(self.request('/api/rex/convert', {'name':'loop.rx2','data':'AA=='})[0], 400)
        paths = [item['path'] for item in state['files']]
        self.assertNotIn('.acultur8-lab/targets.json', paths)
        self.assertNotIn('.git/config', paths)
        self.assertEqual(self.request('/api/file?path=.acultur8-lab/targets.json')[0], 400)
        self.assertEqual(self.request('/api/file?path=.git/config')[0], 400)
        # A prepared baseline must reopen offline without invoking Git again.
        with patch('lab_paths.subprocess.run', side_effect=AssertionError('Unexpected command')):
            baseline, metadata = lab.prepare_baseline(lab.WORK, lab.DATA)
        self.assertEqual(baseline, lab.ORIGINAL)
        self.assertEqual(metadata['commit'], lab.UPSTREAM_REF)

    def test_external_runtime_and_decoder_are_excluded_from_exports(self):
        original_data, original_sdk, original_helper = lab.DATA, lab.REX_SDK, lab.REX_HELPER
        try:
            lab.DATA = lab.WORK / 'private-runtime'
            lab.REX_SDK = lab.WORK / 'private-sdk'
            lab.REX_HELPER = lab.WORK / 'private-converter.py'
            lab.DATA.mkdir(exist_ok=True)
            lab.REX_SDK.mkdir(exist_ok=True)
            (lab.DATA / 'loop.rx2').write_bytes(b'private loop')
            (lab.REX_SDK / 'SDK.h').write_text('restricted fixture', encoding='utf-8')
            lab.REX_HELPER.write_text('restricted fixture', encoding='utf-8')
            for name in ['private-runtime/loop.rx2', 'private-sdk/SDK.h', 'private-converter.py']:
                self.assertNotIn(name, [item['path'] for item in lab.files()])
                self.assertEqual(self.request('/api/file?path=' + name)[0], 400)
        finally:
            for folder in [lab.DATA, lab.REX_SDK]:
                shutil.rmtree(folder)
            lab.REX_HELPER.unlink()
            lab.DATA, lab.REX_SDK, lab.REX_HELPER = original_data, original_sdk, original_helper

    def test_new_source_file_is_editable(self):
        name = 'firmware/src/dsp/new_slice_test.c'
        (lab.WORK / name).write_text('/* newly added source */\n', encoding='utf-8')
        status, opened = self.request('/api/file?path=' + name)
        self.assertEqual(status, 200)
        self.assertTrue(opened['changed'])
        self.assertIn('+/* newly added source */', opened['diff'])
        status, saved = self.request('/api/save', {'path': name, 'sha': opened['sha'], 'content': opened['content'] + '/* edit */\n'})
        self.assertEqual(status, 200)
        self.assertIn('/* edit */', saved['content'])

    def test_emulator_routes_and_stale_build(self):
        output = lab.WORK / 'build/emu'
        output.mkdir(parents=True, exist_ok=True)
        for name in ['index.html', 'emu.js', 'emu.css', 'worklet.js']:
            (output / name).write_text('test fixture', encoding='utf-8')
        wasm = b'\x00asm\x01\x00\x00\x00'
        (output / 'x0x.wasm').write_bytes(wasm)
        (output / 'manifest.json').write_text(json.dumps({'source_sha256': lab.source_hash(lab.WORK), 'wasm_sha256': lab.digest(wasm)}), encoding='utf-8')
        state = self.request('/api/emulator')[1]
        self.assertTrue(state['ready'])
        self.assertFalse(state['stale'])
        self.assertEqual(self.request('/emu/x0x.wasm'), (200, wasm))
        self.assertEqual(self.request('/emu/../server.py')[0], 404)
        path = lab.WORK / 'firmware/src/app/x0x.h'
        old = path.read_bytes()
        path.write_bytes(old + b'\n/* stale check */\n')
        self.assertTrue(self.request('/api/emulator')[1]['stale'])
        path.write_bytes(old)
        lab.BUILD['running'] = True
        try:
            self.assertEqual(self.request('/api/save', {'path': 'README.md', 'sha': '', 'content': 'bad'})[0], 409)
        finally:
            lab.BUILD['running'] = False
        self.assertEqual(self.request('/api/emulator/build', {}, {'X-Lab-Token': 'wrong'})[0], 403)

    def test_checks_export_and_static(self):
        status, checks = self.request('/api/checks', {})
        self.assertEqual(status, 200)
        self.assertEqual(checks['results'][0]['status'], 'pass')
        self.assertIn('blocked', [r['status'] for r in checks['results']])
        status, result = self.request('/api/export', {})
        self.assertEqual(status, 200)
        self.assertEqual(self.request(result['url'])[0], 200)
        with zipfile.ZipFile(lab.DATA / 'exports' / result['name']) as z:
            self.assertIn('fm1-x0x/LICENSE', z.namelist())
            self.assertIn('fm1-x0x/firmware/src/app/ui.c', z.namelist())
            self.assertIn('MOD-LAB-CHANGES.json', z.namelist())
            self.assertFalse(any('.acultur8-lab/' in name or '/.git/' in name for name in z.namelist()))
        for path in ['/', '/app.js', '/style.css']:
            self.assertEqual(self.request(path)[0], 200)

if __name__ == '__main__':
    unittest.main(verbosity=2)

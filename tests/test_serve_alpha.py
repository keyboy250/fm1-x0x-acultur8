# SPDX-License-Identifier: GPL-3.0-only
"""Production packaged-server snapshot, MIME and loopback guards over real HTTP."""
import hashlib
import http.client
import importlib.util
import json
from pathlib import Path
import tempfile
import threading
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location('test_alpha_server', ROOT / 'tools/serve_alpha.py')
alpha = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(alpha)


class AlphaServerTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.site = Path(self.temp.name) / 'site'
        self.site.mkdir()
        self.payloads = {'index.html': b'<html>alpha</html>',
                         'emu/index.html': b'<html>emulator</html>',
                         'emu/app.js': b'export const app = 1;\n',
                         'emu/bridge.mjs': b'export const bridge = 2;\n',
                         'emu/x0x.wasm': b'\0asm\x01\0\0\0'}
        for name, data in self.payloads.items():
            p = self.site / name
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_bytes(data)
        (self.site / 'private-sdk.txt').write_bytes(b'PRIVATE_UNLISTED_FIXTURE')
        self.entries = {name: hashlib.sha256(data).hexdigest() for name, data in self.payloads.items()}
        self.manifest = self.site / 'release-manifest.json'
        self.manifest.write_text(json.dumps({'stamp': 'original', 'files': self.entries}), encoding='utf-8')
        self.server = None

    def tearDown(self):
        if self.server:
            self.server.shutdown()
            self.server.server_close()
            self.thread.join(timeout=3)
            self.assertFalse(self.thread.is_alive(), 'HTTP fixture must stop')
        self.temp.cleanup()

    def start(self):
        self.server = alpha.ThreadingHTTPServer(('127.0.0.1', 0), alpha.handler_for(alpha.package_files(self.site)))
        self.thread = threading.Thread(target=self.server.serve_forever, daemon=True)
        self.thread.start()
        self.assertEqual(self.server.server_address[0], '127.0.0.1')

    def request(self, path, method='GET', host=True):
        conn = http.client.HTTPConnection('127.0.0.1', self.server.server_port, timeout=3)
        try:
            conn.putrequest(method, path, skip_host=True)
            if host is True:
                host = f'127.0.0.1:{self.server.server_port}'
            if host is not None:
                conn.putheader('Host', host)
            conn.endheaders()
            response = conn.getresponse()
            return response.status, dict(response.getheaders()), response.read()
        finally:
            conn.close()

    def test_get_head_module_mime_and_content(self):
        self.start()
        cases = [('/', 'index.html', 'text/html; charset=utf-8'),
                 ('/emu/', 'emu/index.html', 'text/html; charset=utf-8'),
                 ('/emu/app.js?cache=1', 'emu/app.js', 'text/javascript'),
                 ('/emu/bridge.mjs', 'emu/bridge.mjs', 'text/javascript'),
                 ('/emu/x0x.wasm', 'emu/x0x.wasm', 'application/wasm')]
        for route, name, mime in cases:
            for method in ['GET', 'HEAD']:
                with self.subTest(route=route, method=method):
                    status, headers, data = self.request(route, method)
                    self.assertEqual(status, 200)
                    self.assertEqual(headers['Content-Type'], mime)
                    self.assertEqual(headers['Content-Length'], str(len(self.payloads[name])))
                    self.assertEqual(headers['X-Content-Type-Options'], 'nosniff')
                    self.assertEqual(headers['Cache-Control'], 'no-store')
                    self.assertEqual(data, self.payloads[name] if method == 'GET' else b'')
        status, headers, body = self.request('/release-manifest.json')
        self.assertEqual(status, 200)
        self.assertEqual(headers['Content-Type'], 'application/json')
        self.assertEqual(body, self.manifest.read_bytes())

    def test_no_directory_private_or_traversal_serving(self):
        self.start()
        for path in ['/private-sdk.txt', '/tools/serve_alpha.py', '/emu', '/other/',
                     '/emu/../private-sdk.txt', '/emu/%2e%2e/private-sdk.txt',
                     '/%2e/emu/app.js', '/emu/%5c..%5cprivate-sdk.txt',
                     '/emu//app.js', '/emu/%00app.js']:
            for method in ['GET', 'HEAD']:
                with self.subTest(path=path, method=method):
                    status, _, data = self.request(path, method)
                    self.assertEqual(status, 404)
                    self.assertNotIn(b'PRIVATE_UNLISTED_FIXTURE', data)
                    if method == 'HEAD':
                        self.assertEqual(data, b'')

    def test_foreign_missing_or_wrong_port_host_rejected(self):
        self.start()
        for host in ['foreign.example', '127.0.0.1:1', 'localhost', None]:
            for method in ['GET', 'HEAD']:
                with self.subTest(host=host, method=method):
                    status, _, data = self.request('/emu/app.js', method, host)
                    self.assertEqual(status, 403)
                    self.assertNotIn(self.payloads['emu/app.js'], data)
        status, _, data = self.request('/emu/app.js', host=f'localhost:{self.server.server_port}')
        self.assertEqual((status, data), (200, self.payloads['emu/app.js']))

    def test_served_bytes_are_verified_immutable_snapshot(self):
        self.start()
        (self.site / 'emu/app.js').write_bytes(b'TAMPERED_AFTER_START')
        self.manifest.write_text('{"files":{}}', encoding='utf-8')
        status, _, data = self.request('/emu/app.js')
        self.assertEqual((status, data), (200, self.payloads['emu/app.js']))
        status, _, body = self.request('/release-manifest.json')
        self.assertEqual(status, 200)
        self.assertEqual(json.loads(body)['stamp'], 'original')
        self.assertEqual(json.loads(body)['files'], self.entries)

    def test_hash_tamper_missing_and_escape_manifest_fail_before_serving(self):
        valid = self.manifest.read_bytes()
        (self.site / 'emu/app.js').write_bytes(b'TAMPERED_BEFORE_START')
        with self.assertRaisesRegex(ValueError, 'hash mismatch'):
            alpha.package_files(self.site)
        (self.site / 'emu/app.js').write_bytes(self.payloads['emu/app.js'])
        malformed = [({'files': {'emu/missing.js': '0' * 64}}, 'path is invalid'),
                     ({'files': {'emu/app.js': '0' * 64}}, 'hash mismatch'),
                     ({'files': {'../private.txt': '0' * 64}}, 'path is invalid')]
        outside = self.site.parent / 'private.txt'
        outside.write_bytes(b'OUTSIDE_PRIVATE')
        malformed.append(({'files': {str(outside.resolve()): hashlib.sha256(outside.read_bytes()).hexdigest()}}, 'path is invalid'))
        for manifest, reason in malformed:
            with self.subTest(manifest=manifest):
                self.manifest.write_text(json.dumps(manifest), encoding='utf-8')
                with self.assertRaisesRegex(ValueError, reason):
                    alpha.package_files(self.site)
        self.manifest.write_text('not JSON', encoding='utf-8')
        with self.assertRaises(json.JSONDecodeError):
            alpha.package_files(self.site)
        self.manifest.write_bytes(valid)
        self.assertEqual(len(alpha.package_files(self.site)), len(self.entries) + 1)

    def test_manifest_parse_and_serve_use_one_coherent_read(self):
        original = self.manifest.read_bytes()
        replacement = json.dumps({'stamp': 'changed-during-read', 'files': self.entries}).encode()
        read_bytes, read_text = Path.read_bytes, Path.read_text
        reads = []

        def observe(path, data):
            if path == self.manifest:
                reads.append(True)
                path.write_bytes(replacement)
            return data

        def raced_bytes(path):
            return observe(path, read_bytes(path))

        def raced_text(path, *args, **kwargs):
            return observe(path, read_text(path, *args, **kwargs))

        with patch.object(Path, 'read_bytes', raced_bytes), patch.object(Path, 'read_text', raced_text):
            files = alpha.package_files(self.site)
        self.assertEqual(len(reads), 1, 'manifest must be parsed and served from one snapshot')
        self.assertEqual(files['/release-manifest.json'][0], original)
        self.assertEqual(self.manifest.read_bytes(), replacement)


if __name__ == '__main__':
    unittest.main()

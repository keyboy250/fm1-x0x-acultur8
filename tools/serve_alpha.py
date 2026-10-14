#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Serve a verified browser-alpha package on loopback with portable module MIME types."""
import argparse
import hashlib
import json
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import unquote, urlsplit

MIME = {'.html': 'text/html; charset=utf-8', '.js': 'text/javascript',
        '.mjs': 'text/javascript', '.css': 'text/css', '.json': 'application/json',
        '.wasm': 'application/wasm', '.ttf': 'font/ttf', '.svg': 'image/svg+xml',
        '.txt': 'text/plain; charset=utf-8', '.wav': 'audio/wav'}


def package_files(site):
    site = Path(site).resolve()
    manifest = site / 'release-manifest.json'
    manifest_data = manifest.read_bytes()
    entries = json.loads(manifest_data)['files']
    files = {}
    for name, expected in entries.items():
        file = (site / name).resolve()
        if not file.is_relative_to(site) or not file.is_file():
            raise ValueError('Package path is invalid: ' + name)
        data = file.read_bytes()
        if hashlib.sha256(data).hexdigest() != expected:
            raise ValueError('Package hash mismatch: ' + name)
        files['/' + name] = (data, MIME.get(file.suffix.lower(), 'application/octet-stream'))
    files['/release-manifest.json'] = (manifest_data, 'application/json')
    return files


def handler_for(files):
    class Handler(BaseHTTPRequestHandler):
        def do_GET(self):
            self.respond(False)

        def do_HEAD(self):
            self.respond(True)

        def respond(self, head):
            if self.headers.get('Host') not in {
                    f'127.0.0.1:{self.server.server_port}', f'localhost:{self.server.server_port}'}:
                self.send_error(403)
                return
            path = unquote(urlsplit(self.path).path)
            if '\\' in path or any(part in {'.', '..'} for part in path.split('/')):
                self.send_error(404)
                return
            if path.endswith('/'):
                path += 'index.html'
            item = files.get(path)
            if item is None:
                self.send_error(404)
                return
            data, mime = item
            self.send_response(200)
            self.send_header('Content-Type', mime)
            self.send_header('Content-Length', str(len(data)))
            self.send_header('Cache-Control', 'no-store')
            self.send_header('X-Content-Type-Options', 'nosniff')
            self.end_headers()
            if not head:
                self.wfile.write(data)
    return Handler


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--site', required=True, type=Path)
    parser.add_argument('--port', type=int, default=8130)
    args = parser.parse_args()
    files = package_files(args.site)
    with ThreadingHTTPServer(('127.0.0.1', args.port), handler_for(files)) as server:
        print(f'Verified {len(files)-1} files. http://127.0.0.1:{args.port}/emu/', flush=True)
        server.serve_forever()


if __name__ == '__main__':
    main()

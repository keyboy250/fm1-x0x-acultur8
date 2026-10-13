# SPDX-License-Identifier: GPL-3.0-only
"""Publication boundaries, link resolution and synthetic-bank provenance."""
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
import wave
import sys

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('site_builder', ROOT / 'tools/acultur8_site.py')
site = importlib.util.module_from_spec(spec)
spec.loader.exec_module(site)


class StaticSiteTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)

    def tearDown(self):
        self.temp.cleanup()

    def test_project_subpath_links(self):
        (self.root / 'emu').mkdir()
        (self.root / 'index.html').write_text('<a href="emu/#sound">Play</a>')
        (self.root / 'emu/index.html').write_text('<h1 id="sound">Sound</h1><a href="../">Home</a>')
        self.assertEqual(site.validate_site(self.root), 2)
        (self.root / 'emu/index.html').write_text('<h1 id="sound">Sound</h1><a href="/faq">Bad project root</a>')
        with self.assertRaisesRegex(ValueError, 'Root-relative'):
            site.validate_site(self.root)

    def test_missing_asset_or_anchor_fails(self):
        (self.root / 'index.html').write_text('<script src="missing.js"></script>')
        with self.assertRaisesRegex(ValueError, 'Missing site target'):
            site.validate_site(self.root)
        (self.root / 'index.html').write_text('<a href="#missing">Missing</a>')
        with self.assertRaisesRegex(ValueError, 'Missing site anchor'):
            site.validate_site(self.root)

    def test_escape_or_sdk_rejected(self):
        (self.root / 'index.html').write_text('<a href="../../other">Escape</a>')
        with self.assertRaisesRegex(ValueError, 'escapes'):
            site.validate_site(self.root)
        (self.root / 'index.html').write_text('<h1>Fine</h1>')
        for name in ('private.rx2', 'decoder.dll', 'device.fwsc'):
            forbidden = self.root / name
            forbidden.write_bytes(b'synthetic boundary fixture')
            with self.assertRaisesRegex(ValueError, 'Unexpected public artifact'):
                site.validate_site(self.root)
            forbidden.unlink()

    def test_demo_is_repeatable_and_timing_is_separate(self):
        site.demo(self.root / 'first')
        site.demo(self.root / 'second')
        name = 'acultur8-demo_slices.wav'
        self.assertEqual((self.root / 'first' / name).read_bytes(), (self.root / 'second' / name).read_bytes())
        marker = json.loads((self.root / 'first/acultur8-demo_markers.json').read_text())
        self.assertEqual(marker['bpm'], 120)
        self.assertEqual(len(marker['slices']), 6)
        self.assertNotEqual(marker['slices'][1]['startSample'] / marker['sampleRate'], marker['slices'][1]['time'])
        with wave.open(str(self.root / 'first' / name)) as wav:
            self.assertEqual(wav.getnframes(), marker['audioFrames'])
            self.assertEqual(wav.getnchannels(), 1)
        self.assertEqual(hashlib.sha256((self.root / 'first' / name).read_bytes()).hexdigest(), marker['audioSha256'])

    def test_nonempty_destination_preserved(self):
        original = self.root / 'keep.txt'
        original.write_text('user data')
        with self.assertRaisesRegex(ValueError, 'Output must be empty'):
            site.package(self.root)
        self.assertEqual(original.read_text(), 'user data')

    def test_shared_kick_component_survives_static_allowlist(self):
        # Packaging fixture only; no claim about executable WASM or audio.
        sys.path.insert(0, str(ROOT / 'lab'))
        from build_emulator import package_page, source_hash
        emu = self.root / 'build'
        package_page(ROOT, emu)
        (emu / 'worklet.js').write_bytes((ROOT / 'web/emu/worklet.js').read_bytes())
        (emu / 'x0x.wasm').write_bytes(b'packaging boundary fixture')
        (emu / 'manifest.json').write_text(json.dumps({
            'source_sha256': source_hash(ROOT), 'wasm_sha256': site.digest(emu / 'x0x.wasm')}))
        out = self.root / 'public'
        site.package(out, emu)
        page = (out / 'emu/index.html').read_text('utf-8')
        self.assertEqual(page.count('id="variant-title"'), 1)
        self.assertIn('src="variant-ui.js"', page)
        self.assertIn('href="variant-ui.css"', page)
        self.assertEqual(page.count('id="arrangement-title"'), 1)
        self.assertIn('src="arrangement-ui.js"', page)
        self.assertIn('href="arrangement-ui.css"', page)
        for name in ('variant-ui.js', 'variant-ui.css', 'arrangement-ui.js', 'arrangement-ui.css'):
            self.assertEqual((out / 'emu' / name).read_bytes(), (ROOT / 'web/emu' / name).read_bytes())
        self.assertNotIn('/api/rex/', (out / 'emu/slice-ui.js').read_text('utf-8'))
        site.validate_site(out)


if __name__ == '__main__':
    unittest.main(verbosity=2)

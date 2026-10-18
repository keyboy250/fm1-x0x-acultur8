"""Serve the actual packaged editor through the local HTTP allowlist, without Git writes."""
from pathlib import Path
import tempfile
import threading
import unittest
from urllib.error import HTTPError
from urllib.request import urlopen

from build_emulator import package_page
import server as lab


class ArrangementPackageHTTPTest(unittest.TestCase):
    def test_real_package_assets_and_private_boundaries(self):
        source = Path(__file__).resolve().parent.parent
        original_work = lab.WORK
        with tempfile.TemporaryDirectory(prefix='fm1-arrangement-http-') as folder:
            lab.WORK = Path(folder)
            output = lab.WORK / 'build/emu'
            package_page(source, output)
            http = lab.ThreadingHTTPServer(('127.0.0.1', 0), lab.Handler)
            thread = threading.Thread(target=http.serve_forever, daemon=True)
            thread.start()
            base = f'http://127.0.0.1:{http.server_port}/emu/'
            try:
                with urlopen(base) as response:
                    page = response.read().decode('utf-8')
                    self.assertEqual(page.count('id="arrangement-title"'), 1)
                    self.assertIn('src="arrangement-ui.js"', page)
                    self.assertIn('href="arrangement-ui.css"', page)
                for name, mime in [('arrangement-ui.js', 'text/javascript'), ('arrangement-ui.css', 'text/css')]:
                    with urlopen(base + name) as response:
                        self.assertEqual(response.status, 200)
                        self.assertEqual(response.headers['Content-Type'], mime)
                        self.assertEqual(response.read(), (source / 'web/emu' / name).read_bytes())
                for name in ('arrangement-panel.html', 'private.rx2', 'decoder.dll'):
                    with self.assertRaises(HTTPError) as caught:
                        urlopen(base + name)
                    self.assertEqual(caught.exception.code, 404)
            finally:
                http.shutdown()
                http.server_close()
                thread.join()
                lab.WORK = original_work


if __name__ == '__main__':
    unittest.main(verbosity=2)

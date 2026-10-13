#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Single mandatory browser-alpha build/regression command; no optional-pass branches.

python tools/acultur8_check.py --bootstrap
Requires Python, Git and Docker. Builds with the pinned emulator container,
runs host/WASM/parser/sync/worklet/dashboard/site checks, and writes the tested
static artifact path and provenance to build/acultur8-checks.json.
"""
import argparse
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'lab'))
from build_emulator import docker_identity


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bootstrap', action='store_true', help='Build the pinned Docker toolchain if missing')
    parser.add_argument('--image', default='acultur8-fm1-emu-builder:local')
    args = parser.parse_args()
    build = ROOT / 'build'
    build.mkdir(exist_ok=True)
    report = {'status': 'running', 'startedAt': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()),
              'checks': [], 'outsideBrowserAlphaCoverage': [
                  'External schwung DSP reference comparisons (run tests/host/run_*.sh with those source checkouts)',
                  'Native REX SDK converter tests (optional local decoder setup)',
                  'Target firmware image/RAM/update-loader and physical-device qualification']}
    report_path = build / 'acultur8-checks.json'

    def save():
        report_path.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')

    def run(name, command):
        print(f'\n=== {name} ===', flush=True)
        subprocess.run(command, cwd=ROOT, check=True)
        report['checks'].append({'name': name, 'status': 'pass'})
        save()

    save()
    try:
        command = [sys.executable, 'lab/build_emulator.py', '--image', args.image]
        if args.bootstrap:
            command.append('--bootstrap')
        run('fresh-host-and-wasm-build', command)
        docker = ['docker', 'run', '--rm', '--network=none', '--cap-drop=ALL',
                  '--security-opt=no-new-privileges', *docker_identity(), '-v', str(ROOT) + ':/src', '-w', '/src', args.image]
        run('required-engine-regressions', docker + ['sh', 'tools/acultur8_checks.sh'])
        run('portable-dashboard', [sys.executable, 'lab/test_dashboard.py'])
        run('emulator-page-bridge', [sys.executable, 'lab/test_build_emulator.py'])
        run('arrangement-http-assets', [sys.executable, 'lab/test_arrangement_package_http.py'])
        run('target-budget-checker-regressions', [sys.executable, 'tests/test_target_budget.py'])
        run('publication-boundaries', [sys.executable, 'tests/acultur8_site_test.py'])
        site = Path(tempfile.mkdtemp(prefix='site-', dir=build))
        run('static-package', [sys.executable, 'tools/acultur8_site.py', '--output', str(site)])
        relative = site.relative_to(ROOT).as_posix()
        run('published-demo-parser-and-wasm', docker + ['node', 'tests/host/acultur8_demo_test.mjs', relative])
        report['site'] = relative
        report['status'] = 'pass'
    except (subprocess.CalledProcessError, OSError) as error:
        report['status'] = 'fail'
        report['error'] = str(error)
        raise
    finally:
        report['finishedAt'] = time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime())
        save()
    print(f'\nAll required browser-alpha checks passed. Artifact: {report["site"]}', flush=True)


if __name__ == '__main__':
    main()

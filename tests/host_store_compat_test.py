# SPDX-License-Identifier: GPL-3.0-only
"""Run actual host CLI against legacy, current and truncated whole-flash dumps."""
from pathlib import Path
import struct
import subprocess
import sys
import tempfile


def main():
    executable = str(Path(sys.argv[1]).resolve())
    with tempfile.TemporaryDirectory(prefix='fm1-host-store-') as tmp:
        root = Path(tmp)

        def run(script, success=True):
            (root / 'check.x0x').write_text(script, encoding='utf-8')
            result = subprocess.run([executable, 'check.x0x', '.'], cwd=root,
                                    capture_output=True, text=True, timeout=30)
            assert (result.returncode == 0) == success, (script, result.returncode, result.stderr)
            if not success:
                assert 'is short' in result.stderr, result.stderr

        run('tap SAVE\nwait 100\nstoredump baseline.bin\n')
        baseline = (root / 'baseline.bin').read_bytes()
        assert len(baseline) % 8 == 0
        record = len(baseline) // 8
        capacity = record - 4
        assert capacity >= 212
        legacy = baseline[:7 * record]
        (root / 'legacy.bin').write_bytes(legacy)
        run('storeload legacy.bin\nstoredump restored.bin\n')
        restored = (root / 'restored.bin').read_bytes()
        assert restored[:7 * record] == legacy
        assert struct.unpack_from('<i', restored, 7 * record)[0] == -1

        config = struct.pack('<HHBBBB', 37, 16000, 3, 9, 133, 15)
        bank = struct.pack('<IHHI', 0x31464453, 1, 212, 1) + config + bytes(24 * 8)
        current = legacy + struct.pack('<i', len(bank)) + bank + bytes(capacity - len(bank))
        (root / 'current.bin').write_bytes(current)
        run('storeload current.bin\nstoredump roundtrip.bin\n')
        assert (root / 'roundtrip.bin').read_bytes() == current
        run('storeload current.bin\nstoreload legacy.bin\nstoredump replaced.bin\n')
        replaced = (root / 'replaced.bin').read_bytes()
        assert replaced[:7 * record] == legacy
        assert struct.unpack_from('<i', replaced, 7 * record)[0] == -1

        malformed = [legacy[:-1], legacy + b'\x01', legacy + b'\x01\x00\x00',
                     legacy + struct.pack('<i', 212), current[:-1]]
        for data in malformed:
            (root / 'short.bin').write_bytes(data)
            run('storeload short.bin\n', success=False)
    print('PASS actual host CLI: legacy seven-object load, absent defaults, eight-object byte-exact roundtrip; five truncated records rejected')


if __name__ == '__main__':
    main()

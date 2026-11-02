#!/bin/sh
set -eu
python3 - <<'PY'
from pathlib import Path
import zipfile,hashlib
archive=Path('/inputs/pre-overlay-source.zip')
assert hashlib.sha256(archive.read_bytes()).hexdigest()=='5b8b7e7f2bd717541e2063d693452b8726493f431302d318233dda697dcbe9a0'
out=Path('/tmp/optional-overlay-oracle');out.mkdir()
with zipfile.ZipFile(archive) as z:z.extractall(out)
for name in ['optional_overlay_test.c','run_optional_overlay.sh']:
    (out/'tests/host'/name).write_bytes((Path('/work/tests/host')/name).read_bytes())
p=out/'tests/host/optional_overlay_test.c';p.write_text('#define OPTIONAL_OVERLAY_ORACLE\n'+p.read_text(),encoding='utf-8')
p=out/'tests/host/block_bridge_test.c';s=p.read_text().replace('uint32_t plat_control_enter(void){','uint32_t plat_control_enter(void){OPTIONAL_CONTROL_ENTER_HOOK();',1);p.write_text(s,encoding='utf-8')
PY
cd /tmp/optional-overlay-oracle
sh tests/host/run_optional_overlay.sh

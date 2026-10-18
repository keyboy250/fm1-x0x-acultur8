"""Local paths and a recoverable, pinned upstream source comparison cache."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import secrets
import stat
import subprocess
import tempfile
import zipfile

UPSTREAM_REF = '47f8e4c4ad602591b81c21bc5480db7a94f6d6af'
REPO = Path(__file__).resolve().parent.parent


def configured_path(name, default=None):
    value = os.environ.get(name)
    return Path(value).expanduser().resolve() if value else default


def prepare_baseline(source, runtime, ref=UPSTREAM_REF):
    """Read only local Git objects. Never fetch, modify the checkout, or run hooks."""
    source, runtime = Path(source).resolve(), Path(runtime).resolve()
    cache = runtime / 'baseline' / ref
    stamp = cache / '.baseline.json'
    if stamp.is_file():
        metadata = json.loads(stamp.read_text('utf-8'))
        if metadata.get('commit') == ref and (cache / 'firmware').is_dir():
            return cache, metadata
    runtime.mkdir(parents=True, exist_ok=True)
    # Archive creation and extraction finish before the cache becomes visible.
    with tempfile.TemporaryDirectory(prefix='baseline-', dir=runtime) as scratch:
        temporary = Path(scratch)
        archive = temporary / 'upstream.zip'
        try:
            with archive.open('wb') as output:
                subprocess.run(['git', '-C', str(source), 'archive', '--format=zip', ref],
                               stdout=output, stderr=subprocess.PIPE, check=True, timeout=120)
        except (OSError, subprocess.CalledProcessError, subprocess.TimeoutExpired) as error:
            raise RuntimeError('Cannot read the pinned upstream Git commit. Use a full Git clone '
                               f'containing {ref}; the lab never downloads a baseline automatically.') from error
        extracted = temporary / 'tree'
        extracted.mkdir()
        with zipfile.ZipFile(archive) as bundle:
            for member in bundle.infolist():
                target = (extracted / member.filename).resolve()
                mode = member.external_attr >> 16
                if not target.is_relative_to(extracted) or stat.S_ISLNK(mode):
                    raise ValueError('Unsafe path or symlink in the upstream source archive.')
            bundle.extractall(extracted)
        if not (extracted / 'firmware').is_dir():
            raise ValueError('The pinned baseline does not contain firmware source.')
        metadata = {'commit': ref, 'archive_sha256': hashlib.sha256(archive.read_bytes()).hexdigest()}
        (extracted / '.baseline.json').write_text(json.dumps(metadata, indent=2), encoding='utf-8')
        cache.parent.mkdir(parents=True, exist_ok=True)
        if cache.exists():
            # An interrupted/corrupt cache is retained for inspection rather than deleted.
            checkpoint = cache.with_name(cache.name + '.incomplete-' + secrets.token_hex(4))
            cache.rename(checkpoint)
        shutil.move(str(extracted), str(cache))
    return cache, metadata

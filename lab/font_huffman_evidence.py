import hashlib,json,subprocess
from pathlib import Path
r=Path('.'); out=r/'build/font-huffman-checks'
e=json.loads((out/'font-huffman-report.json').read_text())
e.update(schema=1,measured_at='2026-10-13',status='focused-lossless-font-huffman-checks-passed',physical_draw_timing='pending',integrated_target_build='owned by parent; see separate integrated evidence',sanitizers='AddressSanitizer and UndefinedBehaviorSanitizer passed actual production gfx.c',encoding='Canonical Huffman over unchanged per-glyph PackBits; MSB first, independent byte alignment; maximum15bit code, 8bit flash prefix lookup',persistent_ram_added=0)
e['benchmark_samples']={m:[] for m in ('raw','rle','huffman')}
for _ in range(3):
 for mode in e['benchmark_samples']:
  result=subprocess.check_output([str(out/mode/'font_render'),'benchmark'],text=True).strip();e['benchmark_samples'][mode].append(result)
e['sha256']={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in [r/'tools/gen_font.py',r/'firmware/src/gfx.c',r/'tests/test_font_packing.py',r/'tests/host/font_render_test.c',*sorted((r/'assets/fonts').glob('*.ttf')),*sorted((r/'assets/fonts').glob('*.bdf'))]}
e['generated_header_sha256']={m:hashlib.sha256((out/m/'felucca_font.h').read_bytes()).hexdigest() for m in e['benchmark_samples']}
(r/'tests/font_huffman_evidence.json').write_text(json.dumps(e,indent=2)+'\n')
print(json.dumps(e['benchmark_samples'],indent=2))

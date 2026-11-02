#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Give an untouched archived DSP oracle the same decoded mu-law input assets.
Original-vs-codec loss is checked independently by tests/pcm_blocks_test.py.
This changes test assets only, never the pinned oracle's DSP/source code.
"""
import argparse,hashlib,json,struct,wave
from pathlib import Path
import gen_drum_samples as codec
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('reference',type=Path);p.add_argument('--source-commit',required=True);a=p.parse_args();ref=a.reference.resolve()
assert ref.is_relative_to((root/'build').resolve()) and ref!=root,'Only fresh ignored oracle archives may be prepared'
rows=[]
for _,name in codec.SAMPLES:
 source=root/'assets/909'/name;target=ref/'assets/909'/name
 original=source.read_bytes();assert target.read_bytes()==original,'Pinned input asset unexpectedly differs: '+name
 pcm,_=codec.read_wav(source);decoded=[codec.mulaw_dec(codec.mulaw_enc(v)) for v in pcm]
 with wave.open(str(target),'wb') as out:out.setnchannels(1);out.setsampwidth(2);out.setframerate(44100);out.writeframes(b''.join(struct.pack('<h',v) for v in decoded))
 rows.append({'asset':name,'original_sha256':hashlib.sha256(original).hexdigest(),'codec_matched_sha256':hashlib.sha256(target.read_bytes()).hexdigest(),'frames':len(pcm)})
(ref/'reference_codec_profile.json').write_text(json.dumps({'source_commit':a.source_commit,'profile':'decoded-G711-mulaw-inputs','source_code_modified':False,'codec_loss_test':'tests/pcm_blocks_test.py','assets':rows},indent=2)+'\n')
print('Prepared independent DSP oracle: original source, explicitly codec-matched cymbal assets')

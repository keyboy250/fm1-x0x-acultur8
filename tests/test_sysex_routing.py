#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compile actual ota/editor function bodies with deterministic mailbox interleaving.

Optional --parent-source runs the frozen archive as a negative control. Source
function hashes are recorded; the test never substitutes a test implementation.
"""
from pathlib import Path
import argparse, hashlib, json, os, re, subprocess, zipfile

ROOT = Path(__file__).resolve().parent.parent
FUNCTIONS = {'firmware/src/ota.c': ['ota_rd16','ota_rd24','ota_pack7','ota_unpack7','ota_take','ota_service'],
             'firmware/src/app/editor.c': ['ed_service']}

def extract(text, name):
    match = re.search(r'^static[^\n]*\b'+name+r'\([^;]*?\)\s*\{',text,re.M)
    assert match, name
    start = match.start(); pos = match.end(); depth = 1
    while depth:
        assert pos < len(text), name
        depth += (text[pos]=='{') - (text[pos]=='}'); pos += 1
    return text[start:pos]

def run(label, read, fixed, gap_fixed):
    out = ROOT/'build/sysex-routing';out.mkdir(parents=True,exist_ok=True)
    functions = {name:extract(read(path),name) for path,names in FUNCTIONS.items() for name in names}
    if fixed:
        functions['ota_editor_service']=extract(read('firmware/src/ota.c'),'ota_editor_service')
        main=re.sub(r'/\*.*?\*/','',read('firmware/src/app/main_fm1.c'),flags=re.S)
        main=re.sub(r'[ \t]+\n','\n',main)
        assert 'ed_service();\n        backup_service(ota_editor_service());' in main
        assert '        ota_service();\n        if (usb.ota_req)' in main, 'Recovery routing changed'
    harness=(ROOT/'tests/host/sysex_routing_harness.c').read_text().replace('/* ACTUAL_SOURCE_FUNCTIONS */','\n\n'.join(functions.values()))
    source=out/(label+'.c');source.write_text(harness)
    exe=out/label
    subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
                    '-fno-omit-frame-pointer','-DEXPECT_FIXED='+str(int(fixed)),
                    '-DEXPECT_GAP_FIXED='+str(int(gap_fixed)),str(source),'-o',str(exe)],check=True)
    env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1')
    result=subprocess.run([str(exe)],env=env,text=True,capture_output=True,check=True)
    print(result.stdout,end='')
    return {'name':label,'status':'PASS','output':result.stdout.strip(),
            'reply_arrival_cases':6000,'empty_observation_arrival_cases':6000,
            'empty_inbox_race_fixed':gap_fixed,
            'function_sha256':{name:hashlib.sha256(body.encode()).hexdigest() for name,body in functions.items()}}

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--parent-source',type=Path);args=parser.parse_args()
    results=[]
    if args.parent_source:
        with zipfile.ZipFile(args.parent_source) as z:
            results.append(run('parent-negative-control',lambda name:z.read(name).decode(),True,False))
    results.append(run('candidate-routing',lambda name:(ROOT/name).read_text(),True,True))
    (ROOT/'build/sysex-routing/routing-results.json').write_text(json.dumps({'status':'PASS','results':results,
        'scope':'Actual source functions; mocked mailbox/reply hardware boundaries; no physical MIDI acceptance'},indent=2)+'\n')

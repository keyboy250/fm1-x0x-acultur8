#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compile the production control wrappers and HAL gate bodies against an ICFG model.

Only the register reads/writes and csync instruction are simulated. This checks
exact state preservation for enabled, disabled and nested callers; physical IRQ
execution and deadlines require the device.
"""
from pathlib import Path
import subprocess, os, json, hashlib

ROOT = Path(__file__).resolve().parents[1]
hal = (ROOT/'firmware/hal/fm1_irq.h').read_text(encoding='utf-8')
start = hal.index('static inline uint32_t fm1_irq_gate_save(void)')
end = hal.index('\n\n', hal.index('static inline void fm1_irq_gate_restore',start))
helpers = hal[start:end]
assert helpers.count('__asm__ volatile("csync" ::: "memory");') == 1
helpers = helpers.replace('__asm__ volatile("csync" ::: "memory");','test_csync();')
plat = (ROOT/'firmware/src/app/plat_fm1.c').read_text(encoding='utf-8')
wrappers = '\n'.join(line for line in plat.splitlines() if line.startswith(('uint32_t plat_control_enter(', 'void plat_control_leave(')))
assert wrappers.count('\n') == 1
header = '''#include <stdint.h>
static uint32_t test_icfg=0x300u;
static unsigned test_sync;
static uint32_t fm1_icfg(void){return test_icfg;}
static void fm1_icfg_set(uint32_t value){test_icfg=value;}
static void test_csync(void){test_sync++;}
/* Legacy flash helpers intentionally model their unconditional enable. */
static uint32_t irq_save(void){test_icfg&=~0x100u;return 0;}
static void irq_restore(uint32_t value){(void)value;test_icfg|=0x100u;}
''' + helpers+'\n'+wrappers+'\n'
out = ROOT/'build/gen';out.mkdir(parents=True,exist_ok=True)
(out/'control_guard_model.h').write_text(header,encoding='utf-8',newline='\n')
test = '''#include <assert.h>
#include "control_guard_model.h"
int main(void){
 uint32_t values[]={0,0x100,0x200,0x300,0xa5a5a5a5,0xffffffff};
 for(unsigned i=0;i<sizeof(values)/sizeof(values[0]);i++){
  test_icfg=values[i];uint32_t outer=plat_control_enter();
  assert(test_icfg==(values[i]&~0x100u));
  uint32_t inner=plat_control_enter();plat_control_leave(inner);
  assert(test_icfg==(values[i]&~0x100u));
  plat_control_leave(outer);assert(test_icfg==values[i]);
 }
 assert(test_sync==12);return 0;
}
'''
p=ROOT/'build/control_guard_test.c';p.write_text(test,encoding='utf-8',newline='\n')
binary=ROOT/'build/host/control_guard_test';binary.parent.mkdir(exist_ok=True)
subprocess.run([os.environ.get('CC','cc'),'-std=c99','-O2','-Wall','-Wextra','-Wno-unused-function','-I'+str(out),str(p),'-o',str(binary)],check=True)
subprocess.run([str(binary)],check=True)
receipt={'test':'production control wrappers and HAL gate bodies','status':'pass','caller_states':6,'nested':True,'hal_sha256':hashlib.sha256((ROOT/'firmware/hal/fm1_irq.h').read_bytes()).hexdigest(),'platform_sha256':hashlib.sha256((ROOT/'firmware/src/app/plat_fm1.c').read_bytes()).hexdigest(),'physical_irq':False}
(ROOT/'build/control-guard-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8')
print(json.dumps(receipt))

/* SPDX-License-Identifier: GPL-3.0-only */
/* All compiled EXP values against the preserved original2432-word SHA.
 * Default mode calls real packed accessor; raw mode is pinned-oracle support.
 * Emit explicit LE bytes so the reference identity is host-endian independent. */
#define X0X_DRUM_TABLES_DEFINE
#include "x0x_drum_tables.h"
#include <stdio.h>
#include <assert.h>
int main(void){
 assert(sizeof(float)==4 && X0X_EXP_COUNT==19);
 for(unsigned curve=0;curve<X0X_EXP_COUNT;curve++)for(unsigned pot=0;pot<128;pot++){
  union {float f;uint32_t u;} word;word.f=x0x_pot_exp_value(curve,pot);
  uint8_t bytes[4]={(uint8_t)word.u,(uint8_t)(word.u>>8),(uint8_t)(word.u>>16),(uint8_t)(word.u>>24)};
  assert(fwrite(bytes,4,1,stdout)==1);
 }
 return 0;
}

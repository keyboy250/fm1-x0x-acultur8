/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "timing_rate_test_state.h"
int main(void){
 for(unsigned frames=0;frames<=1024;frames++){
  double ideal=(double)frames*24000000.0/X0X_TIMING_RATE_HZ;
  assert(fabs((double)timing_audio_ticks(frames)-ideal)<1.1);
 }
 printf("PASS profile%uHz:1025audio deadline offsets within1.1timer ticks\n",X0X_TIMING_RATE_HZ);
 return 0;
}

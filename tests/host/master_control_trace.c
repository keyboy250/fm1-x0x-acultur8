#include <stdint.h>
#include <stdio.h>
#include "../../firmware/src/app/master_control.h"
#ifndef MC_INITIAL
#define MC_INITIAL(q,o,f) {(q),MC_NO_ADC,(o),(f)}
#endif
int main(void){
    master_control_t m=MC_INITIAL(2048,MC_ADC,MC_HAS_ADC);uint32_t rng=1234567,hash=2166136261u;
    for(unsigned i=0;i<1000000;i++){
        rng=rng*1664525u+1013904223u;unsigned op=rng>>28,value=(rng>>8)%4200u;int rc=0;
        if(op<5)rc=master_control_adc(&m,value);
        else if(op<10)rc=master_control_set(&m,value,(rng>>4)%7u);
        else if(op<15)master_control_detach(&m,(rng>>5)%4u);
        else m=(master_control_t)MC_INITIAL(value%4097u,MC_ADC,MC_HAS_ADC);
        hash=(hash^master_control_state(&m))*16777619u;hash=(hash^(uint32_t)rc)*16777619u;
    }
    printf("%08x\n",hash);return 0;
}

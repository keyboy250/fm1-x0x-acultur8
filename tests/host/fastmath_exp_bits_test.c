/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "../../firmware/src/dsp/fastmath.h"
int main(void) {
    unsigned cases=0;
    for(int n=-126;n<=127;n++)for(int k=-64;k<=64;k++) {
        float f=(float)k/128.0f;
        fm_bits_t base,got; base.f=fm_exp2_nf(0,f);got.f=fm_exp2_nf(n,f);
        uint32_t expected=(uint32_t)((int64_t)base.u+(int64_t)n*8388608LL);
        assert(got.u==expected);cases++;
    }
    assert(fm_exp2_nf(-127,0.0f)==0.0f);
    printf("PASS exponent-bit parity: %u cases, negative/positive exponents, fractions and underflow.\n",cases);
    return 0;
}

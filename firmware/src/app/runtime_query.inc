/* SPDX-License-Identifier: GPL-3.0-only */
/* Command50 ABI1. No read-clear, save, CC, playback or command-queue operation. */
static POLY_COLD void ed_runtime_query(const uint8_t *a,uint32_t na)
{
    uint32_t w[RUNTIME_DIAGNOSTIC_WORDS],status=0,i,crc;
    unsigned token=na>1?a[1]:0,lane=na>2?a[2]:127;
    if(na!=3 || token>127 || lane>=24 || (na && a[0]>127))status=1;
    else if(a[0]!=1)status=3;
    else if(engine_runtime_snapshot(lane,w))status=2;
    ed_b(1);ed_b(token);ed_b(lane);ed_b(status);
    if(!status){ed_b(RUNTIME_DIAGNOSTIC_WORDS);for(i=0;i<RUNTIME_DIAGNOSTIC_WORDS;i++)ed_pq_u32(w[i]);}
    crc=st_crc32(ed_out+1,ed_n-1);ed_pq_u32(crc);
}

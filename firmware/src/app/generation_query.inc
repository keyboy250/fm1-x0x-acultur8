/* SPDX-License-Identifier: GPL-3.0-only */
/* Command51 ABI1:bounded read-only source recipe and result. */
static POLY_COLD void ed_generation_query(const uint8_t *a,uint32_t na)
{
    uint32_t w[24],status=0,i,crc;int count=0;
    unsigned token=na>1?a[1]:0,source=na>2?a[2]:127,page=na>3?a[3]:127,index=na>4?a[4]:127;
    if(na!=5 || token>127 || source>=3 || page>127 || index>127)status=1;
    else if(a[0]!=1)status=3;
    else if((count=cc_generation_snapshot(source,page,index,w))<0)status=3;
    ed_b(1);ed_b(token);ed_b(source);ed_b(page);ed_b(index);ed_b(status);
    if(!status){ed_b((uint32_t)count);for(i=0;i<(uint32_t)count;i++)ed_pq_u32(w[i]);}
    crc=st_crc32(ed_out+1,ed_n-1);ed_pq_u32(crc);
}

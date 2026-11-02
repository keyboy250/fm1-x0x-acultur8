/* SPDX-License-Identifier: GPL-3.0-only */
/* Command49: bounded RAM reads. No flush/save, reservation, selection or setter.
 * Replies reuse ed_out. Pages carry revision/ticket/epoch/project fingerprint;
 * clients accept only a complete same-stamp scan bracketed by summary pages. */
#ifndef PATTERN_QUERY_TEST_HOOK
#define PATTERN_QUERY_TEST_HOOK() ((void)0)
#endif

static POLY_COLD void ed_pq_u32(uint32_t v)
{int i;for(i=0;i<5;i++)ed_b(v>>(7*i));}

static POLY_COLD int ed_pq_motion(int pat,int ordinal)
{
    int k;
    for(k=0;k<NLANE;k++)if(proj.arr.lane[k].used && proj.arr.lane[k].pat==pat){
        if(!ordinal--)return k;
    }
    return -1;
}

/* Explicit wire fields, never raw C struct layouts, padding or pointers. */
static POLY_COLD uint8_t ed_pq_raw(int pat,int section,int index,int pos)
{
    const pattern_t *p=&proj.pat[pat];
    int poly=(proj.variant.arrangement.flags&POLY_FLAG)!=0;
    if(section==0){
        if(pos==0)return (uint8_t)(poly?(poly_expanded((const uint8_t *)&proj.sound)?2:1):0);
        if(pos==1)return p->swing;
        if(pos==2)return (uint8_t)ui_dirty();
        {int k,n=0;for(k=0;k<NLANE;k++)if(proj.arr.lane[k].used && proj.arr.lane[k].pat==pat)n++;return (uint8_t)n;}
    }
    if(section==2){
        const lane_t *l=&proj.arr.lane[index];
        if(pos==0)return l->part;
        if(pos==1)return l->t;
        if(pos==2)return l->v;
        if(pos==3)return l->i;
        return l->val[pos-4];
    }
    if(pos<2){
        if(poly)return (uint8_t)(pos?poly_rate(proj.pat,(const uint8_t *)&proj.sound,pat,index):poly_length(proj.pat,(const uint8_t *)&proj.sound,pat,index));
        if(index<22)return pos?p->drum[index/NDRUM].rate:p->drum[index/NDRUM].len;
        if(index<24)return pos?p->bass[index-22].rate:p->bass[index-22].len;
        return pos?RATE_16:16;
    }
    if(index<22){
        const dpart_t *d=&p->drum[index/NDRUM];
        const uint32_t *bits=pos<10?d->hit[index%NDRUM]:d->accent;
        int byte=pos<10?pos-2:pos-10;
        return (uint8_t)(bits[byte/4]>>(8*(byte%4)));
    }
    if(index<24){
        const bpart_t *b=&p->bass[index-22];
        if(pos==2)return b->dir;
        if(pos==3)return b->transpose;
        switch(pos){
        case 4:return b->gen.density; case 5:return b->gen.accent;
        case 6:return b->gen.slide; case 7:return b->gen.oct_range;
        case 8:return b->gen.root; case 9:return b->gen.scale;
        case 10:return b->gen.base_oct; case 11:return b->gen.mutate_bars;
        }
        if(pos<16)return (uint8_t)(b->gen.seed>>(8*(pos-12)));
        return (pos&1)?b->step[(pos-16)/2].flags:b->step[(pos-16)/2].note;
    }
    if(pos==2)return p->brk.slot_a;
    if(pos==3)return p->brk.slot_b;
    if(pos<4+BRK_NSET)return p->brk.set[pos-4];
    pos-=4+BRK_NSET;
    if(poly)return (uint8_t)((pos&1)?poly_break_slice(proj.pat,(const uint8_t *)&proj.sound,pat,pos/2):poly_break_gate(proj.pat,(const uint8_t *)&proj.sound,pat,pos/2));
    if(pos/2>=32)return 0;
    if(pos&1)return pos/2<16?p->brk.slice[pos/2]:0;
    return (uint8_t)((p->brk.steps>>(pos/2))&1u);
}

static POLY_COLD void ed_pattern_query(const uint8_t *a,uint32_t na)
{
    uint32_t rev=0,ticket=0,epoch=0,fp=0,status=0,crc;
    int pat=na>2u?a[2]:127,section=na>3u?a[3]:127,index=na>5u?(a[4]|(int)a[5]<<7):16383;
    int token=na>1u?a[1]:0,slot=index,len=0,k,j;
    if(na!=6u)status=1;
    else {
        for(k=0;k<6;k++)if(a[k]>127u)status=1;
        if(!status && a[0]!=1u)status=3;
        if(!status && (pat>=NPAT || section>2 || (section==0 && index) || (section==1 && index>=25) || (section==2 && index>=NLANE)))status=1;
        if(!status && section==2 && (slot=ed_pq_motion(pat,index))<0)status=1;
        if(!status && ui_pattern_query_busy())status=2;
    }
    ed_b(1);ed_b((uint32_t)token);ed_b((uint32_t)pat);ed_b((uint32_t)section);ed_b((uint32_t)index);ed_b((uint32_t)index>>7);ed_b(status);
    if(!status){
        rev=variant_current_revision();ticket=engine_queue_written();epoch=seq.transport_epoch;fp=project_fingerprint();
        ed_pq_u32(rev);ed_pq_u32(ticket);ed_pq_u32(epoch);ed_pq_u32(fp);
        len=section==0?4:section==2?68:index<22?18:index<24?144:4+BRK_NSET+128;
        ed_b((uint32_t)len);ed_b((uint32_t)len>>7);
        for(k=0;k<len;k+=7){
            uint32_t mask=0;
            for(j=0;j<7 && k+j<len;j++)mask|=(uint32_t)(ed_pq_raw(pat,section,slot,k+j)>>7)<<j;
            ed_b(mask);
            for(j=0;j<7 && k+j<len;j++)ed_b(ed_pq_raw(pat,section,slot,k+j));
        }
        PATTERN_QUERY_TEST_HOOK();
        if(ui_pattern_query_busy() || rev!=variant_current_revision() || ticket!=engine_queue_written() ||
           epoch!=seq.transport_epoch || fp!=project_fingerprint()){
            ed_n=12;ed_out[11]=2; /* reject raced page without exposing its data */
        }
    }
    crc=st_crc32(ed_out+1,ed_n-1);ed_pq_u32(crc);
}

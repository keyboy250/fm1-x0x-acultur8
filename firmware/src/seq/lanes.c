/* SPDX-License-Identifier: GPL-3.0-only */
#include "lanes.h"
/* Frozen descriptor tails plus whole unused voices. T_FX[1][0] is the TAPE
 * preference, even though engine_nvoices does not expose it. */
static const struct { uint16_t first,offset,count; } cells[] = {
    {0,11,5},
    {5,26,6},
    {11,41,7},
    {18,57,7},
    {25,73,7},
    {32,87,9},
    {41,104,8},
    {49,120,8},
    {57,136,8},
    {65,152,8},
    {73,168,8},
    {81,178,14},
    {95,202,6},
    {101,218,6},
    {107,233,7},
    {114,249,7},
    {121,265,7},
    {128,281,7},
    {135,298,6},
    {141,312,8},
    {149,328,8},
    {157,344,8},
    {165,360,8},
    {173,370,14},
    {187,396,4},
    {191,412,164},
    {355,578,190},
    {545,785,175},
    {720,964,12},
    {732,980,12},
    {744,996,12},
    {756,1012,12},
    {768,1028,124},
    {892,1166,178},
};
__attribute__((noinline)) unsigned poly_sound_offset(unsigned cell)
{
    unsigned lo=0,hi=sizeof cells/sizeof cells[0];
    if(cell>=POLY_BYTES)return 1344;
    while(lo+1<hi){unsigned mid=(lo+hi)/2;if(cells[mid].first<=cell)lo=mid;else hi=mid;}
    return cells[lo].offset+cell-cells[lo].first;
}
int poly_cell_owned(unsigned offset)
{
    unsigned i;for(i=0;i<sizeof cells/sizeof cells[0];i++)
        if(offset>=cells[i].offset && offset<cells[i].offset+cells[i].count)
            return cells[i].first+offset-cells[i].offset<POLY_BYTES;
    return 0;
}
static uint8_t get(const uint8_t *s,unsigned cell){return s[poly_sound_offset(cell)];}
static void put(uint8_t *s,unsigned cell,uint8_t v){s[poly_sound_offset(cell)]=v;}
static unsigned base(int pattern){return 8u+33u*(unsigned)pattern;}
/* 26 packed timing bytes: 13 existing non-bass header bytes +13 slab bytes. */
__attribute__((noinline)) static uint8_t *time_cell(pattern_t *p,uint8_t *s,int pattern,unsigned n)
{
    pattern_t *q=&p[pattern];
    if(n<8)return &((uint8_t *)&q->drum[n/4])[__builtin_offsetof(dpart_t,len)+n%4];
    if(n<10)return &q->brk.rsv[n-8];
    if(n<13)return &q->rsv[n-10];
    return &s[poly_sound_offset(base(pattern)+n-13)];
}
__attribute__((noinline)) static unsigned timing(const pattern_t *p,const uint8_t *s,int pattern,int row)
{
    unsigned bit=(unsigned)row*9,byte=bit/8,shift=bit%8;
    unsigned x=*time_cell((pattern_t *)p,(uint8_t *)s,pattern,byte);
    x|=(unsigned)*time_cell((pattern_t *)p,(uint8_t *)s,pattern,byte+1)<<8;
    return (x>>shift)&511u;
}
int poly_part(int lane){return lane<22?lane/11:lane-20;}
int poly_length(const pattern_t *p,const uint8_t *s,int pattern,int lane)
{return lane>=22 && lane<24?p[pattern].bass[lane-22].len:(int)(timing(p,s,pattern,lane==24?22:lane)&63u)+1;}
int poly_rate(const pattern_t *p,const uint8_t *s,int pattern,int lane)
{return lane>=22 && lane<24?p[pattern].bass[lane-22].rate:(int)(timing(p,s,pattern,lane==24?22:lane)>>6);}
void poly_set(pattern_t *p,uint8_t *s,int pattern,int lane,int length,int rate)
{
    unsigned row,bit,byte,shift,x,mask;uint8_t *a,*b;
    if(lane>=22 && lane<24){p[pattern].bass[lane-22].len=(uint8_t)length;p[pattern].bass[lane-22].rate=(uint8_t)rate;return;}
    row=lane==24?22:(unsigned)lane;bit=row*9;byte=bit/8;shift=bit%8;
    a=time_cell(p,s,pattern,byte);b=time_cell(p,s,pattern,byte+1);
    x=*a|((unsigned)*b<<8);mask=511u<<shift;
    x=(x&~mask)|(((unsigned)(length-1)|((unsigned)rate<<6))<<shift);
    *a=(uint8_t)x;*b=(uint8_t)(x>>8);
}
int poly_break_gate(const pattern_t *p,const uint8_t *s,int pattern,int step)
{
    if(step<32)return (int)((p[pattern].brk.steps>>(unsigned)step)&1u);
    return (get(s,base(pattern)+13u+(unsigned)(step-32)/8)>>(step%8))&1;
}
int poly_break_slice(const pattern_t *p,const uint8_t *s,int pattern,int step)
{
    unsigned n=(unsigned)step/2;uint8_t x=n<16?p[pattern].brk.slice[n]:get(s,base(pattern)+17+n-16);
    return (x>>((step&1)*4))&15;
}
void poly_break_set(pattern_t *p,uint8_t *s,int pattern,int step,int gate,int slice)
{
    uint8_t *a;unsigned n=(unsigned)step/2,sh=(step&1)*4;
    if(step<32){uint32_t mask=1u<<step;p[pattern].brk.steps=(p[pattern].brk.steps&~mask)|(gate?mask:0);}
    else {unsigned cell=base(pattern)+13u+(unsigned)(step-32)/8;uint8_t x=get(s,cell),mask=(uint8_t)(1u<<(step%8));put(s,cell,(uint8_t)((x&~mask)|(gate?mask:0)));}
    a=n<16?&p[pattern].brk.slice[n]:&s[poly_sound_offset(base(pattern)+17+n-16)];
    *a=(uint8_t)((*a&~(15u<<sh))|((unsigned)slice<<sh));
}
POLY_COLD void poly_enable(pattern_t *p,uint8_t *s)
{
    unsigned i;int pat,lane;
    for(i=0;i<POLY_BYTES;i++)put(s,i,0);
    put(s,0,'L');put(s,1,'A');put(s,2,'N');put(s,3,'1');
    put(s,4,129);put(s,5,33);put(s,6,32);put(s,7,25);
    for(pat=0;pat<NPAT;pat++){
        uint8_t len[2],rate[2],slices[16];unsigned j;
        for(j=0;j<2;j++){len[j]=p[pat].drum[j].len;rate[j]=p[pat].drum[j].rate;}
        for(j=0;j<16;j++)slices[j]=p[pat].brk.slice[j];
        for(j=0;j<26;j++)*time_cell(p,s,pat,j)=0;
        for(lane=0;lane<22;lane++)poly_set(p,s,pat,lane,len[lane/11]<1||len[lane/11]>64?16:len[lane/11],rate[lane/11]&3);
        poly_set(p,s,pat,24,16,RATE_16);
        for(j=0;j<16;j++)p[pat].brk.slice[j]=0;
        for(j=0;j<16;j++)poly_break_set(p,s,pat,(int)j,(p[pat].brk.steps>>j)&1,slices[j]<=8?slices[j]:0);
    }
}
int poly_mode(const uint8_t *s){return (get(s,4)&128u)!=0;}
void poly_mode_set(uint8_t *s,int enabled){put(s,4,(uint8_t)(1u|(enabled?128u:0)));}
POLY_COLD int poly_valid(const pattern_t *p,const uint8_t *s)
{
    int pat,lane,step;
    static const uint8_t hdr[8]={'L','A','N','1',1,33,32,25};
    for(step=0;step<8;step++)if((get(s,(unsigned)step)&(step==4?127u:255u))!=hdr[step])return 0;
    for(pat=0;pat<NPAT;pat++){
        if(*time_cell((pattern_t *)p,(uint8_t *)s,pat,25)&128u)return 0;
        for(lane=22;lane<24;lane++)if(poly_length(p,s,pat,lane)<1||poly_length(p,s,pat,lane)>64||poly_rate(p,s,pat,lane)>7)return 0;
        for(step=0;step<64;step++)if(poly_break_slice(p,s,pat,step)>8)return 0;
    }
    return 1;
}


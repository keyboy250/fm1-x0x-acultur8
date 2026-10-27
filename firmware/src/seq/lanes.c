/* SPDX-License-Identifier: GPL-3.0-only */
#include "lanes.h"
#include "rate_slice_codec.h"
#include "rate_families.h"
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
POLY_COLD int poly_expanded(const uint8_t *s){return get(s,3)=='2' || get(s,3)=='3';}
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
    unsigned bits=poly_expanded(s)?11u:9u,bit=(unsigned)row*bits,byte=bit/8,shift=bit%8;
    unsigned x=*time_cell((pattern_t *)p,(uint8_t *)s,pattern,byte);
    x|=(unsigned)*time_cell((pattern_t *)p,(uint8_t *)s,pattern,byte+1)<<8;
    if(bits==11 && shift>5)x|=(unsigned)*time_cell((pattern_t *)p,(uint8_t *)s,pattern,byte+2)<<16;
    return (x>>shift)&((1u<<bits)-1u);
}
int poly_part(int lane){return lane<22?lane/11:lane-20;}
POLY_COLD int poly_length(const pattern_t *p,const uint8_t *s,int pattern,int lane)
{return lane>=22 && lane<24?p[pattern].bass[lane-22].len:(int)(timing(p,s,pattern,lane==24?22:lane)&63u)+1;}
POLY_COLD int poly_rate(const pattern_t *p,const uint8_t *s,int pattern,int lane)
{return lane>=22 && lane<24?p[pattern].bass[lane-22].rate:(int)(timing(p,s,pattern,lane==24?22:lane)>>6);}
POLY_COLD void poly_set(pattern_t *p,uint8_t *s,int pattern,int lane,int length,int rate)
{
    unsigned row,bit,byte,shift,x,mask;uint8_t *a,*b;
    if(lane>=22 && lane<24){p[pattern].bass[lane-22].len=(uint8_t)length;p[pattern].bass[lane-22].rate=(uint8_t)rate;return;}
    row=lane==24?22:(unsigned)lane;bit=row*(poly_expanded(s)?11u:9u);byte=bit/8;shift=bit%8;
    a=time_cell(p,s,pattern,byte);b=time_cell(p,s,pattern,byte+1);
    x=*a|((unsigned)*b<<8);if(poly_expanded(s)&&shift>5)x|=(unsigned)*time_cell(p,s,pattern,byte+2)<<16;mask=(poly_expanded(s)?2047u:511u)<<shift;
    x=(x&~mask)|(((unsigned)(length-1)|((unsigned)rate<<6))<<shift);
    *a=(uint8_t)x;*b=(uint8_t)(x>>8);
    if(poly_expanded(s)&&shift>5)*time_cell(p,s,pattern,byte+2)=(uint8_t)(x>>16);
}
int poly_break_gate(const pattern_t *p,const uint8_t *s,int pattern,int step)
{
    if(step<32)return (int)((p[pattern].brk.steps>>(unsigned)step)&1u);
    return (get(s,base(pattern)+(poly_expanded(s)?19u:13u)+(unsigned)(step-32)/8)>>(step%8))&1;
}
POLY_COLD int poly_break_slice(const pattern_t *p,const uint8_t *s,int pattern,int step)
{
    if(poly_expanded(s)){uint8_t packed[26];unsigned i;for(i=0;i<26;i++)packed[i]=i<16?p[pattern].brk.slice[i]:get(s,base(pattern)+23u+i-16);return rf_slice_get(packed,(unsigned)step);}
    unsigned n=(unsigned)step/2;uint8_t x=n<16?p[pattern].brk.slice[n]:get(s,base(pattern)+17+n-16);
    return (x>>((step&1)*4))&15;
}
POLY_COLD void poly_break_set(pattern_t *p,uint8_t *s,int pattern,int step,int gate,int slice)
{
    if(poly_expanded(s)){uint8_t packed[26],slices[64];unsigned i;for(i=0;i<26;i++)packed[i]=i<16?p[pattern].brk.slice[i]:get(s,base(pattern)+23u+i-16);if(!rf_slice_unpack(slices,packed)||step<0||step>=64||slice<0||slice>8)return;slices[step]=(uint8_t)slice;rf_slice_pack(packed,slices);for(i=0;i<26;i++)if(i<16)p[pattern].brk.slice[i]=packed[i];else put(s,base(pattern)+23u+i-16,packed[i]);}
    uint8_t *a;unsigned n=(unsigned)step/2,sh=(step&1)*4;
    if(step<32){uint32_t mask=1u<<step;p[pattern].brk.steps=(p[pattern].brk.steps&~mask)|(gate?mask:0);}
    else {unsigned cell=base(pattern)+(poly_expanded(s)?19u:13u)+(unsigned)(step-32)/8;uint8_t x=get(s,cell),mask=(uint8_t)(1u<<(step%8));put(s,cell,(uint8_t)((x&~mask)|(gate?mask:0)));}
    if(poly_expanded(s))return;
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
    const uint8_t hdr[8]={'L','A','N',poly_sync_enabled(s)?'3':poly_expanded(s)?'2':'1',1,33,32,25};
    for(step=0;step<8;step++)if((get(s,(unsigned)step)&(step==4?127u:poly_sync_enabled(s)&&step>=5?(step==7?31u:63u):255u))!=hdr[step])return 0;
    for(pat=0;pat<NPAT;pat++){
        if(!poly_expanded(s)&&(*time_cell((pattern_t *)p,(uint8_t *)s,pat,25)&128u))return 0;
        for(lane=0;lane<25;lane++)if(poly_rate(p,s,pat,lane)>(poly_sync_enabled(s)?RF_COUNT-1:poly_expanded(s)?20:7))return 0;
        for(lane=22;lane<24;lane++)if(poly_length(p,s,pat,lane)<1||poly_length(p,s,pat,lane)>64)return 0;
        for(step=0;step<64;step++)if(poly_break_slice(p,s,pat,step)<0 || poly_break_slice(p,s,pat,step)>8)return 0;
    }
    if(poly_expanded(s))for(lane=0;lane<25;lane++)if(poly_rate_config(p,s,lane,2)>2)return 0;
    return 1;
}


/* Config uses six formerly unused owned cells and the three spare high bits of
 * each LAN2 timing record. It never overlaps rate/length, slices or sound cells. */
static uint8_t *config_cell(pattern_t *p,uint8_t *s,unsigned bit,unsigned *shift)
{
 if(bit>=144){unsigned b=bit-144;*shift=b<4?6+b%2:5+b-4;return &s[poly_sound_offset(b<4?5+b/2:7)];}
 if(bit<48){*shift=bit%8;return &s[poly_sound_offset(1064+bit/8)];}
 bit-=48;*shift=5+bit%3;return time_cell(p,s,(int)(bit/3),31);
}
POLY_COLD int poly_rate_config(const pattern_t *p,const uint8_t *s,int lane,int field)
{
 unsigned bit=(unsigned)lane*5+(field==0?0:field==1?2:3),width=field==1?1:2,i,value=0;
 if(!poly_expanded(s))return 0;
 for(i=0;i<width;i++){unsigned shift;uint8_t *a=config_cell((pattern_t *)p,(uint8_t *)s,bit+i,&shift);value|=((unsigned)*a>>shift&1)<<i;}
 return (int)value;
}
POLY_COLD void poly_rate_config_set(pattern_t *p,uint8_t *s,int lane,int field,int value)
{
 unsigned bit=(unsigned)lane*5+(field==0?0:field==1?2:3),width=field==1?1:2,i;
 for(i=0;i<width;i++){unsigned shift;uint8_t *a=config_cell(p,s,bit+i,&shift);*a=(uint8_t)((*a&~(1u<<shift))|(((unsigned)value>>i&1)<<shift));}
}
POLY_COLD int poly_expand(pattern_t *p,uint8_t *s)
{
 int pat,lane;unsigned i;
 if(poly_expanded(s))return poly_valid(p,s);
 if(!poly_valid(p,s))return 0;
 for(pat=0;pat<NPAT;pat++){
  uint8_t length[25],rate[25],slices[64],packed[26],gates[4];
  for(lane=0;lane<25;lane++){length[lane]=(uint8_t)poly_length(p,s,pat,lane);rate[lane]=(uint8_t)poly_rate(p,s,pat,lane);}
  for(i=0;i<64;i++)slices[i]=(uint8_t)poly_break_slice(p,s,pat,(int)i);
  for(i=0;i<4;i++)gates[i]=get(s,base(pat)+13u+i);
  rf_slice_pack(packed,slices);put(s,3,'2');
  for(i=0;i<32;i++)*time_cell(p,s,pat,i)=0;
  for(lane=0;lane<25;lane++)poly_set(p,s,pat,lane,length[lane],rate[lane]);
  for(i=0;i<4;i++)put(s,base(pat)+19u+i,gates[i]);
  for(i=0;i<26;i++)if(i<16)p[pat].brk.slice[i]=packed[i];else put(s,base(pat)+23u+i-16,packed[i]);
  put(s,3,'1');
 }
 put(s,3,'2');for(i=1064;i<1070;i++)put(s,i,0);
 return poly_valid(p,s);
}

/* 125 policy bits are LAN2-owned. LAN3 adds 1 global +25 follow bits:
 * bits125..143 use the remaining record bits;144..150 use header high bits.
 * Header low bits, rates, slices, lengths and sound controls are untouched. */
int poly_sync_enabled(const uint8_t *s){return get(s,3)=='3';}
static unsigned sync_bit(const pattern_t *p,const uint8_t *s,unsigned bit){unsigned shift;uint8_t *a=config_cell((pattern_t *)p,(uint8_t *)s,bit,&shift);return (*a>>shift)&1u;}
static void sync_put(pattern_t *p,uint8_t *s,unsigned bit,unsigned v){unsigned shift;uint8_t *a=config_cell(p,s,bit,&shift);*a=(uint8_t)((*a&~(1u<<shift))|(v<<shift));}
int poly_sync_upgrade(pattern_t *p,uint8_t *s){unsigned i;if(poly_sync_enabled(s))return poly_valid(p,s);if(!poly_expand(p,s))return 0;for(i=125;i<=150;i++)sync_put(p,s,i,i!=125);put(s,3,'3');return poly_valid(p,s);}
int poly_sync_mode(const pattern_t *p,const uint8_t *s){return poly_sync_enabled(s)?(int)sync_bit(p,s,125):0;}
int poly_sync_override(const pattern_t *p,const uint8_t *s,int lane){unsigned f;if(lane<0||lane>=25||!poly_sync_enabled(s)||sync_bit(p,s,126u+(unsigned)lane))return 0;f=(unsigned)poly_rate_config(p,s,lane,0);return f==0?1:f==2?2:3;}
void poly_sync_set(pattern_t *p,uint8_t *s,int lane,int value){if(lane<0){sync_put(p,s,125,(unsigned)value);return;}sync_put(p,s,126u+(unsigned)lane,value==0);if(value)poly_rate_config_set(p,s,lane,0,value==1?0:value==2?2:3);}
unsigned poly_sync_family(const pattern_t *p,const uint8_t *s,int lane){int local=poly_sync_override(p,s,lane);if(!poly_sync_enabled(s))return (unsigned)poly_rate_config(p,s,lane,0);return local==1 || (!local&&poly_sync_mode(p,s))?RF_GRID:local==2?RF_STRAIGHT_TRIPLET:RF_FULL;}

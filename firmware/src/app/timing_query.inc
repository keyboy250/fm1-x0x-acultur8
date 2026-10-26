/* SPDX-License-Identifier: GPL-3.0-only */
static uint32_t ed_tc_crc(const uint8_t *p,uint32_t n) {
 uint32_t c=~0u;while(n--){c^=*p++;for(unsigned i=0;i<8;i++)c=(c>>1)^(0xEDB88320u&-(c&1u));}return ~c;
}
static int ed_tc_word(const uint8_t *p,uint32_t *v) {
 if(p[4]>15)return 0;
 *v=0;for(unsigned i=0;i<5;i++)*v|=(uint32_t)p[i]<<(i*7);return 1;
}
static void ed_timing_query(const uint8_t *a,uint32_t n) {
 uint32_t w[10],args[5],crc=0,status=0;uint8_t wire[32];
 unsigned op=n>2?a[2]:127;
 for(unsigned i=0;i<n;i++)if(a[i]>127)status=1;
 if(n<3||a[0]!=1)status=n&&a[0]!=1?7:1;
 else if(op>2||n!=(op?33u:3u))status=1;
 if(!status&&op){
  for(unsigned i=0;i<5;i++)if(!ed_tc_word(a+3+i*5,args+i))status=1;
  wire[0]=0x7d;wire[1]=0x46;wire[2]=0x4c;wire[3]=52;
  for(unsigned i=0;i<28;i++)wire[4+i]=a[i];
  if(!ed_tc_word(a+28,&crc)||ed_tc_crc(wire,32)!=crc)status=1;
  if(!status&&((op==1&&!args[0])||(op==2&&args[0])))status=1;
  if(!status&&!ED_TIMING_TX_IDLE())status=2;
  if(!status)status=(uint32_t)engine_timing_update(args[0],args[1],args[2],args[3],args[4]);
 }
 engine_timing_status(w);
 ed_b(1);ed_b(n>1?a[1]:0);ed_b(op);ed_b(status);ed_b(10);
 for(unsigned j=0;j<10;j++)for(unsigned k=0;k<5;k++)ed_b(w[j]>>(k*7));
 crc=ed_tc_crc(ed_out+1,ed_n-1);
 for(unsigned k=0;k<5;k++)ed_b(crc>>(k*7));
}

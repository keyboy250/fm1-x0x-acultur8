/* SPDX-License-Identifier: GPL-3.0-only */
#define PATTERN_COPY_NO_MAIN
#include "pattern_copy_test.c"
static unsigned writes,wire_n,incoming_n,race;
static uint8_t wire[288],incoming[80];
static int flash_ok=1;
static void fm1_irq_off(void) {} static void fm1_irq_on(void) {}
static void fm1_wdt_feed(void) {} static void audio_silence(void) {}
static void fl_inval(uint32_t a,uint32_t n){(void)a;(void)n;writes++;}
static int fl_erase4k(uint32_t a,uint32_t *t){(void)a;*t=0;writes++;return 0;}
static int fl_write(uint32_t a,const void *p,uint32_t n){(void)a;(void)p;(void)n;writes++;return 0;}
static void ota_wire_send(const uint8_t *p,uint32_t n){CHECK(n<=sizeof wire);memcpy(wire,p,n);wire_n=n;}
static int ota_frame_get(const uint8_t **p,uint32_t *n){if(!incoming_n)return 0;*p=incoming;*n=incoming_n;return 1;}
static void ota_frame_done(void){incoming_n=0;}
static void interleave(void){
    if(race==1)variant_changed();
    if(race==2)seq.transport_epoch++;
    if(race==3)proj.pat[1].bass[0].step[63].note^=1;
    if(race==4)seq.req_start=1;
}
#define PATTERN_QUERY_TEST_HOOK() interleave()
#include "../../firmware/src/app/editor.c"
static unsigned raw_n;static uint8_t raw[160];
static uint32_t get7(unsigned offset,unsigned n){uint32_t x=0;for(unsigned i=0;i<n;i++)x|=(uint32_t)wire[offset+i]<<(7*i);return x;}
static void send(int pat,int section,int index,int version,int length){
    uint8_t frame[]={0x7D,0x46,0x4C,49,(uint8_t)version,77,(uint8_t)pat,(uint8_t)section,(uint8_t)(index&127),(uint8_t)(index>>7),0};
    incoming_n=(unsigned)length;memcpy(incoming,frame,incoming_n);wire_n=0;ed_service();
    CHECK(!incoming_n&&wire_n>=18&&wire_n<=sizeof wire&&wire[0]==0xF0&&wire[wire_n-1]==0xF7);
    for(unsigned i=1;i<wire_n-1;i++)CHECK(wire[i]<128);
    CHECK(get7(wire_n-6,5)==st_crc32(wire+1,wire_n-7));
    raw_n=0;
    if(wire[11]==0){
        unsigned n=get7(32,2),pos=34;CHECK(n<=sizeof raw);
        while(raw_n<n){unsigned mask=wire[pos++];for(unsigned j=0;j<7&&raw_n<n;j++)raw[raw_n++]=(uint8_t)(wire[pos++]|((mask>>j&1u)<<7));}
        CHECK(pos==wire_n-6);
    }else CHECK(wire_n==18);
}
static void read_unchanged(int pat,int section,int index,int status){
    project_t p=proj;seq_t sq=seq;undo_t u=undo;
    uint8_t shadow[sizeof undo_shadow],panel[sizeof ui];
    memcpy(shadow,undo_shadow,sizeof shadow);memcpy(panel,&ui,sizeof panel);
    unsigned stored=store_writes,rev=variant_current_revision(),w=engine_queue_written(),r=engine_queue_read();
    uint32_t oldknown[sizeof stored_hash/sizeof stored_hash[0]];memcpy(oldknown,stored_hash,sizeof oldknown);
    send(pat,section,index,1,10);if(wire[11]!=status)fprintf(stderr,"query P%d section%d index%d got status%u expected%d queue%u/%u rec%d keys%d btn%d\n",pat+1,section,index,wire[11],status,engine_queue_read(),engine_queue_written(),ui.rec,ui.keys,ui.btn);CHECK(wire[11]==status);
    CHECK(!memcmp(&p,&proj,sizeof p)&&!memcmp(&sq,&seq,sizeof sq));
    CHECK(!memcmp(&u,&undo,sizeof u)&&!memcmp(shadow,undo_shadow,sizeof shadow));
    CHECK(!memcmp(panel,&ui,sizeof panel));
    CHECK(rev==variant_current_revision()&&stored==store_writes&&w==engine_queue_written()&&r==engine_queue_read()&&!writes);
    CHECK(!memcmp(oldknown,stored_hash,sizeof oldknown));
}
static void validate(int pat,int section,int index){
    pattern_t *p=&proj.pat[pat];int poly=(proj.variant.arrangement.flags&POLY_FLAG)!=0;
    CHECK(get7(12,5)==variant_current_revision()&&get7(17,5)==engine_queue_written());
    CHECK(get7(22,5)==seq.transport_epoch&&get7(27,5)==project_fingerprint());
    if(section==0){
        int count=0;for(int k=0;k<NLANE;k++)if(proj.arr.lane[k].used&&proj.arr.lane[k].pat==pat)count++;
        CHECK(raw_n==4&&raw[0]==(poly?(poly_expanded((uint8_t *)&proj.sound)?2:1):0));
        CHECK(raw[1]==p->swing&&raw[2]==ui.dirty&&raw[3]==count);return;
    }
    if(section==2){
        int ordinal=0,found=-1;
        for(int k=0;k<NLANE;k++)if(proj.arr.lane[k].used&&proj.arr.lane[k].pat==pat){if(ordinal++==index){found=k;break;}}
        CHECK(found>=0);lane_t *l=&proj.arr.lane[found];
        CHECK(raw_n==68&&raw[0]==l->part&&raw[1]==l->t&&raw[2]==l->v&&raw[3]==l->i);
        CHECK(!memcmp(raw+4,l->val,64));return;
    }
    if(index<22){
        dpart_t *d=&p->drum[index/11];
        CHECK(raw_n==18);
        CHECK(raw[0]==(poly?poly_length(proj.pat,(uint8_t *)&proj.sound,pat,index):d->len));
        CHECK(raw[1]==(poly?poly_rate(proj.pat,(uint8_t *)&proj.sound,pat,index):d->rate));
        for(int st=0;st<64;st++){
            CHECK(((raw[2+st/8]>>(st%8))&1)==sm_get(d->hit[index%11],st));
            CHECK(((raw[10+st/8]>>(st%8))&1)==sm_get(d->accent,st));
        }
    }else if(index<24){
        bpart_t *b=&p->bass[index-22];CHECK(raw_n==144);
        CHECK(raw[0]==b->len&&raw[1]==b->rate&&raw[2]==b->dir&&raw[3]==b->transpose);
        CHECK(!memcmp(raw+4,&b->gen,8));
        CHECK(((uint32_t)raw[12]|(uint32_t)raw[13]<<8|(uint32_t)raw[14]<<16|(uint32_t)raw[15]<<24)==b->gen.seed);
        for(int st=0;st<64;st++){CHECK(raw[16+2*st]==b->step[st].note);CHECK(raw[17+2*st]==b->step[st].flags);}
    }else{
        CHECK(raw_n==4+BRK_NSET+128&&raw[0]==(poly?poly_length(proj.pat,(uint8_t *)&proj.sound,pat,24):16));
        CHECK(raw[1]==(poly?poly_rate(proj.pat,(uint8_t *)&proj.sound,pat,24):RATE_16));
        CHECK(raw[2]==p->brk.slot_a&&raw[3]==p->brk.slot_b&&!memcmp(raw+4,p->brk.set,BRK_NSET));
        for(int st=0;st<64;st++){
            CHECK(raw[4+BRK_NSET+2*st]==(poly?poly_break_gate(proj.pat,(uint8_t *)&proj.sound,pat,st):st<32?(p->brk.steps>>st)&1u:0));
            CHECK(raw[5+BRK_NSET+2*st]==(poly?poly_break_slice(proj.pat,(uint8_t *)&proj.sound,pat,st):st<16?p->brk.slice[st]:0));
        }
    }
}
static void record(FILE *f){uint8_t n[2]={(uint8_t)wire_n,(uint8_t)(wire_n>>8)};CHECK(fwrite(n,1,2,f)==2&&fwrite(wire,1,wire_n,f)==wire_n);}
static void pattern(FILE *f,int pat){
    read_unchanged(pat,0,0,0);validate(pat,0,0);int count=raw[3];if(f)record(f);
    for(int k=0;k<25;k++){read_unchanged(pat,1,k,0);validate(pat,1,k);if(f)record(f);}
    for(int k=0;k<count;k++){read_unchanged(pat,2,k,0);validate(pat,2,k);if(f)record(f);}
    read_unchanged(pat,0,0,0);if(f)record(f);
}
static void snapshot(const char *name){FILE *f=fopen(name,"wb");CHECK(f!=0);pattern(f,1);pattern(f,2);fclose(f);}
#ifndef PATTERN_QUERY_MAIN
#define PATTERN_QUERY_MAIN main
#endif
int PATTERN_QUERY_MAIN(void){
    char path[120];
    for(int mode=0;mode<3;mode++){
        mode_setup(mode);proj.set.autosave_off=1;ui.dirty=1;
        for(int pat=0;pat<32;pat++)pattern(0,pat);
        snprintf(path,sizeof path,"build/query-review/mode-%d.bin",mode);snapshot(path);
    }
    for(int scope=0;scope<25;scope++){
        mode_setup(2);proj.set.autosave_off=1;ui.dirty=1;settle();ui.dirty=1;
        snprintf(path,sizeof path,"build/query-review/scope-%d-before.bin",scope);snapshot(path);
        CHECK(ui_pattern_copy(1,2,scope)==VR_OK);render();
        snprintf(path,sizeof path,"build/query-review/scope-%d-after.bin",scope);snapshot(path);
        CHECK(ui_external_undo(0)==VR_OK);render();
        snprintf(path,sizeof path,"build/query-review/scope-%d-undo.bin",scope);snapshot(path);
    }
    /* Full pool, high-bit values and ordinal159 exercise the two-byte index. */
    mode_setup(2);
    for(int k=0;k<NLANE;k++){lane_t *l=&proj.arr.lane[k];memset(l,0,sizeof *l);l->used=1;l->pat=1;l->part=2;l->t=T_PROB;l->v=32;l->i=(uint8_t)k;memset(l->val,255,64);l->val[63]=135;}
    snapshot("build/query-review/full-motion.bin");
    mode_setup(2);
    for(int flag=0;flag<13;flag++){
        if(flag==0)seq.playing=1;
        if(flag==1)seq.req_start=1;
        if(flag==2)seq.req_stop=1;
        if(flag==3)seq.req_cont=1;
        if(flag==4)seq.transport_stage=1;
        if(flag==5)seq.transport_hold=1;
        if(flag==6)ui.rec=1;
        if(flag==7)ui.keys=1;
        if(flag==8)ui.btn=1;
        if(flag==9)external_edit=1;
        if(flag==10)save_wait=1;
        if(flag==11)save_hold=1;
        if(flag==12){perf.test=0;perf.phase=1;}
        read_unchanged(1,1,22,2);mode_setup(2);
    }
    variant_command_t cmd={1,VC_SET_STRIP,1,0,0,VP_BOOST,0,5};variant_reply_t reply;
    CHECK(variant_command(&cmd,&reply)==VR_QUEUED);read_unchanged(1,0,0,2);render();
    for(int n=4;n<=11;n++)if(n!=10){send(1,0,0,1,n);CHECK(wire[11]==1);}
    send(1,0,0,2,10);CHECK(wire[11]==3);
    read_unchanged(32,0,0,1);read_unchanged(1,3,0,1);read_unchanged(1,1,25,1);read_unchanged(1,2,160,1);
    for(race=1;race<=4;race++){send(1,1,22,1,10);CHECK(wire[11]==2&&wire_n==18);mode_setup(2);}race=0;
    CHECK(!writes);
    printf("PATTERN QUERY PASS: %d checkpoints; all32 patterns/25 lanes/3 formats, all25 copy+Undo snapshots, motion159, wire CRC, unsaved RAM, unchanged UI/history/queue/transport/revision/save/storage, malformed/busy/raced reads.\n",checks);
    return 0;
}

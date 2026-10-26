/* SPDX-License-Identifier: GPL-3.0-only */
#define PATTERN_QUERY_MAIN inherited_query_main
#include "pattern_query_test.c"
#undef PATTERN_QUERY_MAIN
static uint32_t diag[RUNTIME_DIAGNOSTIC_WORDS];
static void query(unsigned lane,int version,unsigned length,int status)
{
    project_t old=proj;seq_t oldseq=seq;undo_t oldu=undo;pattern_freeze_t oldf=*seq.freeze;
    uint8_t oldui[sizeof ui],shadow[sizeof undo_shadow];memcpy(oldui,&ui,sizeof ui);memcpy(shadow,undo_shadow,sizeof shadow);
    unsigned sw=store_writes,qw=engine_queue_written(),qr=engine_queue_read(),rev=variant_current_revision();
    uint8_t frame[]={0x7d,0x46,0x4c,50,(uint8_t)version,77,(uint8_t)lane,0};
    memcpy(incoming,frame,length);incoming_n=length;ed_service();CHECK(!incoming_n);
    CHECK(wire_n==(status?15:276));if(wire[7]!=(length>=7?lane:127u) || wire[8]!=status){fprintf(stderr,"diag lane%u version%d len%u status%u expected%d bytes:",lane,version,length,wire[8],status);for(unsigned z=0;z<14;z++)fprintf(stderr," %u",wire[z]);fprintf(stderr,"\n");}CHECK(wire[7]==(length>=7?lane:127u) && wire[8]==status);
    CHECK(get7(wire_n-6,5)==st_crc32(wire+1,wire_n-7));
    if(!status){CHECK(wire[9]==RUNTIME_DIAGNOSTIC_WORDS);for(unsigned i=0;i<RUNTIME_DIAGNOSTIC_WORDS;i++)diag[i]=get7(10+5*i,5);}
    CHECK(!memcmp(&old,&proj,sizeof proj)&&!memcmp(&oldseq,&seq,sizeof seq)&&!memcmp(&oldf,seq.freeze,sizeof oldf));
    CHECK(!memcmp(&oldu,&undo,sizeof undo)&&!memcmp(oldui,&ui,sizeof ui)&&!memcmp(shadow,undo_shadow,sizeof shadow));
    CHECK(sw==store_writes && qw==engine_queue_written() && qr==engine_queue_read() && rev==variant_current_revision() && !writes);
}
int main(void)
{
    for(int format=0;format<3;format++){
        mode_setup(format);for(int p=0;p<NPAT;p++)for(int b=0;b<2;b++)proj.pat[p].bass[b].gen.mutate_bars=0;
        for(int lane=0;lane<24;lane++){
            query(lane,1,7,0);CHECK(diag[10]==(unsigned)lane && diag[11]==3);
            CHECK(diag[41]==0 && diag[40]==0);
        }
        seq.variation=(pattern_variation_t){1,3,0,8,0,0,0,0};seq.ppat[0]=3;
        proj.pat[3].drum[0].len=16;proj.pat[3].drum[0].hit[1][0]=(1u<<1)|(1u<<2)|(1u<<8)|(1u<<15);proj.pat[3].drum[0].hit[1][1]=0;
        if(format)poly_set(proj.pat,(uint8_t *)&proj.sound,3,1,16,0);
        seq.freeze->state=FREEZE_RECORD;seq.freeze->bass.len=16;seq.freeze->count=4;
        for(int i=0;i<64;i++)seq.freeze->bass.step[i].flags=G_NOTE; /* stale tail must not escape */
        query(1,1,7,0);CHECK(diag[41]==1 && diag[44]==15 && diag[45]==0);
        seq.freeze->count=65;query(1,1,7,0);CHECK(!diag[40]&&!diag[44]&&!diag[45]);seq.freeze->count=4;
        seq.ppat[0]=4;query(1,1,7,0);CHECK(!diag[41]&&!diag[40]);seq.ppat[0]=3;
        seq.freeze->count=0;seq.freeze->state=FREEZE_RECORD;seq.playing=1;
        query(1,1,7,0);CHECK(diag[4]==1);seq.playing=0;
        query(24,1,7,1);query(1,2,7,3);query(1,1,6,1);query(1,1,8,1);
        seq.req_start=1;query(1,1,7,0);CHECK(diag[5]==1);seq.req_start=0;
        /* Actual engine-driven capture: query every rendered quantum, never advance during query. */
        seq_start(&seq);unsigned seen=0;
        for(int i=0;i<1800;i++){int32_t audio[512];engine_render(audio,256);query(1,1,7,0);
            if(diag[40] && diag[38]==16){CHECK((diag[44]&~65535u)==0);CHECK(diag[44]==((diag[32]&1u)?0x80f8u:0x8106u));seen++;}
        }
        CHECK(seen>0);seq_stop(&seq);{int32_t audio[512];engine_render(audio,256);}
    }
    CHECK(engine_runtime_snapshot(24,diag)==-1); /* explicit invalid request */
    FILE *f=fopen("build/runtime-diagnostics/native-frame.bin","wb");CHECK(f);query(1,1,7,0);fwrite(wire,1,wire_n,f);fclose(f);
    printf("RUNTIME DIAGNOSTIC PASS: %d checkpoints; all24 lanes/3formats, actual engine playback queries, immutable project/seq/capture/UI/Undo/queue/storage, prefix staleness, mismatch and malformed frames.\n",checks);
    return 0;
}

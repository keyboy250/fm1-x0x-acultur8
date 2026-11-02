/* SPDX-License-Identifier: GPL-3.0-only */
#define BLOCK_BRIDGE_NO_MAIN
#include "block_bridge_test.c"
static int cmd(int op,int target,int voice,int index,int value)
{variant_command_t c={VARIANT_ABI,(uint16_t)op,0,(uint8_t)target,(uint8_t)voice,(uint8_t)index,0,value};variant_reply_t r;return variant_command(&c,&r);}
static const unsigned masks[10]={0xab5,0x6ad,0x5ab,0xad5,0x6b5,0x5ad,0x56b,0x295,0x4a9,0xfff};
static int legal(int key,int mode,int n){return !!(masks[mode]&(1u<<((n+12-key)%12)));}
static int oracle(int key,int mode,int desired,int low,int high)
{int best=-1,d=1000;for(int n=low;n<=high;n++){int x=abs(n-desired);if(legal(key,mode,n)&&x<d){best=n;d=x;}}return best;}
static void fixture_store(const char *path)
{
    FILE *f=fopen(path,"wb");CHECK(f!=0);fputs("{",f);int first=1;
    for(int i=0;i<OBJ_NOBJ;i++)if(store_len[i]>=0){fprintf(f,"%s\"%d\":[",first?"":",",i);first=0;for(int j=0;j<store_len[i];j++)fprintf(f,"%s%u",j?",":"",store[i][j]);fputc(']',f);}
    fputs("}\n",f);CHECK(fclose(f)==0);
}
static void scale_oracle(void)
{
    const int bounds[][2]={{0,127},{24,72},{61,61},{0,0},{127,127},{60,64}};
    for(int key=0;key<12;key++)for(int mode=0;mode<10;mode++)for(int source=0;source<128;source++)for(int t=-24;t<=24;t+=3){
        uint8_t out=199;int want=oracle(key,mode,source+t,0,127);
        CHECK(harmony_scale_note(key,mode,source,t,0,127,&out)&&out==want);
    }
    for(int k=0;k<12;k++)for(int m=0;m<10;m++)for(unsigned b=0;b<sizeof bounds/sizeof bounds[0];b++)for(int t=-127;t<=127;t++){
        int low=bounds[b][0],high=bounds[b][1],want=oracle(k,m,60+t,low,high);uint8_t out=199;
        CHECK(harmony_scale_note(k,m,60,t,low,high,&out)==(want>=0));CHECK(out==(want<0?199:want));
    }
    uint8_t out=199;CHECK(!harmony_scale_note(12,0,60,0,0,127,&out)&&out==199);
    CHECK(!harmony_scale_note(0,10,60,0,0,127,&out)&&out==199);
    CHECK(!harmony_scale_note(0,0,128,0,0,127,&out)&&out==199);
    CHECK(!harmony_scale_note(0,0,60,128,0,127,&out)&&out==199);
    CHECK(!harmony_scale_note(0,0,60,0,73,24,&out)&&out==199);
}
static void durations(void)
{
    for(int bars=1;bars<=32;bars++)for(int degree=0;degree<8;degree++){
        fresh();song_setup();proj.arr.song.len=192;ui.song_sel=(uint8_t)(degree==0?0:degree==7?191:95);settle();before=proj;
        pref_set(PR(R_SREP,0,0,0),bars-1);pref_set(PR(R_BDEGREE,0,0,0),degree);render();
        variant_snapshot_t v;CHECK(variant_snapshot(VS_BLOCK,ui.song_sel,0,&v)==VR_OK);
        CHECK(v.value[6]==(unsigned)bars&&v.value[7]==(unsigned)degree);
        CHECK(pref_get(PR(R_SREP,0,0,0))==bars-1);CHECK(pref_get(PR(R_BDEGREE,0,0,0))==degree);
        CHECK(!memcmp(proj.pat,before.pat,sizeof proj.pat));CHECK(!memcmp(&proj.sound,&before.sound,sizeof proj.sound));
        CHECK(!memcmp(proj.arr.lane,before.arr.lane,sizeof proj.arr.lane));CHECK(variant_valid(&proj.variant));
        CHECK(project_save()==0);after=proj;CHECK(project_load()==0);render();CHECK(!memcmp(&proj,&after,sizeof proj));
    }
    fresh();song_setup();ui.song_sel=0;settle();before=proj;pref_set(PR(R_SREP,0,0,0),31);render();undo_commit(&undo);
    CHECK(ui_external_undo(0)==VR_OK);render();
    CHECK(!memcmp(&before,&proj,sizeof proj));
    CHECK(ui_external_undo(1)==VR_OK);render();CHECK(ARR_BARS(&song()->bar[0],proj.variant.arrangement.degree[0])==32);
    int bank=ui_duplicate_block(0);CHECK(bank>=0);render();CHECK(ARR_BARS(&song()->bar[1],proj.variant.arrangement.degree[1])==32);
    CHECK(ui_delete_block(0)==VR_OK);render();CHECK(ui_external_undo(0)==VR_OK);render();CHECK(ARR_BARS(&song()->bar[0],proj.variant.arrangement.degree[0])==32);
    before=proj;CHECK(cmd(VC_SET_BLOCK,0,255,32,0)==VR_INVALID);CHECK(!memcmp(&before,&proj,sizeof proj));
    /* A packed extension without its fail-closed marker, or unknown bits, is invalid. */
    variant_settings_t v=proj.variant;v.arrangement.flags&=~ARR_EXTENDED;CHECK(!variant_valid(&v));
    v=proj.variant;v.arrangement.degree[0]|=32;CHECK(!variant_valid(&v));
    for(int schema=2;schema<=4;schema++){v=proj.variant;v.schema=(uint16_t)schema;CHECK(!variant_saved_valid(&v));}
}
static void long_playback(void)
{
    for(int external=0;external<2;external++){
        fresh();song_setup();proj.arr.song.len=2;ui.song_sel=0;pref_set(PR(R_SREP,0,0,0),31);render();
        CHECK(cmd(VC_SET_LANE_MODE,0,0,0,1)==VR_QUEUED);render();
        CHECK(cmd(VC_SET_LANE,1,1,0,5)==VR_QUEUED);render();
        seq_init(&play,proj.pat);play.song=&proj.arr.song;play.bpm=120;play.send_clock=0;
        play.lane_sound=(uint8_t *)&proj.sound;play.lane_enabled=1;seq_arrangement_apply(&play,&proj.variant.arrangement);seq_start(&play);
        if(external)seq_ext_clock(&play);
        play_advance(1);CHECK(play.song_pos==0&&play.song_rep==32);
        for(int bar=1;bar<=32;bar++){
            if(external)for(int tick=0;tick<96;tick++){seq_ext_clock(&play);play_advance(919);}
            else play_advance(88200);
            CHECK(play.song_pos==(bar==32?1:0));CHECK(play.arrangement.degree==0);
            if(bar<32)CHECK(play.song_rep==32-bar);
        }
    }
    /* A long block's packed duration bits are not a primed clock in plain lane mode. */
    for(int bits=2;bits<=6;bits+=2){
        arrangement_settings_t idle=proj.variant.arrangement;idle.flags&=~ARR_ENABLED;
        seq_init(&play,proj.pat);play.song=&proj.arr.song;play.bpm=120;play.lane_sound=(uint8_t *)&proj.sound;play.lane_enabled=1;
        seq_arrangement_apply(&play,&idle);play.playing=1;play.arrangement.clock_started=(uint8_t)bits;play.clk_to_next=0;
        seq_advance_poly(&play,0,&silent_sink);CHECK(play.arrangement.clock_started&1u);CHECK(play.arrangement.tick==0);
    }
    seq.bpm=120;proj.set.bpm_x10=1200;
    CHECK(project_save()==0);fixture_store("build/long-scale-review/long-store.json");
}
static void adherence(void)
{
    fresh();song_setup();bpart_t *bp=&proj.pat[seq.ppat[TRK_BASS0]].bass[0];
    bp->len=1;bp->step[0]=(bstep_t){60,G_NOTE};settle();before=proj;
    pref_set(PR(R_AKEY,0,0,0),0);pref_set(PR(R_ASCALE,0,0,0),HARMONY_IONIAN);pref_set(PR(R_AFOLLOW,0,0,0),BASS_FOLLOW_SCALE);render();
    CHECK(arrangement_bass_mode(proj.variant.arrangement.flags,0)==BASS_FOLLOW_SCALE);
    cc_sources[0].ready=1;cc_sources[0].applied.bass=0;
    for(unsigned raw=0;raw<128;raw++){
        unsigned number=1061,cc=number%88;cc=cc<32?cc:cc+32;
        CHECK(cc_message(11u|((0xb0u+number/88)<<8)|(cc<<16)|(raw<<24),0)==CC_APPLIED);render();
        CHECK(arrangement_bass_mode(proj.variant.arrangement.flags,0)==(raw*2+63)/127);
    }
    CHECK(cc_parameter(1061,3,0)==CC_APPLIED);render();CHECK(arrangement_bass_mode(proj.variant.arrangement.flags,0)==BASS_FOLLOW_SCALE);
    for(int key=0;key<12;key++)for(int mode=0;mode<10;mode++){
        CHECK(cmd(VC_SET_ARR_GLOBAL,0,0,1,key)==VR_QUEUED);render();CHECK(cmd(VC_SET_ARR_GLOBAL,0,0,2,mode)==VR_QUEUED);render();
        for(int t=-24;t<=24;t++){
            uint8_t note;CHECK(seq_bass_resolve(&seq,0,60,t,&note));CHECK(note==oracle(key,mode,60+t,24,72));
            CHECK(legal(key,mode,note));
        }
    }
    CHECK(!memcmp(proj.pat,before.pat,sizeof proj.pat));CHECK(!memcmp(&proj.sound,&before.sound,sizeof proj.sound));
    CHECK(cmd(VC_SET_ARR_GLOBAL,0,0,2,0)==VR_QUEUED);CHECK(cmd(VC_SET_ARR_GLOBAL,0,0,1,0)==VR_QUEUED);render();
    /* Scale works with degree NONE and also when arrangement playback is OFF. */
    CHECK(cmd(VC_SET_ARR_GLOBAL,0,0,0,0)==VR_QUEUED);render();uint8_t note;
    CHECK(seq_bass_resolve(&seq,0,60,1,&note)&&note==60);CHECK(seq_bass_resolve(&seq,0,60,3,&note)&&note==62);
    engine_bass_on(0,63,0,0);render();CHECK(b303[0].gate&&fabsf(b303[0].osc_freq-b303[0].tuning*fm_exp2f(-7.0f/12))<0.001f);
    engine_bass_on(0,-20,0,0);render();CHECK(fabsf(b303[0].osc_freq-b303[0].tuning*fm_exp2f(-45.0f/12))<0.001f);
    seq.ch[TRK_BASS0]=0;midi_in(9u|(0x90u<<8)|(63u<<16)|(100u<<24));render();
    CHECK(bass_held[0]==63&&b303[0].gate&&fabsf(b303[0].osc_freq-b303[0].tuning*fm_exp2f(-7.0f/12))<0.001f);
    midi_in(8u|(0x80u<<8)|(63u<<16));render();CHECK(!b303[0].gate&&bass_held[0]==255);
    bp=&proj.pat[seq.ppat[TRK_BASS0]].bass[0];bp->transpose=27;
    seq_start(&seq);render();CHECK(seq.playing&&seq.t[TRK_BASS0].sounding);CHECK((seq.t[TRK_BASS0].held_note&127u)==62);
    seq_stop(&seq);render();CHECK(cmd(VC_SET_BASS_FOLLOW,0,0,0,0)==VR_QUEUED);render();
    CHECK(seq_bass_resolve(&seq,0,60,3,&note)&&note==63); /* Off retains chromatic behavior. */
    CHECK(cmd(VC_SET_BASS_FOLLOW,0,0,0,3)==VR_QUEUED);render();CHECK(project_save()==0);after=proj;fixture_store("build/long-scale-review/scale-store.json");CHECK(project_load()==0);render();CHECK(!memcmp(&proj,&after,sizeof proj));
    CHECK(cmd(VC_SET_BASS_FOLLOW,0,0,1,61)==VR_QUEUED);CHECK(cmd(VC_SET_BASS_FOLLOW,0,0,2,61)==VR_QUEUED);render();
    note=199;CHECK(!seq_bass_resolve(&seq,0,60,1,&note)&&note==199);seq_start(&seq);render();CHECK(!seq.t[TRK_BASS0].sounding);seq_stop(&seq);render();
}
int main(void){scale_oracle();durations();long_playback();adherence();printf("LONG SCALE: %d checkpoints PASS (32 bars, unequal lanes, internal/external clock, legal final notes, history/storage)\n",checks);return 0;}

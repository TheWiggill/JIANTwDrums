/* SPDX-License-Identifier: GPL-3.0-only
 * Four single-note lines with overlapping releases. A new note should reuse
 * that track's released voice before fading another track's audible tail. */
#define main hostsim_main
#include "hostsim.c"
#undef main
/* (cpu5) the same four lines under a load of 70 .. 90 %, simulated: the half's share fed to the guard (load_half)
 * every 4 blocks, as audio.c does; each part's measured cost set as the FM-1's scheduler would (mc_tk: ticks a block,
 * ~6 % of a half a voice). Checks: while the voices are under the cap and the average under the guard's threshold
 * no tail is cut by a new note (nothing given up); one 95 % half in a 70 % song (a spike) neither trips the guard nor
 * takes a voice; the average past 85 % trips the guard and never cuts a held lead; a prediction past 90 % (the
 * average 86, a voice 6 %) gives up a released tail for the new note instead of adding a voice */
static uint32_t tails_held(void)                        /* the released tails still ringing (stage 3) */
{
    uint32_t p,i,n=0;
    for(p=0;p<4;p++) for(i=0;i<NVOICE;i++) n+=trk[p].v[i].active&&trk[p].v[i].stage==3u;
    return n;
}
#define VOICE_PCT 6u                                    /* a voice's share of a half, as measured: 6 % */
static void load_tick(uint32_t base)                    /* a block rendered; a half's share (the base, the parts' own
                                                         * voices on top: as the FM-1 would measure it) to the guard every 4 */
{
    static uint32_t blk; int32_t out[2*CTL]; uint32_t p,i,nv;
    for(p=0;p<4;p++) {                                  /* each part's block as the scheduler times it: 1045 ticks a voice */
        for(nv=0,i=0;i<NVOICE;i++) nv+=trk[p].v[i].active;
        mc_tk[p]=1045u*(nv?nv:1u);
    }
    mix_block(out,CTL);
    if(!(++blk&3u)) { uint32_t pct=base+VOICE_PCT*voices_busy(); load_half(pct,pct,0); }
}
static int load_scene(void)
{
    uint32_t p,b,kills,checks=0,busy_max=0,steals,tails;
    host_tracks_init(); voice_headroom=0; sys_load=LOAD_FULL;
    for(p=0;p<4;p++) {                                  /* (ANALOG / VOICE: FM6's preset 0 never ends its release) */
        host_preset(&trk[p],p&1?ENGI_VOICE:ENGI_ANALOG,0);
        trk[p].p[P_VOICE]=V_POLY; trk[p].p[P_REL]=55; trk[p].p[P_SUS]=127;
    }
    for(b=0;b<512;b++) load_tick(40);                   /* the average settles (40 + the voices' 6 % each) */
    /* four single-note lines, a note every 120 blocks on the next part, its last one let go: overlapping tails */
    for(b=0;b<4000;b++) {
        uint32_t pct=28u+(b/40u)%9u;                    /* 28 .. 36 + the voices (4 .. 8 of them): 52 .. 84 */
        if(b%120u==0u) {
            track_t *t=&trk[(b/120u)%4u]; uint32_t i,note=48u+(b/120u)%12u;
            for(i=0;i<NVOICE;i++) if(t->v[i].active&&t->v[i].gate) trk_note_off(t,t->v[i].note);
            kills=voice_kills; tails=tails_held();
            if(!load_tight&&voices_busy()<voice_cap()) {
                trk_note_on(t,note,100); checks++;
                if(voice_kills!=kills||tails_held()!=tails) { printf("voice-load: a tail cut at block %u (avg %u, busy %u)\n",b,load_avg_q8/256,voices_busy()); return 10; }
            } else trk_note_on(t,note,100);
            if(voices_busy()>busy_max) busy_max=voices_busy();
        }
        load_tick(pct);
    }
    printf("voice-load: 52..84 %%: %u notes under the cap and the threshold, none cut a tail (busy at most %u, average %u, trips %u)\n",checks,busy_max,load_avg_q8/256,guard_trips);
    if(checks<16u||guard_trips) return 11;
    for(b=0;b<300;b++) load_tick(70u-VOICE_PCT*voices_busy());   /* 70 % */
    load_half(95,95,0);                                 /* one spike */
    kills=voice_kills; tails=tails_held();
    trk_note_on(&trk[0],72,100);
    if(load_tight||voice_kills!=kills||tails_held()!=tails) { printf("voice-load: the spike tripped or took a voice\n"); return 12; }
    printf("voice-load: a 95 %% half in a 70 %% song: no trip, nothing taken PASS\n");
    for(b=0;b<300;b++) load_tick(88u-VOICE_PCT*voices_busy());   /* 88 %: the average past 85, the guard trips */
    if(!load_tight) return 13;
    {
        uint32_t i,held=0;
        for(p=0;p<4;p++) for(i=0;i<NVOICE;i++) held+=trk[p].v[i].active&&trk[p].v[i].gate&&trk[p].v[i].stage!=4u;
        for(b=0;b<400;b++) { if(b%90u==0u) trk_note_on(&trk[b/90u%4u],60u+b/90u,100); load_tick(88u-VOICE_PCT*voices_busy()); }
        printf("voice-load: at 88 %% the guard trips (cap %u), every held lead kept\n",voice_cap());
        for(p=0;p<4;p++) { uint32_t h=0; for(i=0;i<NVOICE;i++) h+=trk[p].v[i].active&&trk[p].v[i].gate&&trk[p].v[i].stage!=4u; if(!h) return 14; }
        (void)held;
    }
    for(p=0;p<4;p++) { trk[p].p[P_REL]=30; trk_all_off(&trk[p]); }
    for(b=0;b<4000;b++) load_tick(50u-VOICE_PCT*voices_busy());   /* 50 %, calm: the guard lets go, the tails die */
    if(load_tight) return 15;
    for(p=0;p<4;p++) trk_note_on(&trk[p],50u+p,100);    /* a lead a track again */
    for(b=0;b<300;b++) load_tick(80u-VOICE_PCT*voices_busy());   /* 80 %: under the guard; a burst of notes past 90 */
    if(load_tight||voices_busy()>=voice_cap()) { printf("voice-load: at 80 %%: tight %u busy %u cap %u avg %u\n",load_tight,voices_busy(),voice_cap(),load_avg_q8/256); return 16; }
    for(p=0;p<4;p++) trk[p].p[P_REL]=55;
    trk_note_on(&trk[0],40,100); trk_note_on(&trk[2],42,100);   /* two notes held, then let go: two tails to give up */
    for(b=0;b<60;b++) load_tick(80u-VOICE_PCT*voices_busy());
    trk_note_off(&trk[0],40); trk_note_off(&trk[2],42);
    for(b=0;b<4;b++) load_tick(80u-VOICE_PCT*voices_busy());
    steals=load_steals; p=voices_busy(); tails=tails_held();
    trk_note_on(&trk[1],75,100); trk_note_on(&trk[1],77,100); trk_note_on(&trk[1],79,100);   /* 80 + 3 x 6 > 90 */
    printf("voice-load: at 80 %% three notes at once (6 %% each): a released tail given up past 90 %%, the count kept (%u -> %u, steals %u, tails %u -> %u)\n",p,voices_busy(),load_steals-steals,tails,tails_held());
    if(load_steals<steals+1u||voices_busy()>p+2u||tails_held()>=tails||load_tight) return 17;
    return 0;
}
int main(int argc,char **argv)
{
    uint32_t budget=argc>1?(uint32_t)atoi(argv[1]):8u;
    uint32_t p,b,kills; int32_t out[2*CTL]; voice_t *other=0;
    if(argc>1&&!strcmp(argv[1],"load")) return load_scene();
    host_tracks_init(); voice_headroom=budget==12; sys_load=LOAD_FULL;
    if(budget!=8&&budget!=12) return 4;
    for(p=0;p<4;p++) {
        host_preset(&trk[p],p&1?ENGI_VOICE:ENGI_FM6,0);
        trk[p].p[P_VOICE]=V_POLY; trk[p].p[P_REL]=100;
        trk_note_on(&trk[p],48+p,100);
    }
    for(b=0;b<128;b++) mix_block(out,CTL);
    for(p=0;p<4;p++) {
        trk_note_off(&trk[p],48+p);
        if(budget==12) {
            trk_note_on(&trk[p],54+p,100);
            trk_note_off(&trk[p],54+p);
        }
        trk_note_on(&trk[p],60+p,100);
    }
    for(p=0;p<NVOICE;p++) if(trk[0].v[p].active&&trk[0].v[p].note==48) other=&trk[0].v[p];
    if(!other||voices_busy()!=budget) return 2;
    trk_note_off(&trk[3],63); kills=voice_kills;
    trk_note_on(&trk[3],67,100);
    printf("voice-fairness: busy=%u cross_track_fades=%u oldest_other_tail_stage=%u\n",
           voices_busy(),voice_kills-kills,other->stage);
    if(voice_kills!=kills||other->stage==4||voices_busy()!=budget) return 1;
    for(p=0;p<3;p++) {
        uint32_t i,held=0;
        for(i=0;i<NVOICE;i++)held+=trk[p].v[i].active&&trk[p].v[i].gate&&trk[p].v[i].note==60+p;
        if(held!=1) return 3;
    }
    printf("voice-fairness: other tracks retain their lead and release PASS\n");
    /* If this part has no released slot, the normal global fallback must
     * still admit a new note and protect the other tracks' held leads. */
    for(p=0;p<NVOICE;p++) if(trk[3].v[p].active&&trk[3].v[p].stage!=4u) trk[3].v[p].gate=1;
    kills=voice_kills; trk_note_on(&trk[3],71,100);
    if(voice_kills!=kills+1||other->stage!=4||voices_busy()!=budget) return 5;
    printf("voice-fairness: global fallback remains available when no own release exists PASS\n");
    return 0;
}

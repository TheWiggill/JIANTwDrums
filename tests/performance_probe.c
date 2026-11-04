/* SPDX-License-Identifier: GPL-3.0-only
 * Repeatable host investigation; ns here are NOT FM1 utilization estimates.
 * Usage: performance_probe MODE ENGINE PRESET TRACKS NOTES FXMASK RTYPE OPS
 * MODE: track, mix, fx, idle, fmops, events_idle. FXMASK: chorus=1 delay=2 reverb=4.
 * One process per scene avoids state contamination. Thread CPU time excludes
 * scheduler pauses; compare medians of interleaved runs on the same machine.
 */
#define main hostsim_main
#include "hostsim.c"
#undef main
static volatile uint32_t probe_sink;
static uint64_t cpu_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
    return (uint64_t)ts.tv_sec * 1000000000u + ts.tv_nsec;
}
int main(int argc, char **argv)
{
    const char *mode=argc>1?argv[1]:"track";
    uint32_t en=argc>2?strtoul(argv[2],0,0):ENGI_FM6, pr=argc>3?strtoul(argv[3],0,0):0;
    uint32_t tracks=argc>4?strtoul(argv[4],0,0):1, notes=argc>5?strtoul(argv[5],0,0):4;
    uint32_t sends=argc>6?strtoul(argv[6],0,0):0, rt=argc>7?strtoul(argv[7],0,0):0;
    uint32_t ops=argc>8?strtoul(argv[8],0,0):6, b,k,j,live=0, blocks=FS*12u/CTL;
    int32_t out[2*CTL]; uint64_t t0,elapsed; int fxmode=!strcmp(mode,"fx"), dry=!strcmp(mode,"idle")||!strcmp(mode,"events_idle");
    if(en>=NENGINES||!tracks||tracks>NTRK||notes>NVOICE||ops>6||!ops||rt>4) return 2;
    host_tracks_init(); voice_headroom=1; sys_load=LOAD_FULL; mc_host=0;
    song.g[G_RTYPE]=rt;
    for(k=0;k<NTRK;k++) {
        host_preset(&trk[k],en,pr); trk[k].p[P_VOICE]=V_POLY; trk[k].p[P_SUS]=127;
        trk[k].p[P_DIST]=0;
        trk[k].p[P_CHOR]=(sends&1)?64:0;
        trk[k].p[P_DLY]=(sends&2)?64:0;
        trk[k].p[P_REV]=(sends&4)?64:0;
        if(!strcmp(mode,"fmops")) {
            uint8_t p[FP_SIZE+1u]; fm6_unpack(FM6_INIT,p);
            p[FP_ALG]=31; p[FP_FB]=0;
            for(j=0;j<6;j++) {
                uint8_t *o=p+j*FP_OP;
                o[FP_R1]=o[FP_R1+1]=o[FP_R1+2]=o[FP_R1+3]=99;
                o[FP_L1]=o[FP_L1+1]=o[FP_L1+2]=99; o[FP_L1+3]=0;
                o[FP_OL]=j<ops?99:0; o[FP_FC]=j+1; o[FP_KVS]=o[FP_LD]=o[FP_RD]=0;
            }
            fm6_set_patch(k,p); fm6_slot[k]=FM6_OWN; fm6_neutral(k);
            trk[k].p[P_ATK]=trk[k].p[P_DEC]=trk[k].p[P_REL]=0;
        }
    }
    if(fxmode) for(j=0;j<CTL;j++) {
        int32_t s=sine_i(j*0x051eb852u)/4;
        fxq_c[j]=(sends&1)?s:0; fxq_d[j]=(sends&2)?s:0; fxq_r[j]=(sends&4)?s:0;
    }
    t0=cpu_ns();
    for(b=0;b<blocks+1024u;b++) {
        if(b==1024u) t0=cpu_ns();
        if(!fxmode&&!dry&&!(b&255u)) for(k=0;k<tracks;k++) for(j=0;j<notes;j++)
            trk_note_on(&trk[k],en==ENGI_DRUM?DRUM_LANE_NOTE[j%NELEM(DRUM_LANE_NOTE)]:48+j*3,100);
        if(!strcmp(mode,"events_idle")) { events_block(CTL); probe_sink^=rng_state; }
        else if(fxmode) { fx_run(CTL); probe_sink^=(uint32_t)fx_wet[b%CTL]; }
        else if(!strcmp(mode,"track")||!strcmp(mode,"fmops")) {
            for(k=0;k<tracks;k++) track_render(&trk[k],out,CTL);
            probe_sink^=(uint32_t)out[b%CTL];
        } else { mix_block(out,CTL); probe_sink^=(uint32_t)out[b%(2*CTL)]; }
    }
    elapsed=cpu_ns()-t0;
    for(k=0;k<tracks;k++)for(j=0;j<NVOICE;j++)live+=trk[k].v[j].active;
    printf("mode=%s engine=%u preset=%u tracks=%u requested=%u live=%u sends=%u reverb=%u ops=%u ns_per_sample=%.4f checksum=%u\n",
        mode,en,pr,tracks,notes,live,sends,rt,ops,(double)elapsed/(blocks*CTL),probe_sink);
    if(en==ENGI_FM6) {
        uint32_t active_ops=0;
        for(j=0;j<6;j++) active_ops+=fm6_note[0][0].op[j].gain_out>=FM6_THRESH;
        printf("first_fm6_voice_active_operators=%u\n",active_ops);
    }
    printf("sizeof fm6_states=%zu chorus=%zu delay=%zu reverb_models=%zu reverb_comb=%zu comb=%zu slicer=%zu\n",
        sizeof fm6_note,sizeof cho_buf,sizeof dly_u,sizeof rev_x,sizeof rev_comb,sizeof comb_buf,sizeof sl_buf);
    return 0;
}

/* SPDX-License-Identifier: GPL-3.0-only
 * Compare the zero-ring shortcut against forcing the original three taps.
 * Covers tails, restart after silence, parameter movement, and LOAD changes.
 */
#define main hostsim_main
#include "hostsim.c"
#undef main
static uint64_t hash_bytes(uint64_t h,const void *p,size_t n)
{
    const uint8_t *s=p;
    while(n--) h=(h^*s++)*1099511628211ull;
    return h;
}
int main(int argc,char **argv)
{
    uint64_t h=1469598103934665603ull;
    uint32_t reference=argc>1&&!strcmp(argv[1],"reference"), b,i,rng=12345,skips=0;
    host_tracks_init();
    for(b=0;b<12000;b++) {
        uint32_t send=b%1000u>=80u&&b%1000u<200u;
        song.g[G_CDEPTH]=(b/19)%128; song.g[G_CRATE]=(b/37)%128;
        song.g[G_CSTER]=(int32_t)((b/7)%128)-64;
        song.g[G_WIDTH]=(b/23)%128; sys_load=(b/500)%3;
        for(i=0;i<CTL;i++) {
            rng=rng*1664525u+1013904223u;
            fxq_c[i]=send?(int32_t)(rng>>16)-32768:0;
            fxq_d[i]=fxq_r[i]=0;
        }
        if(reference) cho_zeros=0; /* execute the old taps even on a zero ring */
        else if(cho_zeros>=CHO_LEN&&!send) {
            skips++;
            for(i=0;i<CHO_LEN;i++) if(cho_buf[i]) return 2;
        }
        fx_run(CTL);
        h=hash_bytes(h,fx_wet,sizeof fx_wet);
        h=hash_bytes(h,fx_sl,sizeof fx_sl); h=hash_bytes(h,fx_sr,sizeof fx_sr);
        h=hash_bytes(h,&fx,sizeof fx);
        h=hash_bytes(h,&rng_state,sizeof rng_state);
    }
    h=hash_bytes(h,cho_buf,sizeof cho_buf);
    if(!reference&&skips<1000) return 3;
    printf("chorus-sleep: hash=%016llx mode=%s quiet_blocks=%u\n",(unsigned long long)h,reference?"reference":"sleep",skips);
    return 0;
}

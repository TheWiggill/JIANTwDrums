/* SPDX-License-Identifier: GPL-3.0-only
 * Compare all zero-buffer shortcuts against forcing the original FX kernels.
 * Covers fresh silence, sends, tails, resets, stereo/grain delay, reverb models,
 * parameter movement and LOAD changes; hash complete audio, FX state and buffers.
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
        uint32_t send=b%2000u>=500u&&b%2000u<520u;
        if(b==2000 || b==8000) fx_trip(0,CTL);
        if(b>=3000 && b<6000) {
            song.g[G_DPING]=(b/700)%2; song.g[G_WIDTH]=(b/500)%128;
            song.g[G_DPIT]=(b/800)%3; song.g[G_DSPRY]=(b/900)%20;
            song.g[G_RTYPE]=(b/400)%5; song.g[G_RMOD]=(b/37)%100;
        } else {
            song.g[G_DPING]=song.g[G_WIDTH]=song.g[G_DPIT]=song.g[G_DSPRY]=0;
            song.g[G_RTYPE]=song.g[G_RMOD]=0;
        }
        song.g[G_CDEPTH]=(b/19)%128; song.g[G_CRATE]=(b/37)%128;
        song.g[G_CSTER]=(int32_t)((b/7)%128)-64;
        sys_load=(b/500)%3;
        for(i=0;i<CTL;i++) {
            rng=rng*1664525u+1013904223u;
            fxq_c[i]=send?(int32_t)(rng>>16)-32768:0;
            fxq_d[i]=fxq_r[i]=send?(int32_t)(rng>>17)-16384:0;
        }
        if(reference) { room_used=dly_used=1; cho_zeros=0; } /* force original kernels even when their buffers are pristine */
        else if(cho_zeros>=CHO_LEN&&!send) {
            skips++;
            for(i=0;i<CHO_LEN;i++) if(cho_buf[i]) return 2;
        }
        fx_run(CTL);
        h=hash_bytes(h,fx_wet,sizeof fx_wet);
        h=hash_bytes(h,fx_sl,sizeof fx_sl); h=hash_bytes(h,fx_sr,sizeof fx_sr);
        h=hash_bytes(h,&fx,sizeof fx);
        h=hash_bytes(h,&rng_state,sizeof rng_state);
        h=hash_bytes(h,cl_g,sizeof cl_g); h=hash_bytes(h,shim_fb,sizeof shim_fb);   /* (cpu5: every model's clocks) */
        h=hash_bytes(h,&sh_tlp,sizeof sh_tlp); h=hash_bytes(h,&dlo,sizeof dlo);
    }
    h=hash_bytes(h,cho_buf,sizeof cho_buf);
    h=hash_bytes(h,dly_buf,sizeof dly_buf); h=hash_bytes(h,rev_comb,sizeof rev_comb);
    h=hash_bytes(h,&rev_u,sizeof rev_u); h=hash_bytes(h,&rev_x,sizeof rev_x);
    if(!reference&&skips<1000) return 3;
    printf("empty-fx: hash=%016llx mode=%s quiet_blocks=%u\n",(unsigned long long)h,reference?"reference":"sleep",skips);
    return 0;
}

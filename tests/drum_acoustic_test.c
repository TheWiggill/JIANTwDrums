/* SPDX-License-Identifier: GPL-3.0-only */
#define main hostsim_main
#include "hostsim.c"
#undef main
#ifndef BASELINE_DRUM
#define ACOUSTIC(x) (dx_acoustic=(x))
#else
#define ACOUSTIC(x) ((void)(x))
#endif
static uint32_t hit(uint32_t lane,uint32_t amount,uint32_t vel,int32_t *peak)
{
    uint32_t i,k,h=2166136261u;int32_t b[CTL];
    memset(trk,0,sizeof trk);memset(drum_kit,0,sizeof drum_kit);host_tracks_init();
    memcpy(dx_kit,DX_KIT_DEF,sizeof dx_kit);memcpy(dx_lay,DX_LAY_DEF,sizeof dx_lay);
    memset(dx_mot,0,sizeof dx_mot);dx_tone=dx_hands=dx_glue=0;dx_mot_rng=0x9e3779b9u;
    host_preset(&trk[0],ENGI_DRUM,0);ACOUSTIC(amount);
    trk[0].p[P_CHOR]=trk[0].p[P_DLY]=trk[0].p[P_REV]=0;
    trk_note_on(&trk[0],DRUM_LANE_NOTE[lane],vel);*peak=0;
    for(i=0;i<FS/CTL;i++){track_render(&trk[0],b,CTL);for(k=0;k<CTL;k++){
        uint32_t a=b[k]<0?(uint32_t)-b[k]:(uint32_t)b[k];if(a>(uint32_t)*peak)*peak=(int32_t)a;
        h=(h^(uint32_t)b[k])*16777619u;
    }}
    return h;
}
int main(void)
{
    uint32_t l;int bad=0;int32_t peak;
    for(l=0;l<8;l++){
        uint32_t off=hit(l,0,100,&peak);printf("original lane %u %08x\n",l,off);
#ifndef BASELINE_DRUM
        uint32_t on=hit(l,127,100,&peak);printf("on lane %u %08x peak %d\n",l,on,peak);bad|=on==off||peak<32||peak>32768;
        uint32_t again=hit(l,127,100,&peak);printf("again lane %u %08x peak %d\n",l,again,peak);bad|=again!=on;
        hit(l,64,24,&peak);printf("quiet %u %d\n",l,peak);bad|=peak>32768;
        hit(l,127,127,&peak);printf("hard %u %d\n",l,peak);bad|=peak>32768;
#endif
    }
    printf("DRUM acoustic %s\n",bad?"FAILED":"passed");return bad;
}

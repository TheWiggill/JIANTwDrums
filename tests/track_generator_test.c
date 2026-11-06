/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int verify(track_t *t,unsigned genre,unsigned role,unsigned len)
{
    static project_t before,after;
    unsigned k=trk_index(t),i,j;int ok=1;
    song.sel=(uint8_t)k;t->p[P_SLEN]=(int16_t)len;t->seq_fit=128u|16u;
    t->p[P_LEVEL]=75;t->p[P_SDIV]=5;t->p[P_REV]=17;
    for(i=len;i<NSTEP;i++) t->step[i]=(step_t){{75},1,ST_NOTE,0,96};
    motion_clear(t);motion_set_lock(t,0,P_E0,20);
    tg_config[k]=(tg_config_t){367,(uint8_t)genre,(uint8_t)role,60,70};
    project_capture(&before);
    __typeof__(gp) p=gp;__typeof__(gs) s=gs;__typeof__(gx) x=gx;ggenre_t g=gmut;uint32_t q=gq;
    char name[20];memcpy(name,gmut_name,20);
    trackgen_make();project_capture(&after);
    ok &= !memcmp(&p,&gp,sizeof p)&&!memcmp(&s,&gs,sizeof s)&&!memcmp(&x,&gx,sizeof x)&&
          !memcmp(&g,&gmut,sizeof g)&&q==gq&&!memcmp(name,gmut_name,20);
    ok &= !memcmp(before.g,after.g,sizeof before.g)&&before.sel==after.sel&&
          !memcmp(&before.chain,&after.chain,sizeof before.chain)&&!memcmp(&before.motion,&after.motion,sizeof before.motion);
    for(i=0;i<NTRK;i++) {
        ok &= !memcmp(before.t[i].p,after.t[i].p,sizeof before.t[i].p);
        if(i!=k) ok &= !memcmp(before.t[i].step,after.t[i].step,sizeof before.t[i].step);
    }
    ok &= t->seq_fit==(128u|16u)&&song.playing;
    for(i=0;i<NSTEP;i++) {
        const step_t *st=&t->step[i];ok &= st->n<=4&&st->time<=ST_REST;
        if(i>=len) ok &= !memcmp(st,&before.t[k].step[i],sizeof *st);
        if(!drum_track(t)) for(j=0;j<st->n&&i<len;j++) ok &= st->note[j]==scale_snap(t,st->note[j]);
    }
    step_t first[NSTEP];memcpy(first,t->step,sizeof first);trackgen_make();
    ok &= !memcmp(first,t->step,sizeof first);
    return ok;
}
int main(void)
{
    int bad=0,ok=1;unsigned count=0;
    ui_power_on();song.playing=1;
    /* Start with an actual HOME song context, then generate into individual
     * engines while preserving that context and every other track. */
    song.playing=0;gp.genre=1;gp.form=0;gen_go();song.playing=1;
    for(unsigned e=0;e<NENGINES;e++) {
        if(!eng_ok(e)||e==ENGI_INPUT) continue;
        song.sel=1;set_engine_of(&trk[1],e);trk[1].engine=trk[1].eng_req;
        for(unsigned role=0;role<(e==ENGI_DRUM?1u:4u);role++)
            for(unsigned li=0;li<4;li++) {
                static const unsigned lens[]={1,7,18,64};
                ok &= verify(&trk[1],1u+(e+role)%NGENRE,role,lens[li]);count++;
            }
    }
    bad+=check("track GEN preserves HOME, other tracks, sounds, locks, FIT and live transport",ok);
    printf("track GEN: %u engine/role/length configurations\n",count);
    ok=1;song.sel=0;set_engine_of(&trk[0],ENGI_DRUM);trk[0].engine=ENGI_DRUM;
    for(unsigned g=0;g<=NGENRE;g++) ok &= verify(&trk[0],g,0,16);
    ok &= verify(&trk[0],GEN_EXPLORE,0,16);
    bad+=check("track GEN all drum genres and EXPLORE are bounded, deterministic and isolated",ok);
    song.sel=1;set_engine_of(&trk[1],ENGI_ANALOG);trk[1].engine=ENGI_ANALOG;
    song.playing=0;go_page(GR_TRACKGEN);ui.force=1;frame();
    ok=page_visible(ui.page)&&act_col()==5&&strstr(ui.col[0],"GENRE");
    unsigned previous=tg_config[1].role;edit_param(2,1);ok &= tg_config[1].role==(previous<3?previous+1:3);
    bad+=check("track GEN final SEQ page exposes genre, seed, role and energy",ok);
    set_engine_of(&trk[1],ENGI_INPUT);trk[1].engine=ENGI_INPUT;
    bad+=check("track GEN hidden for INPUT, which has no pitched sequence engine",!page_visible(ui.page));
    printf("track GEN: track_t %zu B, main stack limit 15.75 KiB; %s\n",sizeof(track_t),bad?"FAILED":"all ok");
    return bad!=0;
}

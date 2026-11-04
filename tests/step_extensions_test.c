/* SPDX-License-Identifier: GPL-3.0-only */
#define STEP_EXTENSIONS_TEST 1
#include "comp_test.c"


static int shift_tests(void)
{
    static const uint8_t notes[8] = {60, 62, 64, 65, 67, 69, 71, 72};
    uint8_t nt[64]; uint16_t at[64]; step_t before[8]; uint32_t n;
    int bad = 0;
    track_t *t = pat(8, notes);
    comp_set(&t->step[1], 0, CA_SHIFT, CC_ALWAYS, 0, 1, 65); /* +2 */
    memcpy(before, t->step, sizeof before); seq_start();
    n = run_at(t, 8, nt, at, 64);
    { static const uint8_t want[] = {60,64,62,67,69,71,72};
      bad += check("SHIFT +2 removes source and replaces destination without moving head or editing pattern",
          seq_is(nt,n,want,sizeof want) && at[2] == 3 && t->seq_idx == 7 && !memcmp(before,t->step,sizeof before)); }
    t = pat(8, notes);
    comp_set(&t->step[5], 0, CA_SHIFT, CC_ALWAYS, 0, 1, 60); /* -3 */
    seq_start(); n = run_at(t,8,nt,at,64);
    { static const uint8_t want[] = {60,62,69,65,67,71,72};
      bad += check("SHIFT -3 anticipates note on the first pass, once only",seq_is(nt,n,want,sizeof want) && at[2]==2); }
    t = pat(8, notes);
    comp_set(&t->step[0], 0, CA_SHIFT, CC_NM, CNM_MAKE(2,2), 1, 62); /* -1 wraps */
    seq_start(); n=run_at(t,16,nt,at,64);
    bad += check("SHIFT N:M is decided for each pass and wraps inside LEN",
                 n==15 && nt[0]==60 && at[0]==0 && nt[14]==60 && at[14]==15);
    t = pat(8,notes);
    comp_set(&t->step[1],0,CA_SHIFT,CC_ALWAYS,0,1,65);
    comp_set(&t->step[1],1,CA_OCTU,CC_ALWAYS,0,1,0);
    seq_start(); n=run_at(t,8,nt,at,64);
    bad += check("SHIFT carries the second component to the replaced destination",n==7 && nt[2]==74 && at[2]==3);
    t = pat(8,notes);
    comp_set(&t->step[0],0,CA_SHIFT,CC_ALWAYS,0,1,65);
    comp_set(&t->step[0],1,CA_SHIFT,CC_ALWAYS,0,1,64);
    seq_start(); shift_prepare_pass(t,8,0);
    bad += check("two SHIFT components add their offsets",shift_step(t,3)==&t->step[0] && shift_step(t,0)->time==ST_REST);
    t = pat(8,notes);
    comp_set(&t->step[1],0,CA_SHIFT,CC_ALWAYS,0,1,65);
    comp_set(&t->step[1],1,CA_OCTU,CC_ALWAYS,0,1,0);
    step_set_ratchet(&t->step[1],2);
    seq_start();n=run_at(t,8,nt,at,64);
    {uint32_t i,hits=0;int ok=1;for(i=0;i<n;i++){if(nt[i]==74)hits++;if(nt[i]==65)ok=0;}
     bad+=check("shifted ratchets repeat the moved note with C2, never the replaced note",ok && hits==2);}
    t=pat(8,notes);
    comp_set(&t->step[3],0,CA_SHIFT,CC_NM,CNM_MAKE(2,2),1,60);
    ST_MICRO_SET(&t->step[3],-16);
    seq_start();n=run_at(t,16,nt,at,64);
    {uint32_t i,hits=0;int ok=1;for(i=0;i<n;i++)if(nt[i]==65){ok &= at[i]==(hits?7:2);hits++;}
     bad+=check("SHIFT lookahead keeps early microtiming when N:M changes at a loop boundary",ok && hits==2);}
    return bad;
}
static int chord_tests(void)
{
    static const uint8_t notes[4] = {60,0,0,0};
    int bad=0, ok=1; uint32_t mode,k,n; uint8_t want[4],nt[32];
    track_t *t;
    for(mode=CD_TRI;mode<=CD_POW;mode++) {
        const step_t *s;
        t=pat(4,notes); t->p[P_VOICE]=V_POLY;
        t->p[P_CHRD]=CD_OFF;
        comp_set(&t->step[0],0,CA_CHORD,CC_ALWAYS,0,1,mode-1);
        n=chord_build_mode(t,60,want,mode);
        s=comp_run(t,&t->step[0],4);
        ok &= s->n==n && !memcmp(s->note,want,n) && t->p[P_CHRD]==CD_OFF && t->step[0].n==1;
    }
    bad+=check("CHORD reuses every generator shape without changing track settings or stored notes",ok);
    t=pat(4,notes);t->p[P_VOICE]=V_POLY;
    comp_set(&t->step[0],0,CA_CHORD,CC_ALWAYS,0,1,CD_MAJ-1);
    comp_set(&t->step[0],1,CA_OCTU,CC_ALWAYS,0,1,0);
    seq_start();n=run(t,4,nt,32);
    {static const uint8_t w[]={72,76,79};bad+=check("CHORD stacks with COMP 2 OCT+ in actual playback",seq_is(nt,n,w,3));}
    t=pat(4,notes);t->p[P_VOICE]=V_POLY;
    comp_set(&t->step[0],0,CA_CHORD,CC_FIRST,0,1,CD_MAJ-1);
    comp_set(&t->step[0],1,CA_CHORD,CC_ELSE,0,1,CD_MIN-1);
    t->loops=1;
    {const step_t *s=comp_run(t,&t->step[0],4);bad+=check("CHORD supports C2 ELSE and bar conditions",s->n==3 && s->note[1]==63);}
    gl_blank();set_engine_of(&trk[3], ENGI_ANALOG);trk[3].step[0]=(step_t){{60},1,ST_NOTE,0,100};trk[3].p[P_VOICE]=V_POLY;
    comp_set(&trk[3].step[0],0,CA_CHORD,CC_ALWAYS,0,1,CD_MIN-1);
    gl_set(0,CA_CHORD,CC_ALWAYS,0,1,CD_MAJ-1,1u<<3);
    gl_mono=0;n=grun(4);ok=n>=3 && g_note[1]==63;
    gl_mono=1;n=grun(4);ok &= n>=3 && g_note[1]==64;
    bad+=check("master CHORD POLY lets track CHORD override; MONO keeps master chord",ok);
    t=pat(4,notes);t->p[P_VOICE]=V_MONO;
    comp_set(&t->step[0],0,CA_CHORD,CC_ALWAYS,0,1,CD_MAJ-1);
    {const step_t *s=comp_run(t,&t->step[0],4);bad+=check("CHORD preserves generator's MONO voice limit",s->n==1);}
    return bad;
}
static int extended_format(void)
{
    static const uint8_t notes[4]={60,0,0,0};
    project_store_t st;
    track_t *t=pat(4,notes);uint32_t n;int ok;
    comp_set(&t->step[0],0,CA_SHIFT,CC_NM,CNM_MAKE(2,4),1,60);
    comp_set(&t->step[0],1,CA_CHORD,CC_NNM,CNM_MAKE(1,3),4,CD_MIN7-1);
    gl_set(0,CA_CHORD,CC_ALWAYS,0,1,CD_MAJ7-1,1);
    project_capture(&proj_scratch); ok=proj_pack(&st,&proj_scratch);
    if(ok) { t->step[0].comp=t->step[0].comp2=0;gl_clear();ok=proj_import(&proj_scratch,&st,sizeof st) && !project_restore_runtime(&proj_scratch); }
    ok &= CO_ACT(t->step[0].comp)==CA_SHIFT && t->step[0].carg==60 && t->step[0].cnm[0]==CNM_MAKE(2,4);
    ok &= CO_ACT(t->step[0].comp2)==CA_CHORD && t->step[0].carg2==CD_MIN7-1 && CO_ACT(glane[0].comp)==CA_CHORD;
    return check("SHIFT and CHORD survive project save/load with N:M, C2 and master intact",ok);
}
static int extended_ui(void)
{
    static const uint8_t notes[4]={60,0,0,0};
    track_t *t=pat(4,notes);int ok;
    comp_set(&t->step[0],0,CA_BEND,CC_P50,0,1,12);
    go_page(GR_COMP);
    comp_knob(0,1);
    ok=CO_ACT(t->step[0].comp)==CA_SHIFT && CO_COND(t->step[0].comp)==CC_ALWAYS;
    comp_knob(2,1); ok &= CO_COND(t->step[0].comp)==CC_FIRST;
    comp_knob(2,1); ok &= CO_COND(t->step[0].comp)==CC_NFIRST;
    comp_knob(2,1); ok &= CO_COND(t->step[0].comp)==CC_NM && t->step[0].cnm[0]==CNM_MAKE(1,2);
    comp_knob(3,-3); ok &= t->step[0].carg==60;
    comp_knob(0,1); ok &= CO_ACT(t->step[0].comp)==CA_CHORD;
    comp_knob(3,CD_MIN-1);frame();
    ok &= t->step[0].carg==CD_MIN-1 && strstr(ui.col[3],"MIN")!=0;
    return check("COMP controls expose SHIFT pass conditions and selectable CHORD shapes",ok);
}
static int shift_limits(void)
{
    uint8_t notes[64],nt[128];uint16_t at[128];uint32_t i,n;int bad=0,ok=1;track_t *t;
    for(i=0;i<64;i++)notes[i]=(uint8_t)(24+i);
    t=pat(64,notes);comp_set(&t->step[63],0,CA_SHIFT,CC_ALWAYS,0,1,0);
    seq_start();n=run_at(t,64,nt,at,128);
    bad+=check("SHIFT -63 anticipates the last of 64 steps onto the first",n==63 && nt[0]==87 && at[0]==0);
    t=pat(64,notes);comp_set(&t->step[0],0,CA_SHIFT,CC_ALWAYS,0,1,126);
    seq_start();n=run_at(t,64,nt,at,128);
    bad+=check("SHIFT +63 moves first step to last and replaces it",n==63 && nt[62]==24 && at[62]==63);
    t=pat(64,notes);comp_set(&t->step[0],0,CA_SHIFT,CC_NM,CNM_MAKE(16,16),32,126);
    t->loops=15;memset(shift_valid,0,sizeof shift_valid);shift_prepare_pass(t,64,15);
    for(i=0;i<31;i++)ok &= shift_step(t,i)==&t->step[i+1];
    ok &= shift_step(t,31)->time==ST_REST && shift_step(t,63)==&t->step[0];
    bad+=check("SHIFT maximum LENGTH 32 and N:M 16:16 wrap a 64-step pattern safely",ok);
    return bad;
}
int main(void)
{
    int bad=original_comp_main();bad+=shift_tests()+chord_tests()+extended_format()+extended_ui()+shift_limits();
    printf("%s\n",bad?"STEP EXTENSIONS FAILED":"step extensions passed");return !!bad;
}

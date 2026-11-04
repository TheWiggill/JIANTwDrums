/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
int main(void)
{
    static step_t source[NSTEP],before[NSTEP];
    uint32_t i,engine,preset;int bad=0;int16_t sound[8],lock;
    ui_power_on();set_engine_of(&trk[0],5u);track_select(0);go_page(GR_ROLL);
    trk[0].p[P_SLEN]=NSTEP;
    for(i=0;i<NSTEP;i++) {
        step_t *s=&trk[0].step[i];step_clear(s);s->time=ST_NOTE;s->n=1;s->note[0]=(uint8_t)(48+i%24);s->vel=(uint8_t)(64+i);
        s->comp=CO_MAKE(CA_OCTU,CC_NM);s->cnm[0]=CNM_MAKE(1,2);ST_MICRO_SET(s,(int32_t)(i%64)-32);
    }
    motion_set_lock(&trk[0],63,P_FCUT,80);motion_set_lock(&trk[0],0,P_E0,4);
    memcpy(source,trk[0].step,sizeof source);
    ly_white(B_SEQ,TL_COPY);
    bad+=check("SEQ hold COPY SEQ captures all 64 steps without changing source or playing a key",clip.n==64 && !memcmp(source,trk[0].step,sizeof source) && !gates());
    track_select(1);set_engine_of(TSEL,ENGI_ANALOG);my_steps(TSEL);TSEL->p[P_SLEN]=16;
    engine=TSEL->eng_req;preset=TSEL->preset;memcpy(sound,&TSEL->p[P_E0],sizeof sound);memcpy(before,TSEL->step,sizeof before);
    ly_white(B_SEQ,TL_PASTE);
    bad+=check("PASTE SEQ replaces destination sequence and LEN while preserving engine, preset and sound",TSEL->p[P_SLEN]==64 && !memcmp(source,TSEL->step,sizeof source) && TSEL->eng_req==engine && TSEL->preset==preset && !memcmp(sound,&TSEL->p[P_E0],sizeof sound));
    bad+=check("compatible step locks follow the phrase; foreign engine-specific locks do not",motion_lock_get(TSEL,63,P_FCUT,&lock) && lock==80 && !motion_lock_get(TSEL,0,P_E0,&lock));
    bad+=check("UNDO restores the destination before a whole-sequence paste",!undo_do() && !memcmp(before,TSEL->step,sizeof before) && TSEL->p[P_SLEN]==16);
    clip.n=0;ly_white(B_SEQ,TL_PASTE);
    bad+=check("PASTE SEQ refuses an empty clipboard without changing track",msg_is("NOTHING COPIED") && !memcmp(before,TSEL->step,sizeof before));
    clip.n=1;clip.drum=0;chain.running=1;ly_white(B_SEQ,TL_PASTE);
    bad+=check("PASTE SEQ respects song-chain editing guard",msg_is("SONG PLAYS: STOP IT TO EDIT") && !memcmp(before,TSEL->step,sizeof before));
    chain.running=0;
    printf("clipboard RAM bytes: %zu; step bytes: %zu\n",sizeof clip,sizeof(step_t));
    return !!bad;
}

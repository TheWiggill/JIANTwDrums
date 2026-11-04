/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
int main(void)
{
    static project_t q;static project_store_t store;uint32_t i,bad=0,initial;
    ui_power_on();initial=autosave_sig();
    for(i=0;i<128;i++){
        dx_acoustic=(uint8_t)i;project_capture(&q);
        bad+=check("ACOUSTIC packed project round trip",proj_pack(&store,&q)&&proj_import(&q,&store,sizeof store)&&q.dx_acoustic==i);
        dx_acoustic=0;bad+=check("ACOUSTIC runtime restore",!project_restore_runtime(&q)&&dx_acoustic==i);
    }
    dx_acoustic=64;bad+=check("ACOUSTIC participates in autosave",autosave_sig()!=initial);
    dx_acoustic=0;project_capture(&q);bad+=check("legacy default remains zero without DA extension",proj_pack(&store,&q)&&proj_import(&q,&store,sizeof store)&&!q.dx_acoustic);
    set_engine_of(TSEL,ENGI_DRUM);uint32_t page;
    for(page=0;page<NPAGES;page++)if(str_eq(PAGES[page].title,"KIT 4"))break;
    bad+=check("KIT 4 is reachable only on DRUM",page<NPAGES&&page_visible(page));
    ui.home=0;ui.page=page;dx_kit3_knob(0,64);bad+=check("KIT 4 knob 1 edits ACOUSTIC",dx_acoustic==64);
    set_engine_of(TSEL,ENGI_ANALOG);bad+=check("KIT 4 hidden on other engines",!page_visible(page));
    bad+=check("ADD kept at engine 13; no SAMPLE engine",ENGINES[13]==&ENG_ADD&&eng_gone(ENGI_SAMPLE));
    return !!bad;
}

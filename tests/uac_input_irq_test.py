"""Compile the production TIMER5 body with a one-packet USB RX model and real input ring/resampler."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'firmware/src/main.c').read_text();a=s.index('void fm1_timer5_irq(void)');b=s.index('extern void isr_timer5',a);irq=s[a:b]
base=(root/'tests/uac_test.c').resolve()
code=r'''
#define main upstream_main
#include "@BASE@"
#undef main
static struct {unsigned timer_irqs,nested,in_audio;} felucca_dbg;
static uint32_t t5_nested_ticks,now_ticks;
#define FM1_TICKS_PER_US 24u
#define FELUCCA_UART 1
static unsigned rx_pending,rx_n,lost,all_polls,bad_polls,rx_nested,tx_service;
static uint32_t rx_packet[50];
static uint32_t fm1_ticks(void){return now_ticks;}
static void fm1_timer5_ack(void){}
static void fm1_input_tick(void){}
static void uart_midi_poll(void){}
static void input_test_uin_rx(void){if(rx_pending){uin_take(rx_packet,rx_n*4u);rx_pending=0;if(felucca_dbg.in_audio)rx_nested++;}}
static void input_test_uac_service(void){tx_service++;}
static void input_test_usb_poll(void){all_polls++;if(felucca_dbg.in_audio)bad_polls++;if(usb.up&&usb.config&&uin.alt)input_test_uin_rx();}
#define uac_service input_test_uac_service
#define usb_poll input_test_usb_poll
#define uin_rx input_test_uin_rx
@IRQ@
#undef uac_service
#undef usb_poll
#undef uin_rx
static int irq_case(unsigned percent,unsigned phase){
 const double fs=44117.6470588,half=128.*1e6/fs,duration=half*percent/100.;
 uint32_t start_ticks=now_ticks+2400u;
 double next_half=0,next_read=0,end=0;unsigned block=2,acc=0,pkts=0,lost0=0,under0=0,over0=0;
 memset(&uin,0,sizeof uin);ui_w=ui_r=0;usb.up=usb.config=uin.alt=1;uh_aud_on=0;rx_pending=0;lost=all_polls=bad_polls=rx_nested=0;
 for(unsigned t=0;t<10000000;t+=100){
  /* OUT packet, then the audio IRQ; TIMER5 preempts the audio ISR. */
  if(t%1000==phase){if(rx_pending)lost++;rx_pending=1;rx_n=44;if((acc+=100)>=1000){acc-=1000;rx_n++;}pkts++;}
  if(t>=next_half){next_half+=half;end=t+duration;next_read=t;block=0;}
  felucca_dbg.in_audio=t<end;
  now_ticks=start_ticks+t*24;fm1_timer5_irq();
  if(block<2 && t>=next_read){uac_in_fill(64);block++;next_read=t+duration/2.;}
  if(t<1000000){lost0=lost;under0=uin.underruns;over0=uin.overruns;}
 }
 unsigned loss=lost-lost0,under=uin.underruns-under0,over=uin.overruns-over0;
 printf("render %u%% phase %uus: packet loss %u, underruns %u, overruns %u, nested RX %u, forbidden full polls %u\n",percent,phase,loss,under,over,rx_nested,bad_polls);
 return !!(loss||under||over||bad_polls||!rx_nested);
}
int main(){int bad=0;for(unsigned load=20;load<=80;load+=30)for(unsigned phase=0;phase<1000;phase+=300)bad+=irq_case(load,phase);return bad!=0;}
'''.replace('@BASE@',str(base)).replace('@IRQ@',irq)
with tempfile.TemporaryDirectory() as tmp:
 c=Path(tmp)/'test.c';c.write_text(code);exe=Path(tmp)/'test'
 subprocess.run(['cc','-O2','-w','-DHALF_FRAMES=128','-DT_CDC=0',str(c),'-lm','-o',str(exe)],check=True)
 raise SystemExit(subprocess.run([str(exe)]).returncode)

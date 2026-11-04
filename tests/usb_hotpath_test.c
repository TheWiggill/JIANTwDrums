/* SPDX-License-Identifier: GPL-3.0-only
 * USB workload regression: use the real firmware stream, count mode reads as
 * well as checking delivered frames. This is an operation budget, not measured
 * hardware CPU usage. The old loops exceed 600 reads per packet. */
#include <stdint.h>
static uint64_t mode_reads;
#define UAC_STEMS() (mode_reads++, t_stems)
#define main stems_usb_suite_main
#include "stems_usb_test.c"
#undef main

int main(void)
{
    for (uint32_t ch = 8; ch <= 10; ch += 2) {
        double now = 0;
        M.fs = 44117.6; M.load = .70; M.p_late = .043; M.late_max = 1.31;
        reset(ch, 0, 0);
        mode_reads = 0;
        sim(10.0, &now);
        printf("%u channels: %llu mode reads, %llu packets, %llu frames\n", ch,
               (unsigned long long)mode_reads, (unsigned long long)R.pkts, (unsigned long long)R.got);
        check("mode lookup bounded per packet/block, independent of frames/channels", mode_reads < R.pkts * 16u);
        check("each sample arrives once in its own channel", R.got > 440000u && !R.lost && !R.repeats && !R.moved && !R.partial);
    }
    return fails != 0;
}

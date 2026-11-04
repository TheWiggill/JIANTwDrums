// SPDX-License-Identifier: GPL-3.0-only
// JIANT Stems: usbstream.c on the computer, against a pretend FM-1. ioctl() is replaced by a usbfs that completes
// the URBs in order (the IN ones full of a counter, 44 or 45 frames a millisecond as the FM-1 sends 44.1 kHz; the
// OUT ones recorded) and JNIEnv by the few calls the driver makes. Checks: every frame comes through once, in
// order, across many pump() calls; the mic pushed comes out on the OUT side in order, on both sides, at 44.1 frames
// a millisecond; the mic's clock drifting from the USB's (+-200 ppm, ten minutes) keeps the ring at its 10 ms with
// no drop and no silence, read a little faster or slower (1.1: no sample skipped or repeated, a sine continuous); an
// IN packet in error with a frame cut short leaves every later frame whole (1.1); an unplug ends the stream with an
// error; stop() takes every URB back.
//   cc -I$JAVA_HOME/include -I$JAVA_HOME/include/linux usbstream_test.c -o t -lm && ./t
#define _GNU_SOURCE
#include <jni.h>
#include <errno.h>
#include <linux/usbdevice_fs.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <math.h>

/* ---- the pretend usbfs */
#define QN 64
static struct usbdevfs_urb *q[QN];
static int qn, unplug_after = -1, reaps, live;
static int ch = 10, in_counter, acc, cut_at = -1;
static int16_t out_seen[1 << 20];
static int out_n;
static int out_packets_frames, out_packets;

int ioctl(int fd, unsigned long req, ...)
{
    (void)fd;
    va_list ap;
    va_start(ap, req);
    void *arg = va_arg(ap, void *);
    va_end(ap);
    if (req == USBDEVFS_SUBMITURB) {
        if (qn == QN) { errno = ENOMEM; return -1; }
        q[qn++] = arg;
        live++;
        return 0;
    }
    if (req == USBDEVFS_DISCARDURB) return 0;
    if (req == USBDEVFS_REAPURBNDELAY) {
        if (unplug_after >= 0 && reaps >= unplug_after) { errno = ENODEV; return -1; }
        if (!qn) { errno = EAGAIN; return -1; }
        struct usbdevfs_urb *u = q[0];
        memmove(q, q + 1, sizeof q[0] * (size_t)--qn);
        reaps++;
        live--;
        if (u->endpoint & 0x80) {                         /* IN: the counter, frame by frame */
            int16_t *d = u->buffer;
            int maxp = u->buffer_length / u->number_of_packets;
            for (int k = 0; k < u->number_of_packets; k++) {
                int f = 44;
                acc += 100; if (acc >= 1000) { acc -= 1000; f = 45; }
                if (k == 3) f = 0;                         /* one empty packet an URB */
                int16_t *p = (int16_t *)((char *)u->buffer + k * maxp);
                for (int i = 0; i < f * ch; i++) p[i] = (int16_t)(in_counter++);
                u->iso_frame_desc[k].actual_length = (unsigned)(f * ch * 2);
                u->iso_frame_desc[k].status = 0;
                if (reaps == cut_at && k == 5 && f) {     /* (1.1) a packet in error, its last frame cut short */
                    u->iso_frame_desc[k].actual_length -= 6;
                    u->iso_frame_desc[k].status = -EOVERFLOW;
                }
            }
            (void)d;
        } else {                                           /* OUT: what was sent */
            int off = 0;
            for (int k = 0; k < u->number_of_packets; k++) {
                int16_t *p = (int16_t *)((char *)u->buffer + off);
                int n = (int)u->iso_frame_desc[k].length / 2;
                if ((int)u->iso_frame_desc[k].length % 4) { fprintf(stderr, "odd OUT packet\n"); exit(1); }
                for (int i = 0; i < n && out_n < (int)(sizeof out_seen / 2); i++) out_seen[out_n++] = p[i];
                off += (int)u->iso_frame_desc[k].length;
                out_packets_frames += n / 2;
                out_packets++;
            }
            if (off != (int)u->buffer_length) { fprintf(stderr, "OUT length %d != %d\n", off, u->buffer_length); exit(1); }
        }
        *(void **)arg = u;
        return 0;
    }
    errno = EINVAL;
    return -1;
}

/* ---- the pretend JNI: arrays are {length, data} */
typedef struct { jsize n; void *p; } arr;
static jsize JNICALL get_len(JNIEnv *e, jarray a) { (void)e; return ((arr *)a)->n; }
static jbyte *JNICALL get_b(JNIEnv *e, jbyteArray a, jboolean *c) { (void)e; if (c) *c = 0; return ((arr *)a)->p; }
static void JNICALL rel_b(JNIEnv *e, jbyteArray a, jbyte *p, jint m) { (void)e; (void)a; (void)p; (void)m; }
static jshort *JNICALL get_s(JNIEnv *e, jshortArray a, jboolean *c) { (void)e; if (c) *c = 0; return ((arr *)a)->p; }
static void JNICALL rel_s(JNIEnv *e, jshortArray a, jshort *p, jint m) { (void)e; (void)a; (void)p; (void)m; }
static void JNICALL set_i(JNIEnv *e, jintArray a, jsize s, jsize n, const jint *v) { (void)e; memcpy((jint *)((arr *)a)->p + s, v, sizeof(jint) * (size_t)n); }

/* the driver itself, its statics in reach (the drift test drives fill_out directly) */
#include "../../main/cpp/usbstream.c"

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; fprintf(stderr, "FAIL %s:%d: ", __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } } while (0)

int main(void)
{
    struct JNINativeInterface_ fn;
    memset(&fn, 0, sizeof fn);
    fn.GetArrayLength = get_len;
    fn.GetByteArrayElements = get_b;
    fn.ReleaseByteArrayElements = rel_b;
    fn.GetShortArrayElements = get_s;
    fn.ReleaseShortArrayElements = rel_s;
    fn.SetIntArrayRegion = set_i;
    const struct JNINativeInterface_ *envp = &fn;
    JNIEnv *env = (JNIEnv *)&envp;

    static jbyte buf[65536];
    static jint st[10];
    arr ab = {sizeof buf, buf}, as = {10, st};
    int maxp = 920;                                        /* STEMS+IN */

    CHECK(Java_fm_jiant_stems_UsbNative_start(env, NULL, 3, 0x81, maxp, 0x01, ch * 2) == 0, "start");
    CHECK(Java_fm_jiant_stems_UsbNative_start(env, NULL, 3, 0x81, maxp, 0x01, ch * 2) == -EBUSY, "second start");

    /* the mic: a ramp, pushed in pieces as the mic thread would, between the pumps (the pretend device is not
     * paced in real time, so the ring both runs dry and overflows: silence sent, mic dropped, never reordered) */
    enum { NMIC = 30000 };
    static jshort mic[NMIC];
    for (int i = 0; i < NMIC; i++) mic[i] = (jshort)(i + 1);
    int pushed = 0;
    long frames = 0;
    int expect = 0, order_ok = 1;
    for (int call = 0; call < 400; call++) {
        if (pushed < NMIC) {
            int n = 1500 < NMIC - pushed ? 1500 : NMIC - pushed;
            arr am = {n, mic + pushed};
            Java_fm_jiant_stems_UsbNative_pushMic(env, NULL, (jshortArray)&am, n);
            pushed += n;
        }
        int got = Java_fm_jiant_stems_UsbNative_pump(env, NULL, (jbyteArray)&ab, 5, (jintArray)&as);
        CHECK(got >= 0, "pump %d", got);
        CHECK(got % (ch * 2) == 0, "whole frames (%d)", got);
        int16_t *s = (int16_t *)buf;
        for (int i = 0; i < got / 2; i++) if (s[i] != (int16_t)expect++) order_ok = 0;
        frames += got / (ch * 2);
    }
    CHECK(order_ok, "every IN sample once, in order");
    CHECK(frames > 44100, "frames %ld", frames);
    CHECK(st[1] > 0 && st[2] == 0 && st[3] == 0, "stats empty %d err %d errno %d", st[1], st[2], st[3]);
    CHECK(st[4] == 45 * ch * 2, "largest packet %d", st[4]);

    /* OUT: the ramp's samples in order on both sides, each once (some dropped when the ring overflowed), the rest
     * silence; 44.1 frames a packet (a millisecond) */
    int ok = 1, prev = 0, fr = 0;
    for (int i = 0; i + 1 < out_n; i += 2) {
        if (out_seen[i] != out_seen[i + 1]) ok = 0;            /* L == R */
        if (out_seen[i] == 0) continue;                         /* silence */
        if (out_seen[i] < prev) ok = 0;                         /* (equal: a sample repeated for the drift) */
        prev = out_seen[i];
        fr++;
    }
    CHECK(ok, "the mic in order, the same on both sides");
    CHECK(fr > 0 && fr + st[7] <= pushed + pushed / 400 && fr + st[7] >= pushed - 2048 - 441 - pushed / 400,
          "mic out %d + dropped %d vs pushed %d (read 0.2 %% faster or slower at most)", fr, st[7], pushed);
    CHECK(st[5] >= out_packets_frames && st[5] - out_packets_frames <= 4 * 8 * 45, "frames out %d vs %d seen (the rest in flight)",
          st[5], out_packets_frames);
    CHECK(out_packets >= 1000 && (double)out_packets_frames / out_packets > 44.09 && (double)out_packets_frames / out_packets < 44.11,
          "pacing %d frames in %d packets", out_packets_frames, out_packets);

    Java_fm_jiant_stems_UsbNative_stop(env, NULL);
    CHECK(live == 0, "every URB back (%d)", live);
    CHECK(Java_fm_jiant_stems_UsbNative_pump(env, NULL, (jbyteArray)&ab, 5, (jintArray)&as) == -ENODEV, "pump after stop");

    /* the FM-1 unplugged: the stream reports it */
    reaps = 0; unplug_after = 30; qn = 0; live = 0;
    ch = 8;
    CHECK(Java_fm_jiant_stems_UsbNative_start(env, NULL, 3, 0x81, 736, -1, ch * 2) == 0, "restart (STEMS, no OUT)");
    int last = 0;
    for (int call = 0; call < 50; call++) {
        last = Java_fm_jiant_stems_UsbNative_pump(env, NULL, (jbyteArray)&ab, 5, (jintArray)&as);
        if (last < 0) break;
    }
    CHECK(last == -ENODEV, "unplug gives -ENODEV (%d)", last);
    unplug_after = -1;
    Java_fm_jiant_stems_UsbNative_stop(env, NULL);

    /* (1.1) a packet in error with its last frame cut short (6 bytes of a 16-byte frame): only whole frames come out,
     * so every frame after it is still a frame (each channel in its place: the counter runs on in channel order) */
    {
        reaps = 0; qn = 0; live = 0; in_counter = 0; acc = 0; cut_at = 4;
        CHECK(Java_fm_jiant_stems_UsbNative_start(env, NULL, 3, 0x81, 736, -1, ch * 2) == 0, "start (cut frame)");
        int next = 0, aligned = 1, total = 0, skipped = 0;
        for (int call = 0; call < 40; call++) {
            int got = Java_fm_jiant_stems_UsbNative_pump(env, NULL, (jbyteArray)&ab, 5, (jintArray)&as);
            CHECK(got >= 0 && got % (ch * 2) == 0, "whole frames (%d)", got);
            int16_t *s16 = (int16_t *)buf;
            for (int f = 0; f < got / (ch * 2); f++) {
                int v0 = (uint16_t)s16[f * ch];
                for (int c = 1; c < ch; c++) if ((uint16_t)s16[f * ch + c] != (uint16_t)(v0 + c)) aligned = 0;
                if (v0 % ch) aligned = 0;
                if ((uint16_t)v0 != (uint16_t)next) skipped++;
                next = v0 + ch;
                total++;
            }
        }
        CHECK(aligned, "every frame after the cut one whole, each channel in its lane");
        CHECK(skipped == 1 && st[9] == 10 && st[2] == 1, "the cut frame left out (%d jumps, %d bytes cut, %d errors)", skipped, st[9], st[2]);
        cut_at = -1;
        Java_fm_jiant_stems_UsbNative_stop(env, NULL);
        printf("  a frame cut short: left out, %d frames after it whole\n", total);
    }

    /* the drift: ten minutes of OUT URBs (8 ms each) against a mic pushing 128 frames at a time on a clock
     * [ppm] off the USB's; the ring must stay near 10 ms (no drop past 46, no running dry) */
    for (int ppm = -200; ppm <= 200; ppm += 100) {
        memset(&S, 0, sizeof S);
        mr_w = mr_r = 0;
        S.epo = 1;
        static struct { struct usbdevfs_urb u; struct usbdevfs_iso_packet_desc d[OPK]; } urb;
        static int16_t ob[OPK * OMAXP / 2];
        urb.u.buffer = ob;
        double owed = 0, ph = 0, jump_max = 0, step_max = 12000.0 * 2 * 3.14159265 * 300.0 / 44100.0;
        static jshort blk[128];
        uint32_t lo = 1u << 30, hi = 0;
        int16_t prev = 0;
        double corr = 0;
        long ncorr = 0;
        for (long t = 0; t < 600L * 125; t++) {                 /* an URB every 8 ms */
            owed += 8 * 44.1 * (1 + ppm * 1e-6);
            while (owed >= 128) {                               /* (the mic: a 300 Hz sine) */
                for (int i = 0; i < 128; i++, ph += 2 * 3.14159265 * 300.0 / 44100.0) blk[i] = (jshort)(12000.0 * sin(ph));
                arr am = {128, blk};
                Java_fm_jiant_stems_UsbNative_pushMic(env, NULL, (jshortArray)&am, 128);
                owed -= 128;
            }
            fill_out(&urb.u);
            for (int i = 0; t > 1000 && i < (int)urb.u.buffer_length / 4; i++) {   /* (a skip or a repeat: a step 0 or 2x) */
                double d = fabs((double)ob[2 * i] - prev), e = fabs(d - step_max);
                (void)e;
                if (d > jump_max) jump_max = d;
                prev = ob[2 * i];
            }
            if (t <= 1000 && urb.u.buffer_length) prev = ob[2 * (urb.u.buffer_length / 4 - 1)];
            uint32_t fill = mr_w - mr_r;
            if (t > 1000) { if (fill < lo) lo = fill; if (fill > hi) hi = fill; corr += S.slip; ncorr++; }
        }
        corr /= ncorr;
        CHECK(S.drop == 0, "%d ppm: dropped %d", ppm, S.drop);
        CHECK(S.under < 500, "%d ppm: silence %d", ppm, S.under);
        CHECK(lo > 20 && hi < 1000, "%d ppm: the ring between %u and %u frames (after each URB)", ppm, lo, hi);
        CHECK(fabs(corr - ppm) < 30, "%d ppm: read %+.0f ppm faster on average", ppm, corr);
        CHECK(jump_max < 1.2 * step_max + 2, "%d ppm: the mic continuous (largest step %.0f, the sine's %.0f)", ppm, jump_max, step_max);
        printf("  drift %+d ppm: ring %u..%u frames, read %+.0f ppm faster, no sample skipped or repeated (largest step %.0f of %.0f), %d short\n",
               ppm, lo, hi, corr, jump_max, step_max, S.under);
    }

    if (fails) { fprintf(stderr, "%d failed\n", fails); return 1; }
    printf("usbstream: ok (%ld frames in, %d mic frames out)\n", frames, fr);
    return 0;
}

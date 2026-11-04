// SPDX-License-Identifier: GPL-3.0-only
// JIANT Stems: the FM-1's USB audio read (and the phone's mic written) by the app itself, through usbfs on the file
// descriptor UsbDeviceConnection hands over, as JIANT Probe 0.2-0.4 proved on the phone (android/probe/.../usbiso.c):
// Android's audio framework passes only some of a USB device's channels on to apps (an S23 Ultra: two of eight), so
// the app claims the streaming interfaces and does the isochronous transfers itself. The probe read a fixed length
// into one buffer; here the stream stays open: start() submits the URBs, pump() (the recorder's thread, in a loop)
// reaps what came in, copies it out and submits again, and keeps the OUT side (STEMS+IN: the mic, 44.1 stereo
// frames a millisecond) fed from the ring pushMic() fills; stop() takes every URB back.
#include <jni.h>
#include <errno.h>
#include <linux/usbdevice_fs.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>

#define NURB 8              /* IN URBs in flight */
#define NPK 8               /* packets (1 ms each at full speed) an URB */
#define NURB_O 4            /* OUT URBs in flight */
#define OPK 8
#define OMAXP 184           /* 46 stereo frames of 16 bit */

static long long now_us(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (long long)t.tv_sec * 1000000 + t.tv_nsec / 1000;
}

static void naplet(long us)
{
    struct timespec s = {0, us * 1000};
    nanosleep(&s, NULL);
}

/* ---- the mic ring (as the probe's): the mic thread pushes, each OUT packet takes what it holds, kept near 10 ms
 * (more is dropped, less sends silence) */
#define MR_N 32768u
static int16_t mring[MR_N];
static volatile uint32_t mr_w, mr_r;

JNIEXPORT void JNICALL Java_fm_jiant_stems_UsbNative_pushMic(JNIEnv *env, jclass cls, jshortArray a, jint n)
{
    (void)cls;
    jshort *p = (*env)->GetShortArrayElements(env, a, NULL);
    uint32_t w = __atomic_load_n(&mr_w, __ATOMIC_RELAXED);
    for (jint i = 0; i < n; i++)
        mring[(w + (uint32_t)i) & (MR_N - 1u)] = p[i];
    __atomic_store_n(&mr_w, w + (uint32_t)n, __ATOMIC_RELEASE);
    (*env)->ReleaseShortArrayElements(env, a, p, JNI_ABORT);
}

/* ---- the stream */
static struct {
    int on, fd, ep, maxp, epo;
    struct usbdevfs_urb *u[NURB], *uo[NURB_O];
    int live;
    /* stats: packets in, empty, with an error, the last errno, the largest; frames out, out short of mic, mic dropped,
     * mic samples slipped (the drift kept in check) */
    int packets, empty, perr, err, big, sent, under, drop, slip;
    int cut;                    /* (1.1) bytes of a frame cut short in an IN packet, left out (the lanes stay aligned) */
    int fb;                     /* (1.1) the IN stream's bytes a frame (every channel, 16 bit) */
    uint32_t acc;
    int go;
    int32_t avg;                /* the ring's fill, smoothed (frames x 256) */
    uint32_t fr, step;          /* (1.1) the mic read between two of its samples (Q16), and its step a sample */
} S;

/* The phone's mic and the USB schedule run on two clocks: tens of ppm apart, a few frames a second, so over a long
 * take the ring would fill to the 46 ms limit (a jump in the voice's latency) or run dry (silence). The probe's
 * hard limits stay as a backstop; before them the ring is held at its 10 ms. (1.1) By reading the mic a little
 * faster or slower, between its samples (linear: at most 0.2 %, as far from its 10 ms as 3 ms asks), never by a
 * sample skipped or repeated (1.0: one each time the fill strayed 3 ms, a click in the VOX lane), so the round trip
 * measured stays the round trip of the whole take. */
#define TARGET 441
#define BAND 132
#define STEP_MAX 131                /* 0.2 % of 65536 */

static void setup_in(struct usbdevfs_urb *u)
{
    u->type = USBDEVFS_URB_TYPE_ISO;
    u->endpoint = (unsigned char)S.ep;
    u->flags = USBDEVFS_URB_ISO_ASAP;
    u->buffer_length = NPK * S.maxp;
    u->number_of_packets = NPK;
    u->status = 0;
    u->actual_length = 0;
    u->error_count = 0;
    for (int k = 0; k < NPK; k++) {
        u->iso_frame_desc[k].length = (unsigned)S.maxp;
        u->iso_frame_desc[k].actual_length = 0;
        u->iso_frame_desc[k].status = 0;
    }
}

static void fill_out(struct usbdevfs_urb *u)
{
    int16_t *d = u->buffer;
    int total = 0;
    uint32_t r = mr_r, w = __atomic_load_n(&mr_w, __ATOMIC_ACQUIRE);
    if (w - r > 2048u) { S.drop += (int)(w - r - 441u); r = w - 441u; }   /* > 46 ms in hand: back to 10 */
    if (!S.go && w - r >= 441u) { S.go = 1; S.avg = (int32_t)((w - r) << 8); }   /* primed: 10 ms in hand */
    u->type = USBDEVFS_URB_TYPE_ISO;
    u->endpoint = (unsigned char)S.epo;
    u->flags = USBDEVFS_URB_ISO_ASAP;
    u->number_of_packets = OPK;
    u->status = 0;
    u->actual_length = 0;
    u->error_count = 0;
    if (S.go) {                                         /* the drift, once an URB: the read's step from the fill */
        int32_t e = S.avg - (TARGET << 8);
        e = e / 256 * STEP_MAX / BAND;                  /* (3 ms off: the most) */
        if (e > STEP_MAX) e = STEP_MAX;
        if (e < -STEP_MAX) e = -STEP_MAX;
        S.step = 65536u + (uint32_t)e;
        S.slip = e * 1000000 / 65536;                   /* (stats: the correction, ppm) */
    }
    for (int k = 0; k < OPK; k++) {
        int f = 44;                                     /* 44.1 frames a ms: a 45 every tenth */
        S.acc += 100u;
        if (S.acc >= 1000u) { S.acc -= 1000u; f = 45; }
        if (S.go)
            S.avg += ((int32_t)((w - r) << 8) - S.avg) / 128;
        for (int i = 0; i < f; i++) {
            int16_t v = 0;
            if (S.go && w - r >= 2u) {                  /* between the two samples at the read */
                int32_t a = mring[r & (MR_N - 1u)], b = mring[(r + 1u) & (MR_N - 1u)];
                v = (int16_t)(a + (((b - a) * (int32_t)(S.fr >> 2)) >> 14));
                S.fr += S.step ? S.step : 65536u;
                r += S.fr >> 16;
                S.fr &= 0xFFFFu;
            } else if (S.go) {
                S.under++;
            }
            d[2 * (total + i)] = d[2 * (total + i) + 1] = v;   /* the mono mic on both sides */
        }
        u->iso_frame_desc[k].length = (unsigned)(f * 4);
        u->iso_frame_desc[k].actual_length = 0;
        u->iso_frame_desc[k].status = 0;
        total += f;
        S.sent += f;
    }
    u->buffer_length = total * 4;
    mr_r = r;
}

static void free_all(void)
{
    for (int i = 0; i < NURB; i++) if (S.u[i]) { free(S.u[i]->buffer); free(S.u[i]); S.u[i] = NULL; }
    for (int i = 0; i < NURB_O; i++) if (S.uo[i]) { free(S.uo[i]->buffer); free(S.uo[i]); S.uo[i] = NULL; }
}

/* takes every URB back (discard, then reap until none is left or half a second passes) */
static void drain(void)
{
    for (int i = 0; i < NURB; i++) if (S.u[i]) ioctl(S.fd, USBDEVFS_DISCARDURB, S.u[i]);
    for (int i = 0; i < NURB_O; i++) if (S.uo[i]) ioctl(S.fd, USBDEVFS_DISCARDURB, S.uo[i]);
    long long stop = now_us() + 500000;
    while (S.live > 0 && now_us() < stop) {
        struct usbdevfs_urb *r = NULL;
        if (ioctl(S.fd, USBDEVFS_REAPURBNDELAY, &r) == 0) { S.live--; continue; }
        if (errno != EAGAIN && errno != EINTR) break;
        naplet(1000);
    }
    if (S.live == 0) {
        free_all();
    } else {                                            /* leak them: the kernel may still write into them */
        for (int i = 0; i < NURB; i++) S.u[i] = NULL;
        for (int i = 0; i < NURB_O; i++) S.uo[i] = NULL;
    }
    S.live = 0;
}

/* starts the stream: the IN endpoint ep (packets of maxp bytes, frames of fb bytes) and, when epo >= 0, the OUT one.
 * 0, or -errno */
JNIEXPORT jint JNICALL Java_fm_jiant_stems_UsbNative_start(JNIEnv *env, jclass cls, jint fd, jint ep, jint maxp, jint epo, jint fb)
{
    (void)env; (void)cls;
    if (S.on) return -EBUSY;
    memset(&S, 0, sizeof S);
    S.fd = fd; S.ep = ep; S.maxp = maxp; S.epo = epo; S.fb = fb > 0 ? fb : 1;
    S.step = 65536u;
    mr_r = __atomic_load_n(&mr_w, __ATOMIC_ACQUIRE);    /* (no mic from before) */
    size_t sz = sizeof(struct usbdevfs_urb) + NPK * sizeof(struct usbdevfs_iso_packet_desc);
    size_t szo = sizeof(struct usbdevfs_urb) + OPK * sizeof(struct usbdevfs_iso_packet_desc);
    for (int i = 0; i < NURB; i++) {
        S.u[i] = calloc(1, sz);
        if (!S.u[i] || !(S.u[i]->buffer = malloc((size_t)NPK * maxp))) { free_all(); return -ENOMEM; }
    }
    if (epo >= 0)
        for (int i = 0; i < NURB_O; i++) {
            S.uo[i] = calloc(1, szo);
            if (!S.uo[i] || !(S.uo[i]->buffer = malloc((size_t)OPK * OMAXP))) { free_all(); return -ENOMEM; }
        }
    int err = 0;
    for (int i = 0; i < NURB && !err; i++) {
        setup_in(S.u[i]);
        if (ioctl(fd, USBDEVFS_SUBMITURB, S.u[i]) < 0) err = errno; else S.live++;
    }
    for (int i = 0; epo >= 0 && i < NURB_O && !err; i++) {
        fill_out(S.uo[i]);
        if (ioctl(fd, USBDEVFS_SUBMITURB, S.uo[i]) < 0) err = errno; else S.live++;
    }
    if (err) { drain(); return -err; }
    S.on = 1;
    return 0;
}

/* what came in since the last call, into out (as the device sends it: whole frames, the channels interleaved), for
 * up to timeout_ms or until out could not take another URB; the OUT side kept fed meanwhile. stats (10): packets,
 * empty, errors, errno, largest packet, frames out, out frames short of mic, mic frames dropped, the mic's drift
 * correction (ppm; 1.0: samples slipped), bytes of cut frames left out. Returns the bytes, or -errno when the stream
 * broke (the FM-1 unplugged) */
JNIEXPORT jint JNICALL Java_fm_jiant_stems_UsbNative_pump(JNIEnv *env, jclass cls, jbyteArray out, jint timeout_ms, jintArray stats)
{
    (void)cls;
    if (!S.on) return -ENODEV;
    jsize cap = (*env)->GetArrayLength(env, out);
    jbyte *dst = (*env)->GetByteArrayElements(env, out, NULL);
    int got = 0;
    long long end = now_us() + (long long)timeout_ms * 1000;
    while (!S.err && cap - got >= NPK * S.maxp) {
        struct usbdevfs_urb *r = NULL;
        if (ioctl(S.fd, USBDEVFS_REAPURBNDELAY, &r) < 0) {
            if (errno == EAGAIN) {
                if (now_us() >= end) break;
                naplet(500);
                continue;
            }
            if (errno == EINTR) continue;
            S.err = errno;
            break;
        }
        S.live--;
        if (S.epo >= 0 && r->endpoint == (unsigned char)S.epo) {    /* an OUT URB back: the next from the ring */
            fill_out(r);
            if (ioctl(S.fd, USBDEVFS_SUBMITURB, r) < 0) { S.err = errno; break; }
            S.live++;
            continue;
        }
        const char *b = r->buffer;
        for (int k = 0; k < NPK; k++) {
            int n = (int)r->iso_frame_desc[k].actual_length;
            S.packets++;
            if (r->iso_frame_desc[k].status) S.perr++;
            if (!n) { S.empty++; continue; }
            if (n > S.big) S.big = n;
            if (n % S.fb) { S.cut += n % S.fb; n -= n % S.fb; }   /* (1.1) whole frames only: a frame cut short (a
                                                    * packet in error) shifted every channel of every frame after it */
            memcpy(dst + got, b + k * S.maxp, (size_t)n);   /* iso packets sit at their requested offsets */
            got += n;
        }
        setup_in(r);
        if (ioctl(S.fd, USBDEVFS_SUBMITURB, r) < 0) { S.err = errno; break; }
        S.live++;
    }
    (*env)->ReleaseByteArrayElements(env, out, dst, 0);
    jint st[10] = {S.packets, S.empty, S.perr, S.err, S.big, S.sent, S.under, S.drop, S.slip, S.cut};
    jsize ns = (*env)->GetArrayLength(env, stats);
    (*env)->SetIntArrayRegion(env, stats, 0, ns < 10 ? ns : 10, st);
    if (got) return got;
    return S.err ? -S.err : 0;
}

/* every URB back; the stream closed */
JNIEXPORT void JNICALL Java_fm_jiant_stems_UsbNative_stop(JNIEnv *env, jclass cls)
{
    (void)env; (void)cls;
    if (!S.on) return;
    drain();
    S.on = 0;
}

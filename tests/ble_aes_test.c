/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) */
/* (V10.32) Host test of the BLE stack's AES-128, LE legacy pairing's c1 / s1 and the LL's AES-CCM
 * (firmware/bt/ble/ble_aes.c, Optimist's, from charlesvestal/fm1-base 4039a8b), which Just Works pairing brings into
 * the Bluetooth segment (bt_seg.c FELUCCA_BLE_BOND):
 *   AES-128   FIPS-197 appendix C.1
 *   c1, s1    Core Specification Vol 3 Part H appendix D (sample data: k = 0)
 *   CCM       encrypt then decrypt round trip, payloads 1..27; a flipped bit or a wrong counter refused */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define BLE_API static
#define BLE_SMP_LEGACY 1
#define BLE_LL_ENC 1
#include "../firmware/bt/ble/ble_aes.c"

static int fails;
static void check(const char *what, int ok)
{
    printf("%-72s %s\n", what, ok ? "ok" : "FAIL");
    fails += !ok;
}
static void hex(const char *s, uint8_t *b, int rev)   /* most significant first; rev: stored least significant first */
{
    int n = (int)strlen(s) / 2, i;
    for (i = 0; i < n; i++) {
        unsigned v;
        sscanf(s + 2 * i, "%2x", &v);
        b[rev ? n - 1 - i : i] = (uint8_t)v;
    }
}
int main(void)
{
    uint8_t k[16], in[16], out[16], want[16], r[16], preq[7], pres[7], ia[6], ra[6], r2[16];
    hex("000102030405060708090a0b0c0d0e0f", k, 0);
    hex("00112233445566778899aabbccddeeff", in, 0);
    hex("69c4e0d86a7b0430d8cdb78070b4c55a", want, 0);
    ble_aes128(k, in, out);
    check("AES-128: FIPS-197 C.1", !memcmp(out, want, 16));
    memset(k, 0, 16);
    hex("5783D52156AD6F0E6388274EC6702EE0", r, 1);
    hex("07071000000101", preq, 1);
    hex("05000800000302", pres, 1);
    hex("A1A2A3A4A5A6", ia, 1);
    hex("B1B2B3B4B5B6", ra, 1);
    hex("1e1e3fef878988ead2a74dc5bef13b86", want, 1);
    ble_smp_c1(k, r, preq, pres, 1, ia, 0, ra, out);
    check("c1: Core Vol 3 Part H D.2's sample", !memcmp(out, want, 16));
    hex("000F0E0D0C0B0A091122334455667788", r, 1);
    hex("010203040506070899AABBCCDDEEFF00", r2, 1);
    hex("9a1fe1f0e8b0f49b5b4216ae796da062", want, 1);
    ble_smp_s1(k, r, r2, out);
    check("s1: Core Vol 3 Part H D.3's sample", !memcmp(out, want, 16));
    {
        struct ble_ccm a, b;
        uint8_t p[32], q[32];
        int len, ok = 1;
        memset(&a, 0, sizeof a);
        hex("99AD1B5226A37E3E058E3B8E27C2C666", a.key, 0);
        hex("24ABDCBABEBAAFDE", a.iv, 0);
        a.dir = 1;
        b = a;
        for (len = 1; len <= 27; len++) {
            int i;
            for (i = 0; i < len; i++) p[i] = q[i] = (uint8_t)(i * 37 + len);
            ble_ccm_encrypt(&a, 0x02, p, (uint8_t)len);
            ok &= memcmp(p, q, (size_t)len) != 0 || len < 2;
            ok &= ble_ccm_decrypt(&b, 0x02, p, (uint8_t)(len + 4)) && !memcmp(p, q, (size_t)len);
        }
        check("CCM: encrypt then decrypt, payloads 1..27, the counters in step", ok);
        memcpy(p, "MIDI", 4);
        ble_ccm_encrypt(&a, 0x02, p, 4);
        p[1] ^= 4u;
        ok = !ble_ccm_decrypt(&b, 0x02, p, 8);
        memcpy(p, "MIDI", 4);
        a.ctr++;
        ble_ccm_encrypt(&a, 0x02, p, 4);
        ok &= !ble_ccm_decrypt(&b, 0x02, p, 8);
        check("CCM: a flipped bit, a counter out of step: refused", ok);
    }
    printf("ble_aes: %s\n", fails ? "FAILED" : "all ok");
    return fails != 0;
}

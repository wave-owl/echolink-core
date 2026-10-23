/*
 * bench_elk: CPU time of the GSM encoder and decoder per second of speech (8 kHz).
 * Copyright (C) 2026 Gerolf Ziegenhain. GPL-3.0-or-later.
 */
#include <time.h>

#include "echolink_core.h"
#include "vectors_main.h"

/* vectors_main.h hooks are unused here. */
static void *impl_gsm_new(void) { return NULL; }
static void impl_gsm_free(void *st) { (void)st; }
static void impl_gsm_encode(void *st, const int16_t *pcm, uint8_t *frame)
{ (void)st; (void)pcm; (void)frame; }
static void impl_gsm_decode(void *st, const uint8_t *frame, int16_t *pcm)
{ (void)st; (void)frame; (void)pcm; }
static int impl_voice_build(void *e, uint16_t s, const int16_t *p, uint8_t *o)
{ (void)e; (void)s; (void)p; (void)o; return 0; }
static int impl_sdes(const char *c, const char *n, const char *p, uint8_t *o)
{ (void)c; (void)n; (void)p; (void)o; return 0; }
static int impl_bye(uint8_t *o) { (void)o; return 0; }
static int impl_is_sdes(const uint8_t *p, int l) { (void)p; (void)l; return 0; }
static int impl_is_bye(const uint8_t *p, int l) { (void)p; (void)l; return 0; }
static int impl_sdes_item(const uint8_t *p, int l, int i, char *o)
{ (void)p; (void)l; (void)i; (void)o; return 0; }
static void impl_sanitize(const char *i, char *o) { (void)i; (void)o; }
static int impl_chat(const char *c, const char *t, uint8_t *o) { (void)c; (void)t; (void)o; return 0; }
static int impl_info(const char *i, uint8_t *o) { (void)i; (void)o; return 0; }
static int impl_data_parse(const uint8_t *p, int l, char *o) { (void)p; (void)l; (void)o; return 0; }

int main(void)
{
    static int16_t pcm[V_SAMPLES], out[V_SAMPLES];
    static uint8_t pkt[(V_SAMPLES / ELK_PACKET_SAMPLES) * ELK_VOICE_PACKET_BYTES];
    const int rounds = 20;
    const int packets = V_SAMPLES / ELK_PACKET_SAMPLES;
    elk_gsm *enc = elk_gsm_new(), *dec = elk_gsm_new();
    clock_t t0, t1, t2;
    int r, i;
    double seconds = (double)rounds * V_SAMPLES / V_RATE;
    (void)v_main;
    v_signal(pcm, V_SAMPLES);
    t0 = clock();
    for (r = 0; r < rounds; r++) {
        for (i = 0; i < packets; i++) {
            elk_voice_build(enc, (uint16_t)i, pcm + i * ELK_PACKET_SAMPLES,
                            pkt + i * ELK_VOICE_PACKET_BYTES, ELK_VOICE_PACKET_BYTES);
        }
    }
    t1 = clock();
    for (r = 0; r < rounds; r++) {
        for (i = 0; i < packets; i++) {
            elk_voice_parse(dec, pkt + i * ELK_VOICE_PACKET_BYTES, ELK_VOICE_PACKET_BYTES,
                            out + i * ELK_PACKET_SAMPLES, NULL);
        }
    }
    t2 = clock();
    printf("bench_elk: encode %.3f ms CPU per s of speech, decode %.3f ms per s (%s)\n",
           1000.0 * (double)(t1 - t0) / CLOCKS_PER_SEC / seconds,
           1000.0 * (double)(t2 - t1) / CLOCKS_PER_SEC / seconds, elk_version());
    elk_gsm_free(enc);
    elk_gsm_free(dec);
    return 0;
}

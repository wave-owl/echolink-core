/*
 * echolink-core unit tests: API contract, round trips, error codes, robustness.
 * The bit-exact conformance against libgsm and svxlink is the golden comparison of
 * `make test` (tests/golden/, written by the reference harness, `make ref`).
 * Copyright (C) 2026 Gerolf Ziegenhain. GPL-3.0-or-later.
 */
#include "echolink_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;

#define CHECK(cond) do { if (!(cond)) { \
    fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
    failures++; } } while (0)

static uint32_t lcg(uint32_t *s)
{
    *s = *s * 1664525u + 1013904223u;
    return *s;
}

static void test_version(void)
{
    const char *v = elk_version();
    CHECK(strstr(v, "contract=echolink-c1") != NULL);
    CHECK(strstr(v, " abi=1") != NULL);
    CHECK(strstr(v, " tx=1") != NULL);
    CHECK(strstr(v, " gsm=1.0.24") != NULL);
}

static void test_voice_roundtrip(void)
{
    elk_gsm *enc = elk_gsm_new(), *dec = elk_gsm_new();
    int16_t pcm[ELK_PACKET_SAMPLES], out[ELK_PACKET_SAMPLES];
    uint8_t pkt[ELK_VOICE_PACKET_BYTES];
    uint16_t seq = 0;
    int i, n;
    long long energy_in = 0, energy_err = 0;
    for (i = 0; i < ELK_PACKET_SAMPLES; i++) {
        pcm[i] = (int16_t)((i % 40 < 20 ? 1 : -1) * 4000);
    }
    /* settle the predictors over a few packets, then measure */
    for (i = 0; i < 6; i++) {
        n = elk_voice_build(enc, (uint16_t)(0xfffe + i), pcm, pkt, sizeof(pkt));
        CHECK(n == ELK_VOICE_PACKET_BYTES);
        CHECK(elk_voice_parse(dec, pkt, (size_t)n, out, &seq) == ELK_PACKET_SAMPLES);
        CHECK(seq == (uint16_t)(0xfffe + i));
    }
    CHECK(pkt[0] == 0xc0 && pkt[1] == 0x03);
    CHECK(pkt[4] == 0 && pkt[7] == 0 && pkt[8] == 0 && pkt[11] == 0);
    CHECK((pkt[12] >> 4) == 0xd);
    for (i = 0; i < ELK_PACKET_SAMPLES; i++) {
        energy_in += (long long)pcm[i] * pcm[i];
        energy_err += (long long)(out[i] - pcm[i]) * (out[i] - pcm[i]);
    }
    CHECK(energy_in > 0 && energy_err < energy_in);  /* a lossy codec, but the signal survives */
    elk_gsm_free(enc);
    elk_gsm_free(dec);
}

static void test_voice_errors(void)
{
    elk_gsm *st = elk_gsm_new();
    int16_t pcm[ELK_PACKET_SAMPLES] = {0};
    uint8_t pkt[ELK_VOICE_PACKET_BYTES];
    CHECK(elk_voice_build(st, 1, pcm, pkt, sizeof(pkt) - 1) == ELK_ERR_SPACE);
    CHECK(elk_voice_build(NULL, 1, pcm, pkt, sizeof(pkt)) == ELK_ERR_ARG);
    CHECK(elk_voice_build(st, 1, pcm, pkt, sizeof(pkt)) == ELK_VOICE_PACKET_BYTES);
    CHECK(elk_voice_parse(st, pkt, 11, pcm, NULL) == ELK_ERR_SHORT);
    CHECK(elk_voice_parse(st, pkt, 143, pcm, NULL) == ELK_ERR_SHORT);
    pkt[1] = ELK_PT_SPEEX;
    CHECK(elk_voice_parse(st, pkt, sizeof(pkt), pcm, NULL) == ELK_ERR_CODEC);
    pkt[1] = ELK_PT_GSM;
    pkt[0] = 0x80;
    CHECK(elk_voice_parse(st, pkt, sizeof(pkt), pcm, NULL) == ELK_ERR_MALFORMED);
    pkt[0] = 0xc0;
    pkt[12] = 0x00;  /* bad magic in frame 0: silence for that frame, the rest decodes */
    CHECK(elk_voice_parse(st, pkt, sizeof(pkt), pcm, NULL) == ELK_PACKET_SAMPLES);
    CHECK(pcm[0] == 0 && pcm[159] == 0);
    CHECK(elk_gsm_reset(st) == ELK_OK);
    elk_gsm_free(st);
    elk_gsm_free(NULL);
}

static void test_sdes_bye(void)
{
    uint8_t pkt[ELK_PACKET_MAX];
    char item[ELK_SDES_MAX];
    char longname[300];
    int n = elk_sdes_build("DG6FL-L", "Mainz", NULL, pkt, sizeof(pkt));
    CHECK(n > 0 && n % 4 == 0);
    CHECK(pkt[0] == 0xc0 && pkt[1] == ELK_RTCP_RR && pkt[8] == 0xc1 && pkt[9] == ELK_RTCP_SDES);
    CHECK(elk_is_sdes(pkt, (size_t)n) == 1);
    CHECK(elk_is_bye(pkt, (size_t)n) == 0);
    CHECK(elk_classify(pkt, (size_t)n, 1) == ELK_KIND_SDES);
    CHECK(elk_sdes_item(pkt, (size_t)n, ELK_SDES_NAME, item, sizeof(item)) == 20);
    CHECK(strcmp(item, "DG6FL-L        Mainz") == 0);
    CHECK(elk_sdes_item(pkt, (size_t)n, ELK_SDES_CNAME, item, sizeof(item)) == 8);
    CHECK(elk_sdes_item(pkt, (size_t)n, ELK_SDES_PRIV, item, sizeof(item)) == ELK_ERR_MALFORMED);
    CHECK(elk_sdes_item(pkt, (size_t)n, ELK_SDES_NAME, item, 10) == ELK_ERR_ARG);
    CHECK(elk_sdes_build("DG6FL", "x", NULL, pkt, 20) == ELK_ERR_SPACE);
    memset(longname, 'a', sizeof(longname) - 1);
    longname[sizeof(longname) - 1] = 0;
    CHECK(elk_sdes_build("DG6FL", longname, NULL, pkt, sizeof(pkt)) == ELK_ERR_ARG);
    longname[240] = 0;  /* 15 + 240 = 255 still fits one item */
    CHECK(elk_sdes_build("DG6FL", longname, NULL, pkt, sizeof(pkt)) > 0);
    longname[240] = 'a';
    longname[241] = 0;  /* 256 bytes: one too many */
    CHECK(elk_sdes_build("DG6FL", longname, NULL, pkt, sizeof(pkt)) == ELK_ERR_ARG);

    n = elk_bye_build(pkt, sizeof(pkt));
    CHECK(n == 24);
    CHECK(elk_is_bye(pkt, (size_t)n) == 1);
    CHECK(elk_is_sdes(pkt, (size_t)n) == 0);
    CHECK(elk_classify(pkt, (size_t)n, 1) == ELK_KIND_BYE);
    CHECK(elk_bye_build(pkt, 23) == ELK_ERR_SPACE);
}

static void test_data(void)
{
    uint8_t pkt[ELK_PACKET_MAX];
    char text[ELK_PACKET_MAX];
    int n = elk_chat_build("DG6FL", "Hallo", pkt, sizeof(pkt));
    CHECK(n == 6 + 5 + 1 + 5 + 2 + 1);
    CHECK(memcmp(pkt, "oNDATADG6FL>Hallo\r\n", 20) == 0);
    CHECK(elk_classify(pkt, (size_t)n, 0) == ELK_KIND_CHAT);
    CHECK(elk_data_parse(pkt, (size_t)n, text, sizeof(text)) == ELK_KIND_CHAT);
    CHECK(strcmp(text, "DG6FL>Hallo\n\n") == 0);
    n = elk_info_build("a\nb", pkt, sizeof(pkt));
    CHECK(n == 11 && memcmp(pkt, "oNDATA\ra\rb", 11) == 0);
    CHECK(elk_classify(pkt, (size_t)n, 0) == ELK_KIND_INFO);
    CHECK(elk_data_parse(pkt, (size_t)n, text, sizeof(text)) == ELK_KIND_INFO);
    CHECK(strcmp(text, "a\nb") == 0);
    CHECK(elk_data_parse(pkt, 6, text, sizeof(text)) == ELK_ERR_SHORT);
    CHECK(elk_data_parse(pkt, (size_t)n - 1, text, sizeof(text)) == ELK_ERR_MALFORMED);
    CHECK(elk_data_parse(pkt, (size_t)n, text, 2) == ELK_ERR_SPACE);
    pkt[2] = 'X';
    CHECK(elk_data_parse(pkt, (size_t)n, text, sizeof(text)) == ELK_ERR_MALFORMED);
    CHECK(elk_classify(pkt, (size_t)n, 0) == ELK_KIND_NONE);
    CHECK(elk_chat_build("DG6FL", "x", pkt, 5) == ELK_ERR_SPACE);
}

static void test_sanitize(void)
{
    char out[64];
    CHECK(elk_sanitize_callsign("dg6fl-L <x>", out, sizeof(out)) == 8);
    CHECK(strcmp(out, "dg6fl-Lx") == 0);
    CHECK(elk_sanitize_callsign("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789", out, sizeof(out)) == 32);
    CHECK(elk_sanitize_callsign("ABCDEFG", out, 4) == 3);
    CHECK(strcmp(out, "ABC") == 0);
}

/* Random datagrams through every parser and the decoder: no crash, no out-of-bounds read
 * (meaningful under `make sanitize`), no libgsm assertion. */
static void test_fuzz(void)
{
    elk_gsm *st = elk_gsm_new();
    uint32_t seed = 4242u;
    uint8_t *pkt;
    char text[ELK_PACKET_MAX];
    char item[ELK_SDES_MAX];
    int16_t pcm[ELK_PACKET_SAMPLES];
    int i, k;
    for (i = 0; i < 20000; i++) {
        size_t len = (size_t)(lcg(&seed) >> 22);  /* 0..1023 */
        pkt = (uint8_t *)malloc(len ? len : 1);   /* exact size: ASan sees overreads */
        for (k = 0; k < (int)len; k++) pkt[k] = (uint8_t)(lcg(&seed) >> 24);
        if (len > 1 && (i & 1)) { pkt[0] = 0xc0; }
        if (len > 1 && (i % 3 == 0)) { pkt[1] = (uint8_t)(i % 6 ? 0xc9 : 0xc8); }
        (void)elk_classify(pkt, len, i & 1);
        (void)elk_is_sdes(pkt, len);
        (void)elk_is_bye(pkt, len);
        (void)elk_sdes_item(pkt, len, ELK_SDES_NAME, item, sizeof(item));
        (void)elk_data_parse(pkt, len, text, sizeof(text));
        (void)elk_voice_parse(st, pkt, len, pcm, NULL);
        free(pkt);
    }
    elk_gsm_free(st);
}

int main(void)
{
    test_version();
    test_voice_roundtrip();
    test_voice_errors();
    test_sdes_bye();
    test_data();
    test_sanitize();
    test_fuzz();
    if (failures) {
        fprintf(stderr, "test_elk: %d failure(s)\n", failures);
        return 1;
    }
    printf("test_elk: all tests passed\n");
    return 0;
}

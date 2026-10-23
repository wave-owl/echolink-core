/*
 * One vector generator for two implementations: the core (tools/elk_tool.c) and the
 * reference (tools/ref/ref_tool.cpp: unmodified libgsm 1.0.24 and svxlink 26.05.2
 * rtpacket.cpp plus the EchoLinkQso.cpp packet code). Both define the impl_* functions
 * below and include this file; equal output files mean bit-identical behaviour.
 *
 * Files written into <dir>:
 *   gsm_frames.bin        encoder output for the test signal (400 frames x 33 bytes)
 *   gsm_decoded.pcm       decoder output of those frames (int16 little endian)
 *   gsm_fuzz_decoded.pcm  decoder output of 200 pseudo-random frames with the 0xD magic
 *   voice.bin             100 voice packets (seq 65500.., wraps) built from the test signal
 *   packets.txt           SDES/BYE/chat/info packets, the parsers on them and on mutations
 *
 * Copyright (C) 2026 Gerolf Ziegenhain. GPL-3.0-or-later.
 */
#ifndef VECTORS_MAIN_H
#define VECTORS_MAIN_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define V_RATE 8000
#define V_FRAMES 400
#define V_SAMPLES (V_FRAMES * 160)
#define V_FUZZ 200
#define V_PACKETS 100

/* Deterministic test signal: a voiced, amplitude-modulated harmonic series with a moving
 * pitch, LCG noise, 0.5 s of silence and a clipped burst. Integer arithmetic only, so
 * every build on every platform produces the same samples. */
static uint32_t v_lcg(uint32_t *s)
{
    *s = *s * 1664525u + 1013904223u;
    return *s;
}

static void v_signal(int16_t *out, int n)
{
    static int16_t table[1024];
    uint32_t seed = 12345u;
    uint32_t phase = 0;
    int i;
    for (i = 0; i < 1024; i++) {
        /* Bhaskara I's sine approximation in integers: no libm, equal on every platform. */
        int64_t x = i % 512;
        int64_t num = 16 * x * (512 - x);
        int64_t den = 5 * 512 * 512 - 4 * x * (512 - x);
        int32_t val = (int32_t)(8000 * num / den);
        table[i] = (int16_t)(i < 512 ? val : -val);
    }
    for (i = 0; i < n; i++) {
        int seg = i / 4000;                       /* 0.5 s segments */
        uint32_t pitch = 120u + (uint32_t)((i / 80) % 100);   /* Hz */
        int32_t v = 0;
        int h;
        phase += (uint32_t)(((uint64_t)pitch << 26) / V_RATE);   /* 2^26 = one cycle */
        for (h = 1; h <= 6; h++) {
            v += table[((phase >> 16) * (uint32_t)h) & 1023] / h;
        }
        v = v * (int32_t)((i % 1600) < 1200 ? 3 : 1) / 3;   /* syllable envelope */
        v += (int32_t)(v_lcg(&seed) >> 22) - 512;           /* noise */
        if (seg == 3) {
            v = 0;                                          /* silence */
        } else if (seg == 6) {
            v *= 4;                                         /* clipped burst */
        }
        if (v > 32767) v = 32767;
        if (v < -32768) v = -32768;
        out[i] = (int16_t)v;
    }
}

static void v_hex(FILE *f, const char *tag, const uint8_t *p, int n)
{
    int i;
    fprintf(f, "%s %d ", tag, n);
    for (i = 0; i < n; i++) {
        fprintf(f, "%02x", p[i]);
    }
    fputc('\n', f);
}

static FILE *v_open(const char *dir, const char *name)
{
    char path[1024];
    FILE *f;
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    f = fopen(path, "wb");
    if (!f) {
        perror(path);
        exit(2);
    }
    return f;
}

static void v_put16(FILE *f, const int16_t *s, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        uint16_t u = (uint16_t)s[i];
        fputc(u & 0xff, f);
        fputc(u >> 8, f);
    }
}

/* Implementation hooks. */
static void *impl_gsm_new(void);
static void impl_gsm_free(void *st);
static void impl_gsm_encode(void *st, const int16_t *pcm, uint8_t *frame);
static void impl_gsm_decode(void *st, const uint8_t *frame, int16_t *pcm);
static int impl_voice_build(void *enc, uint16_t seq, const int16_t *pcm, uint8_t *out);
static int impl_sdes(const char *call, const char *name, const char *priv, uint8_t *out);
static int impl_bye(uint8_t *out);
static int impl_is_sdes(const uint8_t *p, int len);
static int impl_is_bye(const uint8_t *p, int len);
static int impl_sdes_item(const uint8_t *p, int len, int item, char *out);
static void impl_sanitize(const char *in, char *out);
static int impl_chat(const char *call, const char *text, uint8_t *out);
static int impl_info(const char *info, uint8_t *out);
/* 2 chat, 3 info, <0 rejected; text into out. */
static int impl_data_parse(const uint8_t *p, int len, char *out);

static const char *const SDES_CASES[][3] = {
    {"DG6FL", "Gerolf", NULL},
    {"DG6FL-L", "Mainz JO40DA", NULL},
    {"DB0ABC-R", "Relais", "SPEEX"},
    {"*ECHOTEST*", "", NULL},
    {"SM0SVX", "Tobias Blomberg", "SPEEX"},
    {"A", "", ""},
    {"VERYLONGCALLSIGN-X", "Name longer than fifteen", "priv"},
};

static const char *const CALL_CASES[] = {
    "DG6FL", "dg6fl-l", "DB0ABC-R", "*ECHOTEST*", "OE/DG6FL/P", "D G 6 F L!",
    "TOOLONG_CALLSIGN_ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789", "<script>", "",
};

static const char *const CHAT_CASES[][2] = {
    {"DG6FL", "Hallo aus Mainz"},
    {"DG6FL", ""},
    {"DG6FL", "line1\nline2"},
};

static const char *const INFO_CASES[] = {
    "Wave Owl EchoLink\nDG6FL Mainz",
    "",
    "Conference *TEST*\nStations connected:\nDG6FL\n",
};

static void v_parse_report(FILE *f, const char *tag, const uint8_t *p, int len)
{
    char text[1100];
    char item[300];
    int kind, i;
    static const int items[] = {1, 2, 3, 4, 8};
    fprintf(f, "%s is_sdes=%d is_bye=%d", tag, impl_is_sdes(p, len), impl_is_bye(p, len));
    for (i = 0; i < 5; i++) {
        int n = impl_sdes_item(p, len, items[i], item);
        fprintf(f, " item%d=", items[i]);
        if (n < 0) {
            fprintf(f, "-");
        } else {
            fprintf(f, "%d:", n);
            {
                int k;
                for (k = 0; k < n; k++) fprintf(f, "%02x", (uint8_t)item[k]);
            }
        }
    }
    kind = impl_data_parse(p, len, text);
    fprintf(f, " data=%d", kind < 0 ? -1 : kind);
    if (kind > 0) {
        size_t k;
        fputc(':', f);
        for (k = 0; k < strlen(text); k++) fprintf(f, "%02x", (uint8_t)text[k]);
    }
    fputc('\n', f);
}

static void v_mutations(FILE *f, const char *tag, const uint8_t *p, int len)
{
    uint8_t m[1100];
    int cut;
    char t[64];
    /* truncations */
    for (cut = 0; cut <= len && cut < 40; cut += 3) {
        snprintf(t, sizeof(t), "%s-cut%d", tag, cut);
        v_parse_report(f, t, p, cut);
    }
    /* bit flips in the headers */
    for (cut = 0; cut < len && cut < 20; cut++) {
        memcpy(m, p, (size_t)len);
        m[cut] ^= 0xa5;
        snprintf(t, sizeof(t), "%s-flip%d", tag, cut);
        v_parse_report(f, t, m, len);
    }
}

static int v_main(int argc, char **argv)
{
    const char *dir;
    static int16_t pcm[V_SAMPLES];
    static uint8_t frames[V_FRAMES * 33];
    static int16_t dec[V_SAMPLES];
    uint8_t pkt[1100];
    uint32_t seed = 777u;
    void *enc, *dsta;
    FILE *f;
    int i, n;
    size_t c;
    if (argc != 2) {
        fprintf(stderr, "usage: %s <outdir>\n", argv[0]);
        return 2;
    }
    dir = argv[1];
    v_signal(pcm, V_SAMPLES);

    enc = impl_gsm_new();
    dsta = impl_gsm_new();
    for (i = 0; i < V_FRAMES; i++) {
        impl_gsm_encode(enc, pcm + i * 160, frames + i * 33);
        impl_gsm_decode(dsta, frames + i * 33, dec + i * 160);
    }
    f = v_open(dir, "gsm_frames.bin");
    fwrite(frames, 1, sizeof(frames), f);
    fclose(f);
    f = v_open(dir, "gsm_decoded.pcm");
    v_put16(f, dec, V_SAMPLES);
    fclose(f);
    impl_gsm_free(enc);
    impl_gsm_free(dsta);

    dsta = impl_gsm_new();
    f = v_open(dir, "gsm_fuzz_decoded.pcm");
    for (i = 0; i < V_FUZZ; i++) {
        uint8_t fr[33];
        int16_t out[160];
        int k;
        for (k = 0; k < 33; k++) fr[k] = (uint8_t)(v_lcg(&seed) >> 24);
        fr[0] = (uint8_t)(0xd0 | (fr[0] & 0x0f));
        impl_gsm_decode(dsta, fr, out);
        v_put16(f, out, 160);
    }
    fclose(f);
    impl_gsm_free(dsta);

    enc = impl_gsm_new();
    f = v_open(dir, "voice.bin");
    for (i = 0; i < V_PACKETS; i++) {
        n = impl_voice_build(enc, (uint16_t)(65500 + i), pcm + i * 640, pkt);
        fwrite(pkt, 1, (size_t)n, f);
    }
    fclose(f);
    impl_gsm_free(enc);

    f = v_open(dir, "packets.txt");
    for (c = 0; c < sizeof(SDES_CASES) / sizeof(SDES_CASES[0]); c++) {
        char tag[32];
        n = impl_sdes(SDES_CASES[c][0], SDES_CASES[c][1], SDES_CASES[c][2], pkt);
        snprintf(tag, sizeof(tag), "sdes%d", (int)c);
        v_hex(f, tag, pkt, n);
        v_parse_report(f, tag, pkt, n);
        v_mutations(f, tag, pkt, n);
    }
    n = impl_bye(pkt);
    v_hex(f, "bye", pkt, n);
    v_parse_report(f, "bye", pkt, n);
    v_mutations(f, "bye", pkt, n);
    for (c = 0; c < sizeof(CALL_CASES) / sizeof(CALL_CASES[0]); c++) {
        char out[300];
        impl_sanitize(CALL_CASES[c], out);
        fprintf(f, "call%d %s\n", (int)c, out);
    }
    for (c = 0; c < sizeof(CHAT_CASES) / sizeof(CHAT_CASES[0]); c++) {
        char tag[32];
        n = impl_chat(CHAT_CASES[c][0], CHAT_CASES[c][1], pkt);
        snprintf(tag, sizeof(tag), "chat%d", (int)c);
        v_hex(f, tag, pkt, n);
        v_parse_report(f, tag, pkt, n);
        v_mutations(f, tag, pkt, n);
    }
    for (c = 0; c < sizeof(INFO_CASES) / sizeof(INFO_CASES[0]); c++) {
        char tag[32];
        n = impl_info(INFO_CASES[c], pkt);
        snprintf(tag, sizeof(tag), "info%d", (int)c);
        v_hex(f, tag, pkt, n);
        v_parse_report(f, tag, pkt, n);
        v_mutations(f, tag, pkt, n);
    }
    /* random datagrams */
    for (i = 0; i < 300; i++) {
        char tag[32];
        int len = (int)(v_lcg(&seed) >> 25);   /* 0..127 */
        int k;
        for (k = 0; k < len; k++) pkt[k] = (uint8_t)(v_lcg(&seed) >> 24);
        if (i % 3 == 0 && len > 2) { pkt[0] = 0xc0; pkt[1] = (uint8_t)(i & 1 ? 0xc9 : 0xc8); }
        if (i % 5 == 0 && len > 7) { memcpy(pkt + 1, "NDATA", 5); }
        snprintf(tag, sizeof(tag), "rand%d", i);
        v_parse_report(f, tag, pkt, len);
    }
    fclose(f);
    return 0;
}

#endif /* VECTORS_MAIN_H */

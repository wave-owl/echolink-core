/*
 * echolink-core: GSM 06.10 codec wrapper and RTP voice packets.
 *
 * The codec is libgsm 1.0.24, vendored unmodified under src/gsm/ (gsm.lock). The voice
 * packet follows svxlink 26.05.2 src/echolib/EchoLinkQso.cpp (Qso::sendVoicePacket,
 * Qso::handleAudioPacket) and EchoLinkQso.h (struct VoicePacket, FRAME_COUNT 4).
 *
 * Copyright (C) 2026 Gerolf Ziegenhain. GPL-3.0-or-later.
 */
#include "echolink_core.h"

#include <stdlib.h>
#include <string.h>

#include "gsm/gsm.h"

#define ELK_CORE_VERSION "0.1.1"

struct elk_gsm {
    gsm g;
};

const char *elk_version(void)
{
    return "echolink-core " ELK_CORE_VERSION
           " contract=echolink-c1 abi=1 tx=1 gsm=1.0.24 svxlink=26.05.2";
}

elk_gsm *elk_gsm_new(void)
{
    elk_gsm *st = (elk_gsm *)calloc(1, sizeof(*st));
    if (!st) {
        return NULL;
    }
    st->g = gsm_create();
    if (!st->g) {
        free(st);
        return NULL;
    }
    return st;
}

void elk_gsm_free(elk_gsm *st)
{
    if (!st) {
        return;
    }
    gsm_destroy(st->g);
    free(st);
}

int elk_gsm_reset(elk_gsm *st)
{
    gsm fresh;
    if (!st) {
        return ELK_ERR_ARG;
    }
    fresh = gsm_create();
    if (!fresh) {
        return ELK_ERR_ARG;
    }
    gsm_destroy(st->g);
    st->g = fresh;
    return ELK_OK;
}

int elk_gsm_encode(elk_gsm *st, const int16_t *pcm, uint8_t *frame)
{
    gsm_signal buf[ELK_GSM_SAMPLES];
    int i;
    if (!st || !pcm || !frame) {
        return ELK_ERR_ARG;
    }
    /* gsm_encode takes a non-const buffer; copy instead of casting the caller's away. */
    for (i = 0; i < ELK_GSM_SAMPLES; i++) {
        buf[i] = (gsm_signal)pcm[i];
    }
    gsm_encode(st->g, buf, frame);
    return ELK_OK;
}

int elk_gsm_decode(elk_gsm *st, const uint8_t *frame, int16_t *pcm)
{
    gsm_byte buf[ELK_GSM_FRAME_BYTES];
    gsm_signal out[ELK_GSM_SAMPLES];
    int i;
    if (!st || !frame || !pcm) {
        return ELK_ERR_ARG;
    }
    memcpy(buf, frame, sizeof(buf));
    if (gsm_decode(st->g, buf, out) != 0) {
        return ELK_ERR_FRAME;
    }
    for (i = 0; i < ELK_GSM_SAMPLES; i++) {
        pcm[i] = (int16_t)out[i];
    }
    return ELK_OK;
}

int elk_voice_build(elk_gsm *enc, uint16_t seq, const int16_t *pcm, uint8_t *out, size_t cap)
{
    int i;
    if (!enc || !pcm || !out) {
        return ELK_ERR_ARG;
    }
    if (cap < ELK_VOICE_PACKET_BYTES) {
        return ELK_ERR_SPACE;
    }
    /* struct VoicePacket header: version 0xc0, pt, seqNum (htons), time 0, ssrc 0. */
    out[0] = ELK_VOICE_VERSION;
    out[1] = ELK_PT_GSM;
    out[2] = (uint8_t)(seq >> 8);
    out[3] = (uint8_t)(seq & 0xff);
    memset(out + 4, 0, 8);
    for (i = 0; i < ELK_FRAMES_PER_PACKET; i++) {
        elk_gsm_encode(enc, pcm + i * ELK_GSM_SAMPLES,
                       out + ELK_RTP_HEADER_BYTES + i * ELK_GSM_FRAME_BYTES);
    }
    return ELK_VOICE_PACKET_BYTES;
}

int elk_voice_parse(elk_gsm *dec, const uint8_t *pkt, size_t len, int16_t *pcm, uint16_t *seq)
{
    int i;
    if (!dec || !pkt || !pcm) {
        return ELK_ERR_ARG;
    }
    if (len < ELK_RTP_HEADER_BYTES) {
        return ELK_ERR_SHORT;
    }
    if (pkt[0] != ELK_VOICE_VERSION) {
        return ELK_ERR_MALFORMED;
    }
    if (pkt[1] == ELK_PT_SPEEX) {
        return ELK_ERR_CODEC;
    }
    /* svxlink: every payload type but Speex is GSM; four frames must be present. */
    if (len < (size_t)ELK_VOICE_PACKET_BYTES) {
        return ELK_ERR_SHORT;
    }
    if (seq) {
        *seq = (uint16_t)((pkt[2] << 8) | pkt[3]);
    }
    for (i = 0; i < ELK_FRAMES_PER_PACKET; i++) {
        int rc = elk_gsm_decode(dec, pkt + ELK_RTP_HEADER_BYTES + i * ELK_GSM_FRAME_BYTES,
                                pcm + i * ELK_GSM_SAMPLES);
        if (rc != ELK_OK) {
            /* svxlink plays whatever gsm_decode wrote; the core gives silence instead so a
             * corrupt frame never repeats stale samples. */
            memset(pcm + i * ELK_GSM_SAMPLES, 0, ELK_GSM_SAMPLES * sizeof(int16_t));
        }
    }
    return ELK_PACKET_SAMPLES;
}

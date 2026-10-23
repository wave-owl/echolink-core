/*
 * echolink-core: RTCP control packets and the "oNDATA" data packets.
 *
 * C99 port of svxlink 26.05.2 (commit 595cf9bb3325be753d05b219bdd39feab6a22b33)
 *   src/echolib/rtpacket.cpp  rtp_make_sdes, rtp_make_bye, parseSDES, isRTCPByepacket,
 *                             isRTCPSdespacket, sanitizeCallsign
 *   src/echolib/EchoLinkQso.cpp  Qso::sendChatData, Qso::sendInfoData,
 *                                Qso::handleNonAudioPacket
 * EchoLib -- Copyright (C) 2003-2026 Tobias Blomberg / SM0SVX, GPL-2.0-or-later; rtpacket.cpp
 * derives from the "Speak Freely" RTP routines (public domain, John Walker).
 *
 * Deviations, all for inputs where svxlink is undefined:
 *   D1 builders take the output capacity and refuse instead of overflowing (svxlink writes
 *      into fixed buffers: sprintf into tmp[256], 64-byte BYE buffer);
 *   D2 an SDES item longer than 255 bytes is refused (svxlink's length byte would wrap);
 *   D3 elk_is_sdes/elk_is_bye stop on a zero-length step instead of looping forever on a
 *      sub-packet whose length word makes the walk stand still (cannot happen: the step is
 *      always >= 4, kept as a guard);
 *   D4 parsers take const input and never read past len (svxlink's isRTCP* read p[1] of
 *      a following sub-packet only after checking p + 4 <= end; kept).
 *
 * Copyright (C) 2026 Gerolf Ziegenhain. GPL-3.0-or-later.
 */
#include "echolink_core.h"

#include <stdio.h>
#include <string.h>

/* addText of rtpacket.cpp: length byte + text. */
static int add_text(uint8_t **ap, const uint8_t *end, const char *text, size_t sl)
{
    if (sl > 255) {
        return ELK_ERR_ARG;  /* D2 */
    }
    if (*ap + 1 + sl > end) {
        return ELK_ERR_SPACE;  /* D1 */
    }
    *(*ap)++ = (uint8_t)sl;
    memcpy(*ap, text, sl);
    *ap += sl;
    return ELK_OK;
}

/* Null receiver report + the 4-byte header of the following sub-packet (rtpacket.cpp). */
static void rr_and_header(uint8_t *out, int type)
{
    unsigned short ver = (unsigned short)((ELK_RTP_VERSION << 14) | type | (1 << 8));
    out[0] = (uint8_t)(ELK_RTP_VERSION << 6);
    out[1] = ELK_RTCP_RR;
    out[2] = 0;
    out[3] = 1;
    out[4] = out[5] = out[6] = out[7] = 0;
    out[8] = (uint8_t)(ver >> 8);
    out[9] = (uint8_t)(ver & 0xff);
    out[12] = out[13] = out[14] = out[15] = 0;
}

static int finish(uint8_t *p, uint8_t *ap, const uint8_t *end)
{
    int len;
    while ((ap - p) & 3) {
        if (ap >= end) {
            return ELK_ERR_SPACE;
        }
        *ap++ = 0;
    }
    len = (int)((ap - p) / 4) - 1;
    p[2] = (uint8_t)(len >> 8);
    p[3] = (uint8_t)(len & 0xff);
    return (int)(ap - p) + 8;
}

#define TRY(x) do { int rc_ = (x); if (rc_ != ELK_OK) return rc_; } while (0)

int elk_sdes_build(const char *callsign, const char *name, const char *priv, uint8_t *out,
                   size_t cap)
{
    char tmp[ELK_SDES_MAX + 16];
    const uint8_t *end;
    uint8_t *p, *ap;
    int n;
    if (!callsign || !name || !out) {
        return ELK_ERR_ARG;
    }
    if (cap < 16) {
        return ELK_ERR_SPACE;
    }
    end = out + cap;
    rr_and_header(out, ELK_RTCP_SDES);
    p = out + 8;
    ap = p + 8;
    if (ap + 1 > end) return ELK_ERR_SPACE;
    *ap++ = ELK_SDES_CNAME;
    TRY(add_text(&ap, end, "CALLSIGN", 8));
    if (ap + 1 > end) return ELK_ERR_SPACE;
    *ap++ = ELK_SDES_NAME;
    if (strlen(callsign) > 255 || strlen(name) > 255) {
        return ELK_ERR_ARG;
    }
    n = snprintf(tmp, sizeof(tmp), "%-15s%s", callsign, name);
    if (n < 0 || n > 255) {
        return ELK_ERR_ARG;  /* D2 */
    }
    TRY(add_text(&ap, end, tmp, (size_t)n));
    if (ap + 1 > end) return ELK_ERR_SPACE;
    *ap++ = ELK_SDES_EMAIL;
    TRY(add_text(&ap, end, "CALLSIGN", 8));
    if (ap + 1 > end) return ELK_ERR_SPACE;
    *ap++ = ELK_SDES_PHONE;
    TRY(add_text(&ap, end, "08:30", 5));
    if (priv) {
        if (ap + 1 > end) return ELK_ERR_SPACE;
        *ap++ = ELK_SDES_PRIV;
        TRY(add_text(&ap, end, priv, strlen(priv)));
    }
    if (ap + 2 > end) return ELK_ERR_SPACE;
    *ap++ = ELK_SDES_END;
    *ap++ = 0;
    return finish(p, ap, end);
}

int elk_bye_build(uint8_t *out, size_t cap)
{
    const uint8_t *end;
    uint8_t *p, *ap;
    if (!out) {
        return ELK_ERR_ARG;
    }
    if (cap < 24) {
        return ELK_ERR_SPACE;
    }
    end = out + cap;
    rr_and_header(out, ELK_RTCP_BYE);
    p = out + 8;
    ap = p + 8;
    TRY(add_text(&ap, end, "jan2002", 7));
    return finish(p, ap, end);
}

static int version_ok(uint8_t b)
{
    int v = (b >> 6) & 3;
    return v == ELK_RTP_VERSION || v == 1;
}

/* isRTCPSdespacket / isRTCPByepacket of rtpacket.cpp, one walk for both. */
static int saw_type(const uint8_t *p, size_t len, int type)
{
    const uint8_t *end = p + len;
    int saw = 0;
    if (len < 4) {
        return 0;
    }
    if (!version_ok(p[0]) || (p[0] & 0x20) != 0 ||
        (p[1] != ELK_RTCP_SR && p[1] != ELK_RTCP_RR)) {
        return 0;
    }
    do {
        size_t step;
        if (p[1] == type) {
            saw = 1;
        }
        step = (size_t)((((unsigned)p[2] << 8) | p[3]) + 1) * 4;
        if ((size_t)(end - p) <= step) {
            break;  /* svxlink: the next header would not fit (p + 4 > end) */
        }
        p += step;
    } while (p + 4 <= end && ((p[0] >> 6) & 3) == ELK_RTP_VERSION);
    return saw;
}

int elk_is_sdes(const uint8_t *pkt, size_t len)
{
    return pkt ? saw_type(pkt, len, ELK_RTCP_SDES) : 0;
}

int elk_is_bye(const uint8_t *pkt, size_t len)
{
    return pkt ? saw_type(pkt, len, ELK_RTCP_BYE) : 0;
}

int elk_sdes_item(const uint8_t *packet, size_t packet_len, int item, char *out, size_t cap)
{
    const uint8_t *p = packet;
    const uint8_t *end;
    if (!packet || !out || cap < ELK_SDES_MAX) {
        return ELK_ERR_ARG;
    }
    end = packet + packet_len;
    out[0] = 0;
    while (p + 4 <= end && version_ok(p[0])) {
        size_t len = (size_t)((((unsigned)p[2] << 8) | p[3]) + 1) * 4;
        if ((size_t)(end - p) < len) {
            break;
        }
        if (p[1] == ELK_RTCP_SDES && (p[0] & 0x1f) > 0) {
            const uint8_t *cp = p + 8;
            const uint8_t *lp = p + len;
            while (cp + 2 <= lp) {
                uint8_t itype = cp[0];
                uint8_t ilen = cp[1];
                if (itype == ELK_SDES_END) {
                    break;
                }
                if (cp + 2 + ilen > lp) {
                    break;
                }
                if (item == itype) {
                    memcpy(out, cp + 2, ilen);
                    out[ilen] = 0;
                    return ilen;
                }
                cp += ilen + 2;
            }
            break;
        }
        p += len;
    }
    return ELK_ERR_MALFORMED;
}

int elk_sanitize_callsign(const char *in, char *out, size_t cap)
{
    size_t n = 0;
    const char *src;
    if (!in || !out || cap == 0) {
        return ELK_ERR_ARG;
    }
    for (src = in; *src != 0 && n < ELK_CALLSIGN_MAX && n + 1 < cap; ++src) {
        char ch = *src;
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') ||
            ch == '-' || ch == '/' || ch == '*' || ch == '_') {
            out[n++] = ch;
        }
    }
    out[n] = 0;
    return (int)n;
}

int elk_chat_build(const char *callsign, const char *text, uint8_t *out, size_t cap)
{
    size_t lc, lt, need;
    if (!callsign || !text || !out) {
        return ELK_ERR_ARG;
    }
    lc = strlen(callsign);
    lt = strlen(text);
    need = 6 + lc + 1 + lt + 2 + 1;
    if (need > ELK_PACKET_MAX) {
        return ELK_ERR_ARG;
    }
    if (need > cap) {
        return ELK_ERR_SPACE;
    }
    memcpy(out, "oNDATA", 6);
    memcpy(out + 6, callsign, lc);
    out[6 + lc] = '>';
    memcpy(out + 7 + lc, text, lt);
    memcpy(out + 7 + lc + lt, "\r\n", 3);  /* incl. the NUL svxlink sends */
    return (int)need;
}

int elk_info_build(const char *info, uint8_t *out, size_t cap)
{
    size_t li, need, i;
    if (!info || !out) {
        return ELK_ERR_ARG;
    }
    li = strlen(info);
    need = 7 + li + 1;
    if (need > ELK_PACKET_MAX) {
        return ELK_ERR_ARG;
    }
    if (need > cap) {
        return ELK_ERR_SPACE;
    }
    memcpy(out, "oNDATA\r", 7);
    for (i = 0; i < li; i++) {
        out[7 + i] = (uint8_t)(info[i] == '\n' ? '\r' : info[i]);
    }
    out[7 + li] = 0;
    return (int)need;
}

int elk_data_parse(const uint8_t *pkt, size_t len, char *out, size_t cap)
{
    const uint8_t *nul;
    size_t start, n, i;
    int kind;
    if (!pkt || !out || cap == 0) {
        return ELK_ERR_ARG;
    }
    if (len < 7) {
        return ELK_ERR_SHORT;
    }
    if (memcmp(pkt + 1, "NDATA", 5) != 0) {
        return ELK_ERR_MALFORMED;
    }
    kind = pkt[6] == 0x0d ? ELK_KIND_INFO : ELK_KIND_CHAT;
    start = kind == ELK_KIND_INFO ? 7 : 6;
    nul = (const uint8_t *)memchr(pkt, 0, len);
    if (!nul || (size_t)(nul - pkt) < start) {
        return ELK_ERR_MALFORMED;
    }
    n = (size_t)(nul - pkt) - start;
    if (n + 1 > cap) {
        return ELK_ERR_SPACE;
    }
    for (i = 0; i < n; i++) {
        char ch = (char)pkt[start + i];
        out[i] = ch == '\r' ? '\n' : ch;
    }
    out[n] = 0;
    return kind;
}

int elk_classify(const uint8_t *pkt, size_t len, int control)
{
    if (!pkt || len == 0) {
        return ELK_KIND_NONE;
    }
    if (control) {
        if (elk_is_bye(pkt, len)) {
            return ELK_KIND_BYE;
        }
        return elk_is_sdes(pkt, len) ? ELK_KIND_SDES : ELK_KIND_NONE;
    }
    if (pkt[0] == ELK_VOICE_VERSION) {
        return ELK_KIND_VOICE;
    }
    if (len >= 7 && memcmp(pkt + 1, "NDATA", 5) == 0) {
        return pkt[6] == 0x0d ? ELK_KIND_INFO : ELK_KIND_CHAT;
    }
    return ELK_KIND_NONE;
}

/*
 * ref_tool: writes the conformance vectors of the reference implementations.
 *   - GSM 06.10: libgsm 1.0.24, compiled unmodified from ref/libgsm (libgsm's own flags
 *     -DSASR -DWAV49 -DNeedFunctionPrototypes=1, assertions on).
 *   - RTCP: svxlink 26.05.2 src/echolib/rtpacket.cpp, compiled unmodified from ref/svxlink.
 *   - Voice, chat and info packets: the statements of svxlink 26.05.2
 *     src/echolib/EchoLinkQso.cpp (Qso::sendVoicePacket, Qso::sendChatData,
 *     Qso::sendInfoData, Qso::handleNonAudioPacket), copied below with only the socket call
 *     and the logging replaced; each block cites its lines.
 * Copyright (C) 2026 Gerolf Ziegenhain. GPL-3.0-or-later.
 */
#include <algorithm>
#include <arpa/inet.h>
#include <string>

extern "C" {
#include "gsm.h"
}
#include "rtp.h"
#include "rtpacket.h"

#include "../../tests/vectors_main.h"

static void *impl_gsm_new(void) { return gsm_create(); }
static void impl_gsm_free(void *st) { gsm_destroy((gsm)st); }
static void impl_gsm_encode(void *st, const int16_t *pcm, uint8_t *frame)
{
    gsm_signal buf[160];
    for (int i = 0; i < 160; i++) buf[i] = pcm[i];
    gsm_encode((gsm)st, buf, frame);
}
static void impl_gsm_decode(void *st, const uint8_t *frame, int16_t *pcm)
{
    gsm_byte buf[33];
    gsm_signal out[160];
    memcpy(buf, frame, 33);
    if (gsm_decode((gsm)st, buf, out) != 0) {
        memset(out, 0, sizeof(out));  /* the core's documented choice for a bad magic */
    }
    for (int i = 0; i < 160; i++) pcm[i] = out[i];
}

/* EchoLinkQso.h struct VoicePacket */
struct VoicePacket {
    struct {
        uint8_t version;
        uint8_t pt;
        uint16_t seqNum;
        uint32_t time;
        uint32_t ssrc;
    } header;
    uint8_t data[1024];
} __attribute__((packed));

static int impl_voice_build(void *enc, uint16_t seq, const int16_t *pcm, uint8_t *out)
{
    /* EchoLinkQso.cpp Qso::sendVoicePacket (GSM branch) */
    VoicePacket voice_packet;
    size_t nbytes = 0;
    const int FRAME_COUNT = 4;
    for (int i = 0; i < FRAME_COUNT; i++) {
        gsm_signal buf[160];
        for (int k = 0; k < 160; k++) buf[k] = pcm[i * 160 + k];
        gsm_encode((gsm)enc, buf, voice_packet.data + i * 33);
        nbytes += 33;
    }
    voice_packet.header.version = 0xc0;
    voice_packet.header.pt = 0x03;
    voice_packet.header.time = htonl(0);
    voice_packet.header.ssrc = htonl(0);
    voice_packet.header.seqNum = htons(seq);
    int len = (int)(nbytes + sizeof(voice_packet.header));
    memcpy(out, &voice_packet, (size_t)len);
    return len;
}

static int impl_sdes(const char *call, const char *name, const char *priv, uint8_t *out)
{
    unsigned char buf[1100];
    int n = rtp_make_sdes(buf, call, name, priv);
    memcpy(out, buf, (size_t)n);
    return n;
}
static int impl_bye(uint8_t *out) { return rtp_make_bye(out); }
static int impl_is_sdes(const uint8_t *p, int len)
{
    return isRTCPSdespacket(const_cast<unsigned char *>(p), len) ? 1 : 0;
}
static int impl_is_bye(const uint8_t *p, int len)
{
    return isRTCPByepacket(const_cast<unsigned char *>(p), len) ? 1 : 0;
}
static int impl_sdes_item(const uint8_t *p, int len, int item, char *out)
{
    if (!parseSDES(out, const_cast<unsigned char *>(p), len, (unsigned char)item)) {
        return -1;
    }
    return (int)strlen(out);
}
static void impl_sanitize(const char *in, char *out)
{
    strcpy(out, in);
    sanitizeCallsign(out);
}
static int impl_chat(const char *callsign, const char *msg, uint8_t *out)
{
    /* EchoLinkQso.cpp Qso::sendChatData */
    std::string buf("oNDATA" + std::string(callsign) + '>' + msg + "\r\n");
    memcpy(out, buf.c_str(), buf.length() + 1);
    return (int)buf.length() + 1;
}
static int impl_info(const char *info, uint8_t *out)
{
    /* EchoLinkQso.cpp Qso::sendInfoData */
    std::string info_msg("oNDATA\r");
    info_msg += info;
    std::replace(info_msg.begin(), info_msg.end(), '\n', '\r');
    memcpy(out, info_msg.c_str(), info_msg.length() + 1);
    return (int)info_msg.length() + 1;
}
static int impl_data_parse(const uint8_t *cbuf, int len, char *out)
{
    /* EchoLinkQso.cpp Qso::handleNonAudioPacket */
    unsigned char *buf = const_cast<unsigned char *>(cbuf);
    if (len < 7) {
        return -1;
    }
    if (memcmp(buf + 1, "NDATA", 5) == 0) {
        if (buf[6] == 0x0d) {
            char *info_buf = reinterpret_cast<char *>(buf);
            char *null = (char *)memchr(info_buf, 0, len);
            if (null == 0 || null < info_buf + 7) {
                return -1;
            }
            std::string info_msg(info_buf + 7, null);
            std::replace(info_msg.begin(), info_msg.end(), '\r', '\n');
            strcpy(out, info_msg.c_str());
            return 3;
        } else {
            char *chat_buf = reinterpret_cast<char *>(buf);
            char *null = (char *)memchr(buf, 0, len);
            if (null == 0 || null < chat_buf + 6) {
                return -1;
            }
            std::string chat_msg(chat_buf + 6, null);
            std::replace(chat_msg.begin(), chat_msg.end(), '\r', '\n');
            strcpy(out, chat_msg.c_str());
            return 2;
        }
    }
    return -1;
}

int main(int argc, char **argv)
{
    return v_main(argc, argv);
}

/*
 * echolink-core -- EchoLink station-to-station packets and the GSM 06.10 codec in C99.
 *
 * Contract echolink-c1, ABI 1, capability tx=1 (the encoder and every packet builder).
 *
 * - GSM 06.10 full-rate codec: libgsm 1.0.24 (Jutta Degener and Carsten Bormann, TU Berlin),
 *   vendored unmodified under src/gsm/ (gsm.lock), the codec every EchoLink client speaks.
 * - Packets: a C99 port of svxlink 26.05.2 echolib (Tobias Blomberg SM0SVX): rtpacket.cpp
 *   (RTCP SDES/BYE build and parse, callsign sanitising) and EchoLinkQso.cpp (the RTP voice
 *   packet with four GSM frames, the "oNDATA" chat and info packets).
 *
 * The core has no sockets, no threads and no global state; every function is reentrant.
 * Copyright (C) 2026 Gerolf Ziegenhain. GPL-3.0-or-later (see LICENSE and NOTICE).
 */
#ifndef ECHOLINK_CORE_H
#define ECHOLINK_CORE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNUC__)
#define ELK_API __attribute__((visibility("default")))
#else
#define ELK_API
#endif

#define ELK_SAMPLE_RATE 8000
#define ELK_GSM_SAMPLES 160          /* samples per GSM frame (20 ms) */
#define ELK_GSM_FRAME_BYTES 33       /* bytes per GSM frame */
#define ELK_FRAMES_PER_PACKET 4      /* svxlink EchoLinkQso.h FRAME_COUNT */
#define ELK_PACKET_SAMPLES 640       /* 80 ms per voice packet (BLOCK_TIME) */
#define ELK_RTP_HEADER_BYTES 12
#define ELK_VOICE_PACKET_BYTES 144   /* 12 + 4 * 33 */
#define ELK_VOICE_VERSION 0xc0       /* first byte of every voice packet */
#define ELK_PT_GSM 0x03
#define ELK_PT_SPEEX 0x96            /* not supported by this core */
#define ELK_RTP_VERSION 3            /* EchoLink's RTP version field (svxlink rtp.h) */
#define ELK_RTCP_SR 200
#define ELK_RTCP_RR 201
#define ELK_RTCP_SDES 202
#define ELK_RTCP_BYE 203
#define ELK_SDES_END 0
#define ELK_SDES_CNAME 1
#define ELK_SDES_NAME 2
#define ELK_SDES_EMAIL 3
#define ELK_SDES_PHONE 4
#define ELK_SDES_PRIV 8
#define ELK_SDES_MAX 256             /* svxlink RTP_MAX_SDES: item text buffer incl. NUL */
#define ELK_CALLSIGN_MAX 32          /* svxlink sanitizeCallsign max_len */
#define ELK_PACKET_MAX 1024          /* largest packet any builder writes */

/* Return codes. */
#define ELK_OK 0
#define ELK_ERR_ARG (-1)     /* NULL pointer or a value out of range */
#define ELK_ERR_SPACE (-2)   /* the output buffer is too small */
#define ELK_ERR_SHORT (-3)   /* the packet is shorter than its kind needs */
#define ELK_ERR_CODEC (-4)   /* Speex payload (pt 0x96): not supported */
#define ELK_ERR_FRAME (-5)   /* a GSM frame without the 0xD magic nibble */
#define ELK_ERR_MALFORMED (-6)

/* Kinds returned by elk_classify() and elk_data_parse(). */
#define ELK_KIND_NONE 0
#define ELK_KIND_VOICE 1
#define ELK_KIND_CHAT 2
#define ELK_KIND_INFO 3
#define ELK_KIND_SDES 4
#define ELK_KIND_BYE 5

/* Version string: "echolink-core <x.y.z> contract=echolink-c1 abi=1 tx=1 gsm=1.0.24
 * svxlink=26.05.2". */
ELK_API const char *elk_version(void);

/* ------------------------------------------------------------------ GSM 06.10 codec */

typedef struct elk_gsm elk_gsm;

/* One codec state; use one for encoding and a separate one for decoding (libgsm keeps
 * the predictor state in it). NULL when out of memory. */
ELK_API elk_gsm *elk_gsm_new(void);
ELK_API void elk_gsm_free(elk_gsm *st);
/* Back to the state of a fresh elk_gsm_new(). */
ELK_API int elk_gsm_reset(elk_gsm *st);
/* 160 samples -> one 33-byte frame (libgsm gsm_encode). */
ELK_API int elk_gsm_encode(elk_gsm *st, const int16_t *pcm, uint8_t *frame);
/* One 33-byte frame -> 160 samples (libgsm gsm_decode); ELK_ERR_FRAME on a bad magic. */
ELK_API int elk_gsm_decode(elk_gsm *st, const uint8_t *frame, int16_t *pcm);

/* ------------------------------------------------------------------ voice packets */

/* 640 samples -> one 144-byte RTP voice packet: 0xc0, pt 3, seq (network order),
 * timestamp 0, ssrc 0, four GSM frames (svxlink Qso::sendVoicePacket). Returns 144. */
ELK_API int elk_voice_build(elk_gsm *enc, uint16_t seq, const int16_t *pcm, uint8_t *out,
                            size_t cap);
/* A received voice packet -> 640 samples (svxlink Qso::handleAudioPacket: any payload type
 * except Speex is GSM). Returns 640, ELK_ERR_SHORT, ELK_ERR_CODEC or ELK_ERR_FRAME; *seq
 * gets the sequence number when not NULL. */
ELK_API int elk_voice_parse(elk_gsm *dec, const uint8_t *pkt, size_t len, int16_t *pcm,
                            uint16_t *seq);

/* ------------------------------------------------------------------ RTCP control packets */

/* SDES (svxlink rtp_make_sdes): null RR + SDES with CNAME "CALLSIGN", NAME "%-15s%s"
 * (callsign, name), EMAIL "CALLSIGN", PHONE "08:30", PRIV (only when priv != NULL), END,
 * zero padding. ELK_ERR_ARG when an item would exceed 255 bytes (svxlink overflows). */
ELK_API int elk_sdes_build(const char *callsign, const char *name, const char *priv,
                           uint8_t *out, size_t cap);
/* BYE (svxlink rtp_make_bye): null RR + BYE with the reason "jan2002", 24 bytes. */
ELK_API int elk_bye_build(uint8_t *out, size_t cap);
/* svxlink isRTCPSdespacket / isRTCPByepacket: 1 or 0. */
ELK_API int elk_is_sdes(const uint8_t *pkt, size_t len);
ELK_API int elk_is_bye(const uint8_t *pkt, size_t len);
/* svxlink parseSDES: the first SDES item of type *item* into out (NUL-terminated). Returns
 * its length, or ELK_ERR_MALFORMED when absent; cap must be at least ELK_SDES_MAX. */
ELK_API int elk_sdes_item(const uint8_t *pkt, size_t len, int item, char *out, size_t cap);
/* svxlink sanitizeCallsign: keep A-Z a-z 0-9 - / * _, at most 32 characters. Returns the
 * length of the result. */
ELK_API int elk_sanitize_callsign(const char *in, char *out, size_t cap);

/* ------------------------------------------------------------------ data packets (port 5198) */

/* Chat (svxlink Qso::sendChatData): "oNDATA" + callsign + '>' + text + "\r\n" + NUL. */
ELK_API int elk_chat_build(const char *callsign, const char *text, uint8_t *out, size_t cap);
/* Info (svxlink Qso::sendInfoData): "oNDATA\r" + info with every '\n' -> '\r' + NUL. */
ELK_API int elk_info_build(const char *info, uint8_t *out, size_t cap);
/* svxlink Qso::handleNonAudioPacket: ELK_KIND_CHAT (text from byte 6) or ELK_KIND_INFO
 * (text from byte 7), up to the first NUL, every '\r' -> '\n'; ELK_ERR_SHORT below 7 bytes,
 * ELK_ERR_MALFORMED without "NDATA" at byte 1 or without a NUL. */
ELK_API int elk_data_parse(const uint8_t *pkt, size_t len, char *out, size_t cap);

/* Which packet is this: voice (first byte 0xc0 on the audio port), chat/info (port 5198,
 * "NDATA"), SDES or BYE (port 5199). *control* is 1 for the control port. */
ELK_API int elk_classify(const uint8_t *pkt, size_t len, int control);

#ifdef __cplusplus
}
#endif

#endif /* ECHOLINK_CORE_H */

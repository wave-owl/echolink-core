/*
 * elk_tool: writes the conformance vectors of the core (tests/vectors_main.h).
 *   elk_tool <outdir>
 * Copyright (C) 2026 Gerolf Ziegenhain. GPL-3.0-or-later.
 */
#include "echolink_core.h"
#include "../tests/vectors_main.h"

static void *impl_gsm_new(void) { return elk_gsm_new(); }
static void impl_gsm_free(void *st) { elk_gsm_free((elk_gsm *)st); }
static void impl_gsm_encode(void *st, const int16_t *pcm, uint8_t *frame)
{
    elk_gsm_encode((elk_gsm *)st, pcm, frame);
}
static void impl_gsm_decode(void *st, const uint8_t *frame, int16_t *pcm)
{
    elk_gsm_decode((elk_gsm *)st, frame, pcm);
}
static int impl_voice_build(void *enc, uint16_t seq, const int16_t *pcm, uint8_t *out)
{
    return elk_voice_build((elk_gsm *)enc, seq, pcm, out, ELK_PACKET_MAX);
}
static int impl_sdes(const char *call, const char *name, const char *priv, uint8_t *out)
{
    return elk_sdes_build(call, name, priv, out, ELK_PACKET_MAX);
}
static int impl_bye(uint8_t *out) { return elk_bye_build(out, ELK_PACKET_MAX); }
static int impl_is_sdes(const uint8_t *p, int len) { return elk_is_sdes(p, (size_t)len); }
static int impl_is_bye(const uint8_t *p, int len) { return elk_is_bye(p, (size_t)len); }
static int impl_sdes_item(const uint8_t *p, int len, int item, char *out)
{
    /* the reference harness reports strlen() of svxlink's parseSDES result */
    return elk_sdes_item(p, (size_t)len, item, out, ELK_SDES_MAX) < 0 ? -1 : (int)strlen(out);
}
static void impl_sanitize(const char *in, char *out) { elk_sanitize_callsign(in, out, 300); }
static int impl_chat(const char *call, const char *text, uint8_t *out)
{
    return elk_chat_build(call, text, out, ELK_PACKET_MAX);
}
static int impl_info(const char *info, uint8_t *out)
{
    return elk_info_build(info, out, ELK_PACKET_MAX);
}
static int impl_data_parse(const uint8_t *p, int len, char *out)
{
    int rc = elk_data_parse(p, (size_t)len, out, 1100);
    return rc == ELK_KIND_CHAT ? 2 : rc == ELK_KIND_INFO ? 3 : -1;
}

int main(int argc, char **argv)
{
    return v_main(argc, argv);
}

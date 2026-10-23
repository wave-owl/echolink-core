# Changelog

## 0.1.1

- Build libgsm's unmodified files without `-Werror`: GCC flags `-Wsign-compare` in
  `private.h` (`GSM_ADD`, `GSM_SUB`); clang does not. No change in output (goldens unchanged).

## 0.1.0

- GSM 06.10 full-rate codec: libgsm 1.0.24 vendored unmodified under `src/gsm/`
  (`gsm.lock`, `tools/check_gsm.sh`), compiled with libgsm's own defines.
- EchoLink packets after svxlink 26.05.2 echolib: RTP voice packet (0xc0, pt 3, four GSM
  frames), RTCP SDES and BYE (`rtpacket.cpp`), callsign sanitising, "oNDATA" chat and info
  packets (`EchoLinkQso.cpp`).
- Goldens written by the reference harness (`make golden`/`make ref`): encoder frames,
  decoder output, decoder on 200 random frames, 100 voice packets, every packet builder and
  parser on 7 SDES, BYE, 3 chat, 3 info cases, their truncations and bit flips and 300 random
  datagrams. The core reproduces all of them byte for byte.
- Version string `echolink-core 0.1.0 contract=echolink-c1 abi=1 tx=1 gsm=1.0.24
  svxlink=26.05.2`; only `elk_*` symbols are exported.

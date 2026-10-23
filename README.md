# echolink-core

EchoLink station-to-station packets and the GSM 06.10 voice codec in C99, for
[Wave Owl](https://github.com/wave-owl) (`src/echolink_native.py`).

- **GSM 06.10 full-rate codec**: [libgsm](https://www.quut.com/gsm/) 1.0.24 by Jutta Degener
  and Carsten Bormann (TU Berlin), vendored **unmodified** under `src/gsm/` and pinned by
  checksum (`gsm.lock`). It is the codec svxlink, Qtel and the EchoLink clients use; 160
  samples at 8 kHz (20 ms) <-> 33 bytes.
- **Packets**: a C99 port of [svxlink](https://github.com/sm0svx/svxlink) 26.05.2 echolib by
  Tobias Blomberg SM0SVX (`rtpacket.cpp`, `EchoLinkQso.cpp`):
  - RTP voice packet: `0xc0`, payload type 3, sequence number, timestamp 0, SSRC 0, four
    GSM frames = 144 bytes = 80 ms (UDP port 5198);
  - RTCP SDES (null RR + CNAME/NAME/EMAIL/PHONE/PRIV) and BYE (UDP port 5199), with svxlink's
    `isRTCPSdespacket`, `isRTCPByepacket`, `parseSDES` and `sanitizeCallsign`;
  - the `oNDATA` chat (`oNDATA<call>><text>\r\n\0`) and info (`oNDATA\r<info>\0`) packets.

The directory server protocol (TCP 5200: login, status, station list) is text over TCP and
stays in the Python package of Wave Owl, as do sockets, timers and the connection state
machine. The core has no sockets, no threads and no global state.

Contract `echolink-c1`, ABI 1, capability `tx=1` (encoder and packet builders). Version
string: `echolink-core 0.1.0 contract=echolink-c1 abi=1 tx=1 gsm=1.0.24 svxlink=26.05.2`.

## Build and test

```sh
git clone --recurse-submodules https://github.com/wave-owl/echolink-core
make            # build/libecholink.a and build/libecholink.{so,dylib}
make test       # C tests (API, round trips, error codes, 20000 random datagrams) + goldens
make sanitize   # the tests under ASan + UBSan
make bench      # CPU per second of speech
make ref        # the reference implementations write the vectors again; byte-identical?
```

Plain make, C99, `-Wall -Wextra -Werror -pedantic` (libgsm's files with three warnings of
their 1990s style switched off), no dependencies besides libc. `make ref` needs a C++
compiler and the submodules `ref/libgsm` and `ref/svxlink`; `REF_LIBGSM=` and `REF_SVXLINK=`
point it at other checkouts.

## Conformance

`tests/golden/` is written only by the reference harness `tools/ref/ref_tool.cpp`
(`make golden`), which compiles libgsm 1.0.24 unmodified with libgsm's own defines and
svxlink's `rtpacket.cpp` unmodified, and copies the packet statements of `EchoLinkQso.cpp`
with only the socket call and logging replaced. The core reproduces every file byte for
byte (`make test`):

| File | Content |
|---|---|
| `gsm_frames.bin` | encoder: 400 frames (8 s) of a voiced test signal with noise, silence and a clipped burst |
| `gsm_decoded.pcm` | decoder on those frames |
| `gsm_fuzz_decoded.pcm` | decoder on 200 random frames with the 0xD magic |
| `voice.bin` | 100 voice packets, sequence numbers wrapping at 65535 |
| `packets.txt` | SDES (7 cases), BYE, chat (3), info (3), sanitised callsigns (9); every parser on each packet, on 13 truncations and 20 bit flips of each, and on 300 random datagrams |

On an Apple M-series machine the codec costs about 0.2 ms CPU per second of speech in
each direction (`make bench`).

## Deviations from svxlink (inputs where svxlink is undefined)

1. Builders take the output capacity and refuse instead of writing past a fixed buffer.
2. An SDES item longer than 255 bytes is refused (svxlink's length byte would wrap).
3. A voice frame without the 0xD magic decodes to silence (svxlink plays whatever
   `gsm_decode` left in its buffer); the reference harness does the same so the goldens
   cover it.
4. Speex voice packets (payload type 0x96) are refused; the core never offers Speex (no
   `PRIV` "SPEEX" in its SDES), so peers send GSM.

## Licence

GPL-3.0-or-later (`LICENSE`); `src/gsm/` keeps libgsm's own notice (`src/gsm/COPYRIGHT`).
See `NOTICE`.

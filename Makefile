# echolink-core -- plain make, C99, no dependencies besides libc.
#   make            static + shared library into $(BUILD) (after the gsm/ pin check)
#   make test       C tests: API, parsers, fuzz, and the goldens of tests/golden/
#   make sanitize   the C tests under AddressSanitizer + UndefinedBehaviorSanitizer
#   make bench      encoder/decoder CPU per second of speech
#   make ref        rebuild the vectors with the reference implementations (unmodified
#                   libgsm 1.0.24 in $(REF_LIBGSM), svxlink 26.05.2 in $(REF_SVXLINK)) and
#                   compare them with tests/golden/ byte for byte
#   make golden     write tests/golden/ from the reference implementations (never from the core)
#   make check-gsm  only the src/gsm/ pin check (gsm.lock)
#   make BUILD=/abs/dir shared   out-of-tree build (used by wave-owl)
BUILD ?= build
CC ?= cc
CXX ?= c++
CFLAGS ?= -O2
REF_LIBGSM ?= ref/libgsm
REF_SVXLINK ?= ref/svxlink

CORE_FLAGS = -std=c99 -Wall -Wextra -Werror -pedantic -fPIC -fvisibility=hidden -Iinclude
# libgsm's own build defines (its Makefile: SASR, WAV49, prototypes; assertions on). The
# vendored files are unmodified; two warnings of their 1990s style are switched off.
GSM_FLAGS = -std=c99 -Wall -Wextra -Werror -pedantic -Wno-comment -Wno-unused-parameter -Wno-unused-const-variable \
            -fPIC -fvisibility=hidden -DSASR -DWAV49 -DNeedFunctionPrototypes=1 -Isrc/gsm
TEST_FLAGS = -std=c99 -Wall -Wextra -Werror -pedantic -Iinclude

UNAME := $(shell uname)
ifeq ($(UNAME),Darwin)
  SHLIB = libecholink.dylib
  SHFLAGS = -dynamiclib -install_name @rpath/libecholink.dylib
else
  SHLIB = libecholink.so
  SHFLAGS = -shared -Wl,-soname,libecholink.so
endif

GSM_SRC = add code decode long_term lpc preprocess rpe short_term table gsm_create \
          gsm_destroy gsm_encode gsm_decode gsm_option
OBJ = $(BUILD)/elk_codec.o $(BUILD)/elk_packets.o $(patsubst %,$(BUILD)/gsm_%.o,$(GSM_SRC))

all: static shared
static: $(BUILD)/libecholink.a
shared: $(BUILD)/$(SHLIB)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/gsm.ok: gsm.lock tools/check_gsm.sh $(wildcard src/gsm/*) | $(BUILD)
	sh tools/check_gsm.sh gsm.lock src/gsm
	touch $@

check-gsm:
	sh tools/check_gsm.sh gsm.lock src/gsm

$(BUILD)/elk_%.o: src/elk_%.c include/echolink_core.h $(BUILD)/gsm.ok
	$(CC) $(CORE_FLAGS) -Isrc $(CFLAGS) -c $< -o $@

$(BUILD)/gsm_%.o: src/gsm/%.c $(BUILD)/gsm.ok
	$(CC) $(GSM_FLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/libecholink.a: $(OBJ)
	ar rcs $@ $^

$(BUILD)/$(SHLIB): $(OBJ)
	$(CC) $(SHFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD)/test_elk: tests/test_elk.c $(BUILD)/libecholink.a
	$(CC) $(TEST_FLAGS) $(CFLAGS) $< $(BUILD)/libecholink.a -o $@ $(LDLIBS)

$(BUILD)/elk_tool: tools/elk_tool.c tests/vectors_main.h $(BUILD)/libecholink.a
	$(CC) $(TEST_FLAGS) $(CFLAGS) $< $(BUILD)/libecholink.a -o $@ $(LDLIBS)

$(BUILD)/bench_elk: tests/bench_elk.c tests/vectors_main.h $(BUILD)/libecholink.a
	$(CC) $(TEST_FLAGS) $(CFLAGS) $< $(BUILD)/libecholink.a -o $@ $(LDLIBS)

test: $(BUILD)/test_elk $(BUILD)/elk_tool
	$(BUILD)/test_elk
	rm -rf $(BUILD)/vectors && mkdir -p $(BUILD)/vectors
	$(BUILD)/elk_tool $(BUILD)/vectors
	sh tools/compare.sh tests/golden $(BUILD)/vectors

SAN_FLAGS = -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all

sanitize:
	$(MAKE) --no-print-directory BUILD=$(BUILD)/san CFLAGS="$(SAN_FLAGS)" \
		LDLIBS="$(LDLIBS) $(SAN_FLAGS)" test

bench: $(BUILD)/bench_elk
	$(BUILD)/bench_elk

# ---------------------------------------------------------------- reference implementations
REF_GSM_OBJ = $(patsubst %,$(BUILD)/ref/gsm_%.o,$(GSM_SRC))

$(BUILD)/ref:
	mkdir -p $(BUILD)/ref

$(BUILD)/ref/gsm_%.o: $(REF_LIBGSM)/src/%.c | $(BUILD)/ref
	$(CC) -c -O2 -DNeedFunctionPrototypes=1 -Wall -Wno-comment -DSASR -DWAV49 \
		-I$(REF_LIBGSM)/inc $< -o $@

$(BUILD)/ref/rtpacket.o: $(REF_SVXLINK)/src/echolib/rtpacket.cpp | $(BUILD)/ref
	$(CXX) -c -O2 -I$(REF_SVXLINK)/src/echolib $< -o $@

$(BUILD)/ref/ref_tool: tools/ref/ref_tool.cpp tests/vectors_main.h $(REF_GSM_OBJ) $(BUILD)/ref/rtpacket.o
	$(CXX) -O2 -I$(REF_LIBGSM)/inc -I$(REF_SVXLINK)/src/echolib $< $(REF_GSM_OBJ) \
		$(BUILD)/ref/rtpacket.o -o $@

ref: $(BUILD)/ref/ref_tool
	sh tools/check_gsm.sh gsm.lock $(REF_LIBGSM)/src $(REF_LIBGSM)/inc
	rm -rf $(BUILD)/ref/vectors && mkdir -p $(BUILD)/ref/vectors
	$(BUILD)/ref/ref_tool $(BUILD)/ref/vectors
	sh tools/compare.sh tests/golden $(BUILD)/ref/vectors

golden: $(BUILD)/ref/ref_tool
	mkdir -p tests/golden
	$(BUILD)/ref/ref_tool tests/golden

clean:
	rm -rf $(BUILD)

.PHONY: all static shared check-gsm test sanitize bench ref golden clean

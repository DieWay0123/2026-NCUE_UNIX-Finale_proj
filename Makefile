CC ?= cc
CPPFLAGS ?= -D_XOPEN_SOURCE=700 -Ilibdiag
CFLAGS ?= -std=c99 -Wall -Wextra -Werror -O2
LDFLAGS ?=
LDLIBS ?=
RM ?= rm -f
RMDIR ?= rm -rf

BUILD_DIR := build
APP_DIR := applets
LIB_DIR := libdiag

APPS := bbtop bbfscheck bbnetmon
LIB_SRCS := \
	$(LIB_DIR)/formatter.c \
	$(LIB_DIR)/fs_reader.c \
	$(LIB_DIR)/net_reader.c \
	$(LIB_DIR)/proc_reader.c \
	$(LIB_DIR)/rule_checker.c
LIB_OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(LIB_SRCS))
APP_OBJS := $(patsubst %,$(BUILD_DIR)/$(APP_DIR)/%.o,$(APPS))
DEPS := $(LIB_OBJS:.o=.d) $(APP_OBJS:.o=.d)

.PHONY: all help clean distclean test check test-bbtop test-bbfscheck test-bbnetmon benchmark

all: $(APPS)

help:
	@printf '%s\n' 'BusyBox Diagnostics Toolkit standalone build'
	@printf '%s\n' ''
	@printf '%s\n' 'Targets:'
	@printf '%s\n' '  all              Build bbtop, bbfscheck, and bbnetmon'
	@printf '%s\n' '  bbtop            Build the process monitor prototype'
	@printf '%s\n' '  bbfscheck        Build the filesystem checker prototype'
	@printf '%s\n' '  bbnetmon         Build the TCP monitor prototype'
	@printf '%s\n' '  test             Run all shell tests'
	@printf '%s\n' '  test-bbtop       Run bbtop tests'
	@printf '%s\n' '  test-bbfscheck   Run bbfscheck tests'
	@printf '%s\n' '  test-bbnetmon    Run bbnetmon tests'
	@printf '%s\n' '  benchmark        Run benchmark scripts'
	@printf '%s\n' '  clean            Remove build output and standalone binaries'

bbtop: $(BUILD_DIR)/$(APP_DIR)/bbtop.o $(LIB_OBJS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

bbfscheck: $(BUILD_DIR)/$(APP_DIR)/bbfscheck.o $(LIB_OBJS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

bbnetmon: $(BUILD_DIR)/$(APP_DIR)/bbnetmon.o $(LIB_OBJS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

test: test-bbtop test-bbfscheck test-bbnetmon

check: test

test-bbtop: bbtop
	BBTOP_BIN=./bbtop sh tests/test_bbtop.sh

test-bbfscheck: bbfscheck
	BBFSCHECK_BIN=./bbfscheck sh tests/test_bbfscheck.sh

test-bbnetmon: bbnetmon
	BBNETMON_BIN=./bbnetmon sh tests/test_bbnetmon.sh

benchmark: all
	BBTOP_BIN=./bbtop sh benchmark/bench_bbtop.sh
	BBFSCHECK_BIN=./bbfscheck sh benchmark/bench_bbfscheck.sh
	BBNETMON_BIN=./bbnetmon sh benchmark/bench_bbnetmon.sh

clean:
	$(RMDIR) $(BUILD_DIR)
	$(RM) $(APPS)

distclean: clean

-include $(DEPS)

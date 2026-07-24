FILENAME := 3it

ifdef TARGET
  EXE := $(FILENAME)_$(TARGET)
else
  EXE := $(FILENAME)
endif

JOBS := $(shell nproc)

OUTPUT = build/$(EXE)

.DEFAULT_GOAL := all

CC    ?= gcc
CXX   ?= g++
STRIP ?= strip

ifeq ($(NDEBUG),1)
OPT := -Os -flto -ffunction-sections -fdata-sections
ifeq ($(filter %-macos,$(TARGET)),)
OPT += -static
LDFLAGS += -Wl,--gc-sections -Wl,--strip-all
else
LDFLAGS += -Wl,-dead_strip -Wl,-S -Wl,-x
endif
else
OPT := -O0 -ggdb -ftrapv
endif

ifeq ($(SANITIZE),1)
OPT += -fsanitize=address,undefined
endif

CFLAGS = $(OPT) -Wall -Wextra -Wpedantic -Wshadow -Wno-error=date-time
CXXFLAGS = $(OPT) -Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor -std=c++17
CPPFLAGS ?= -MMD -MP

SRCS_C   := $(wildcard src/*.c)
SRCS_CXX := $(wildcard src/*.cpp)

ifdef TARGET
  BUILDDIR = build/$(TARGET)
else
  BUILDDIR = build
endif
OBJS := $(SRCS_C:src/%.c=$(BUILDDIR)/%.c.o)
OBJS += $(SRCS_CXX:src/%.cpp=$(BUILDDIR)/%.cpp.o)
DEPS  = $(OBJS:.o=.d)


all: $(OUTPUT)

help:
	@echo "Targets:"
	@echo "  all          Build debug binary (default)"
	@echo "  release      Cross-compile release builds via podman"
	@echo "  release-base Cross-compile release builds directly with zig"
	@echo "  strip        Strip debug symbols from binary"
	@echo "  install      Install binary to \$${DESTDIR}\$${BINDIR}"
	@echo "  clean        Remove build artifacts"
	@echo "  distclean    Deep clean (git clean -xfd)"
	@echo ""
	@echo "Options (make <target> VAR=val):"
	@echo "  NDEBUG=1     Release build optimized for size"
	@echo "  SANITIZE=1   AddressSanitizer + UBSan"
	@echo "  TARGET=...   Cross-compilation target suffix"
	@echo "  PREFIX=\$${HOME}/dev/3do-devkit  Install prefix"
	@echo "  BINDIR=\$${PREFIX}/bin/tools/linux  Install directory"
	@echo ""
	@echo "Cross-compilation (requires zig):"
	@echo "  make release-base"
	@echo "    Builds: x86_64-linux-musl, aarch64-linux-musl,"
	@echo "            x86_64-windows-gnu.exe, aarch64-macos"

$(OUTPUT): $(OBJS) | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -o $(OUTPUT) $(OBJS) $(LDFLAGS)

$(OBJS): | $(BUILDDIR)

strip: $(OUTPUT)
	$(STRIP) --strip-all $(OUTPUT)

$(BUILDDIR)/%.c.o: src/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%.cpp.o: src/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rfv build/

distclean: clean
	git clean -xfd

$(BUILDDIR):
	mkdir -p $@

PREFIX ?= $(HOME)/dev/3do-devkit
BINDIR ?= $(PREFIX)/bin/tools/linux

install: $(OUTPUT)
	install -Dm755 $(OUTPUT) $(DESTDIR)$(BINDIR)/$(EXE)

release-base: clean
	$(MAKE) NDEBUG=1 -j$(JOBS) \
		CC="zig cc -target x86_64-linux-musl" \
		CXX="zig c++ -target x86_64-linux-musl" \
		STRIP="zig llvm-strip" \
		TARGET="x86_64-linux-musl" \
		OPT="-Oz -flto -ffunction-sections -fdata-sections -static"
	$(MAKE) NDEBUG=1 -j$(JOBS) \
		CC="zig cc -target aarch64-linux-musl" \
		CXX="zig c++ -target aarch64-linux-musl" \
		STRIP="zig llvm-strip" \
		TARGET="aarch64-linux-musl" \
		OPT="-Oz -flto -ffunction-sections -fdata-sections -static"
	$(MAKE) NDEBUG=1 -j$(JOBS) \
		CC="zig cc -target x86_64-windows-gnu" \
		CXX="zig c++ -target x86_64-windows-gnu" \
		STRIP="zig llvm-strip" \
		TARGET="x86_64-windows-gnu.exe" \
		OPT="-Oz -ffunction-sections -fdata-sections -static"
	$(MAKE) NDEBUG=1 -j$(JOBS) \
		CC="zig cc -target aarch64-macos" \
		CXX="zig c++ -target aarch64-macos" \
		STRIP="zig llvm-strip" \
		TARGET="aarch64-macos" \
		OPT="-Oz -ffunction-sections -fdata-sections"

release:
	podman build -t localhost/cxxbuilder buildtools/
	podman run --rm --userns=keep-id \
		-e HOME=/tmp \
		-e ZIG_GLOBAL_CACHE_DIR=/tmp/zig-global-cache \
		-e ZIG_LOCAL_CACHE_DIR=/tmp/zig-local-cache \
		-v ${PWD}:/src:Z localhost/cxxbuilder "/src/buildtools/podman-make-release"

.PHONY: help all clean distclean release release-base strip install

-include $(DEPS)

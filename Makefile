FILENAME := 3it

ifdef TARGET
  EXE := $(FILENAME)_$(TARGET)
else
  EXE := $(FILENAME)
endif

JOBS := $(shell nproc)

OUTPUT = build/$(EXE)

CC    ?= gcc
CXX   ?= g++
STRIP ?= strip

ifeq ($(NDEBUG),1)
OPT := -O3 -flto -static
LDFLAGS += -Wl,--strip-all
else
OPT := -O0 -ggdb -ftrapv
endif

ifeq ($(SANITIZE),1)
OPT += -fsanitize=address,undefined
endif

CFLAGS = $(OPT) -Wall -Wextra -Wpedantic -Wshadow
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
	@echo "  install      Install binary to \$${DESTDIR}\$${PREFIX}/bin"
	@echo "  clean        Remove build artifacts"
	@echo "  distclean    Deep clean (git clean -xfd)"
	@echo ""
	@echo "Options (make <target> VAR=val):"
	@echo "  NDEBUG=1     Release build (-O3 -flto -static)"
	@echo "  SANITIZE=1   AddressSanitizer + UBSan"
	@echo "  TARGET=...   Cross-compilation target suffix"
	@echo "  PREFIX=/usr/local  Install prefix"
	@echo "  BINDIR=\$${PREFIX}/bin  Install directory"
	@echo ""
	@echo "Cross-compilation (requires zig):"
	@echo "  make release-base"
	@echo "    Builds: x86_64-linux-musl, aarch64-linux-musl,"
	@echo "            x86_64-windows-gnu.exe, aarch64-macos"

$(OUTPUT): builddir $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(OUTPUT) $(OBJS) $(LDFLAGS)

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

builddir:
	mkdir -p $(BUILDDIR)

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

install: $(OUTPUT)
	install -Dm755 $(OUTPUT) $(DESTDIR)$(BINDIR)/$(EXE)

release-base: clean
	$(MAKE) NDEBUG=1 -j$(JOBS) \
		CC="zig cc -target x86_64-linux-musl" \
		CXX="zig c++ -target x86_64-linux-musl" \
		STRIP="zig llvm-strip" \
		TARGET="x86_64-linux-musl"
	$(MAKE) NDEBUG=1 -j$(JOBS) \
		CC="zig cc -target aarch64-linux-musl" \
		CXX="zig c++ -target aarch64-linux-musl" \
		STRIP="zig llvm-strip" \
		TARGET="aarch64-linux-musl"
	$(MAKE) NDEBUG=1 -j$(JOBS) \
		CC="zig cc -target x86_64-windows-gnu" \
		CXX="zig c++ -target x86_64-windows-gnu" \
		STRIP="zig llvm-strip" \
		TARGET="x86_64-windows-gnu.exe" OPT="-O3 -static"
	$(MAKE) NDEBUG=1 -j$(JOBS) \
		CC="zig cc -target aarch64-macos" \
		CXX="zig c++ -target aarch64-macos" \
		STRIP="zig llvm-strip" \
		TARGET="aarch64-macos" OPT="-O3"

release:
	podman build -t localhost/cxxbuilder buildtools/
	podman run --rm --userns=keep-id \
		-e HOME=/tmp \
		-e ZIG_GLOBAL_CACHE_DIR=/tmp/zig-global-cache \
		-e ZIG_LOCAL_CACHE_DIR=/tmp/zig-local-cache \
		-v ${PWD}:/src:Z localhost/cxxbuilder "/src/buildtools/podman-make-release"

.PHONY: help all clean distclean builddir release release-base strip install

-include $(DEPS)

COMPILER_PREFIX =
PLATFORM = unix
EXE = 3it

JOBS := $(shell nproc)

OUTPUT = build/$(EXE)

CC    = $(COMPILER_PREFIX)-gcc
CXX   = $(COMPILER_PREFIX)-g++
STRIP = $(COMPILER_PREFIX)-strip

ifeq ($(DEBUG),1)
OPT := -O0 -ggdb
else
OPT := -Os -static
endif

ifeq ($(SANITIZE),1)
OPT += -fsanitize=undefined
endif

CFLAGS = $(OPT) -Wall
CXXFLAGS = $(OPT) -Wall -std=c++17
CPPFLAGS ?= -MMD -MP

SRCS_C   := $(wildcard src/*.c)
SRCS_CXX := $(wildcard src/*.cpp)

BUILDDIR = build/$(PLATFORM)
OBJS := $(SRCS_C:src/%.c=$(BUILDDIR)/%.c.o)
OBJS += $(SRCS_CXX:src/%.cpp=$(BUILDDIR)/%.cpp.o)
DEPS  = $(OBJS:.o=.d)


all: $(OUTPUT)

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

builddir:
	mkdir -p $(BUILDDIR)

release:
	$(MAKE) -f Makefile -j$(JOBS) strip
	$(MAKE) -f Makefile.win32 -j$(JOBS) strip
	$(MAKE) -f Makefile.win64 -j$(JOBS) strip

.PHONY: clean builddir release

-include $(DEPS)

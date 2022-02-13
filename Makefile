COMPILER_PREFIX =
PLATFORM = unix
EXE = 3it

OUTPUT = build/$(EXE)

CC    = $(COMPILER_PREFIX)-gcc
CXX   = $(COMPILER_PREFIX)-g++
STRIP = $(COMPILER_PREFIX)-strip

OPT = -Os -static
#OPT = -O0 -g -fsanitize=address -ggdb -fno-omit-frame-pointer
CFLAGS = $(OPT) -Wall -MMD -MP
CXXFLAGS = $(OPT) -Wall -std=c++17 -MMD -MP

SRC_C   = $(wildcard src/*.c)
SRC_CXX = $(wildcard src/*.cpp)

BUILDDIR = build/$(PLATFORM)
OBJ =  $(SRC_C:src/%.c=$(BUILDDIR)/%.c.o)
OBJ += $(SRC_CXX:src/%.cpp=$(BUILDDIR)/%.cpp.o)
DEP =  $(SRC_CXX:src/%.cpp=$(BUILDDIR)/%.cpp.d)

all: $(OUTPUT)

$(OUTPUT): builddir $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(OUTPUT) $(OBJ)

strip: $(OUTPUT)
	$(STRIP) --strip-all $(OUTPUT)

builddir:
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/%.c.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%.cpp.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rfv build/

.PHONY: clean builddir

-include $(DEP)

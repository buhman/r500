OPT = -O0

CFLAGS += -g
CFLAGS += -Wall -Werror -Wfatal-errors
CFLAGS += -Wno-error=unused-variable
CFLAGS += -Wno-error=unused-but-set-variable
CFLAGS += -Wno-narrowing
CFLAGS += $(shell pkg-config --cflags libdrm)

LDFLAGS += $(shell pkg-config --libs libdrm) -lm

VERTEX_SHADERS = $(patsubst %.asm,%.bin,$(wildcard *.vs.asm))
FRAGMENT_SHADERS = $(patsubst %.asm,%.bin,$(wildcard *.fs.asm))
SHADER_BIN = $(VERTEX_SHADERS) $(FRAGMENT_SHADERS)

R500_COMMON = \
	r500/display_controller.o \
	r500/indirect_buffer.o \
	r500/shader.o \
	drm/buffer.o \
	drm/drm.o \
	file.o

matrix_cubesphere: $(R500_COMMON) matrix_cubesphere.o | shaders
	$(CXX) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(ARCH) $(CFLAGS) $(OPT) -c $< -o $@

%.o: %.cpp
	$(CXX) $(ARCH) $(CFLAGS) $(OPT) -c $< -o $@

%.vs.bin: %.vs.asm
	PYTHONPATH=../regs/ python -m assembler.vs $< $@

%.fs.bin: %.fs.asm
	PYTHONPATH=../regs/ python -m assembler.fs $< $@

%.vs.inc: %.vs.asm
	PYTHONPATH=../regs/ python -m assembler.vs $< > $@

%.fs.inc: %.fs.asm
	PYTHONPATH=../regs/ python -m assembler.fs $< > $@

shaders: $(SHADER_BIN)
	@true

#find . -type f ! -name "*.*" -delete
clean:
	find . -type f -name "*.o" -delete

.SUFFIXES:
.INTERMEDIATE:
.SECONDARY:
.PHONY: all clean phony

%: RCS/%,v
%: RCS/%
%: %,v
%: s.%
%: SCCS/s.%

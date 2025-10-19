OPT = -O0

CFLAGS += -g
CFLAGS += -Wall -Werror -Wfatal-errors
CFLAGS += $(shell pkg-config --cflags libdrm)

LDFLAGS += $(shell pkg-config --libs libdrm) -lm

%: %.c
	$(CC) $(ARCH) $(CFLAGS) $(LDFLAGS) $(OPT) $< -o $@

%.inc: %.asm
	PYTHONPATH=../regs/ python -m assembler $< > $@

clean:
	find . -type f ! -name "*.*" -delete

.SUFFIXES:
.INTERMEDIATE:
.SECONDARY:
.PHONY: all clean phony

%: RCS/%,v
%: RCS/%
%: %,v
%: s.%
%: SCCS/s.%

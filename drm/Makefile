OPT = -O0

CFLAGS += -g
CFLAGS += -Wall -Werror -Wfatal-errors -Wno-error=unused-variable -Wno-error=unused-but-set-variable -Wno-narrowing

LDFLAGS += $(shell pkg-config --cflags --libs libdrm) -lm

%: %.c
	$(CC) $(ARCH) $(CFLAGS) $(LDFLAGS) $(OPT) $< -o $@

%: %.cpp
	$(CXX) $(ARCH) $(CFLAGS) $(LDFLAGS) $(OPT) $< -o $@

%.vs.inc: %.vs.asm
	PYTHONPATH=../regs/ python -m assembler.vs $< > $@

%.fs.inc: %.fs.asm
	PYTHONPATH=../regs/ python -m assembler.fs $< > $@

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

# Native eZ80 regressions: host doubles cannot check the CE calling convention.
NAME = RPNDATE
DESCRIPTION = "RPNCE DATE regression tests"
COMPRESSED = YES
ARCHIVED = NO
CFLAGS = -Wall -Wextra -Oz -I../../src/core -I../../src/ui
EXTRA_C_SOURCES = $(wildcard ../../src/core/*.c) ../../src/ui/res.c ../../src/sys/brightness.c
include $(shell cedev-config --makefile)

$(BINDIR)/$(TARGET): $(BINDIR)/RPNCEres.8xv
$(BINDIR)/RPNCEres.8xv: ../../data/RPNCEres.bin
	$(Q)$(call MKDIR,$(@D))
	$(Q)$(CONVBIN) -r -n RPNCEres -j bin -i $< -k 8xv -o $@

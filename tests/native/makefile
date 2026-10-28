NAME = RPNTEST
DESCRIPTION = "rpnCE native tests"
COMPRESSED = YES
ARCHIVED = YES
CFLAGS = -Wall -Wextra -Oz -I../../src/core -I../../src/ui
EXTRA_C_SOURCES = $(wildcard ../../src/core/*.c) ../../src/sys/brightness.c \
    ../../src/ui/res.c ../../src/ui/display.c ../../src/ui/dotfont.c \
    ../../src/ui/menu.c ../../src/ui/actions.c ../../src/ui/catalog.c
include $(shell cedev-config --makefile)

$(BINDIR)/$(TARGET): $(BINDIR)/rpnCEres.8xv
$(BINDIR)/rpnCEres.8xv: ../../data/rpnCEres.bin
	$(Q)$(call MKDIR,$(@D))
	$(Q)$(CONVBIN) -r -n RPNCEres -j bin -i $< -k 8xv -o $@

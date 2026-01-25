# Makefile
CC        := gcc
CFLAGS    := -Werror -Wall -Wextra -O3 -std=c17 -DN_TRACE
BUILDDIR  := ../build
LIBDIR    := $(BUILDDIR)/lib
BINDIR    := $(BUILDDIR)/bin
INCLUDES  := -I../include -I../tools/include
CFLAGS    += $(INCLUDES)
LINKDIR   := -L$(BUILDDIR)/lib
LIBS      := tools
SUBDIRS   := strc url clip b64 calc rsh streq misc

UNAME     := $(shell uname -s)

ifeq ($(UNAME), Linux)
	SUBDIRS := $(SUBDIRS) gateway
endif

all: $(LIBS) $(SUBDIRS)

$(LIBS):
	@$(MAKE) -C $@ \
		CC="$(CC)" \
		CFLAGS="$(CFLAGS)" \
		BUILDDIR="$(BUILDDIR)" \
		LINKDIR="$(LINKDIR)" \
		INCLUDES="$(INCLUDES)" \
		LIBDIR="$(LIBDIR)" \
	  	BINDIR="$(BINDIR)"

$(SUBDIRS): $(LIBS)
	@$(MAKE) -C $@ \
		CC="$(CC)" \
		CFLAGS="$(CFLAGS)" \
		BUILDDIR="$(BUILDDIR)" \
		LINKDIR="$(LINKDIR)" \
		INCLUDES="$(INCLUDES)" \
		LIBDIR="$(LIBDIR)" \
	  	BINDIR="$(BINDIR)"

clean:
	@rm -rf build

.PHONY: all clean $(SUBDIRS) $(LIBS)
# Makefile
CC        := gcc
CFLAGS    := -Wextra -O3 -std=c17
BUILDDIR  := ../build
LIBDIR    := $(BUILDDIR)/lib
BINDIR    := $(BUILDDIR)/bin
INCLUDES  := -I./ -isystem ../include -isystem ../tools/include
CFLAGS    += $(INCLUDES)
LINKDIR   := -L$(BUILDDIR)/lib
LIBS      := tools
SUBDIRS   := strc url clip b64 calc rsh streq misc
MAKEFLAGS += --no-print-directory

UNAME     := $(shell uname -s)

ifeq ($(UNAME), Linux)
	SUBDIRS := $(SUBDIRS) gateway
	CFLAGS  := $(CFLAGS) -pthread \
				-D_XOPEN_SOURCE=700 \
				-D_POSIX_C_SOURCE=200809L \
				-D_GNU_SOURCE
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
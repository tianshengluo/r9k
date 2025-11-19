CC := gcc
CFLAGS := -Wall -O3 -std=c17
INCLUDE := -Itools/include

BUILDDIR := build
OBJDIR := $(BUILDDIR)/objs

TOOLS_DIR := tools
NSZ_DIR := nsz

TOOLS_SRCS := $(wildcard $(TOOLS_DIR)/*.c)
NSZ_SRCS := $(wildcard $(NSZ_DIR)/*.c)

TOOLS_OBJS := $(TOOLS_SRCS:$(TOOLS_DIR)/%.c=$(OBJDIR)/tools/%.o)
NSZ_OBJS := $(NSZ_SRCS:$(NSZ_DIR)/%.c=$(OBJDIR)/len/%.o)

NSZ_BIN := $(BUILDDIR)/len

TARGETS := $(NSZ_BIN)

all:$(TARGETS)

$(NSZ_BIN): $(NSZ_OBJS) $(TOOLS_OBJS)
	$(CC) $(CFLAGS) $(INCLUDE) $^ -o $@

$(OBJDIR)/tools/%.o: $(TOOLS_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

$(OBJDIR)/len/%.o: $(NSZ_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -rf $(BUILDDIR)

.PHONY: all clean
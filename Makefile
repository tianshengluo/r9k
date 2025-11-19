CC := gcc
DEFINES := -D_GNU_SOURCE
CFLAGS := -Wall -O3 -std=c17 $(DEFINES)
INCLUDE := -Itools/include

BUILDDIR := build
OBJDIR := $(BUILDDIR)/objs

TOOLS_DIR := tools
SIZ_DIR := siz

TOOLS_SRCS := $(wildcard $(TOOLS_DIR)/*.c)
SIZ_SRCS := $(wildcard $(SIZ_DIR)/*.c)

TOOLS_OBJS := $(TOOLS_SRCS:$(TOOLS_DIR)/%.c=$(OBJDIR)/tools/%.o)
SIZ_OBJS := $(SIZ_SRCS:$(SIZ_DIR)/%.c=$(OBJDIR)/siz/%.o)

SIZ_BIN := $(BUILDDIR)/siz

TARGETS := $(SIZ_BIN)

all:$(TARGETS)

$(SIZ_BIN): $(SIZ_OBJS) $(TOOLS_OBJS)
	$(CC) $(CFLAGS) $(INCLUDE) $^ -o $@

$(OBJDIR)/tools/%.o: $(TOOLS_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

$(OBJDIR)/siz/%.o: $(SIZ_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -rf $(BUILDDIR)

.PHONY: all clean
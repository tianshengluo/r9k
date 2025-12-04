# ns/Makefile
MODULE      := ns
SRCS        := $(shell find ./ -type f -name '*.c' | sed 's|^\.||')
OBJDIR      := $(BUILD)/obj/$(MODULE)
OBJS        := $(patsubst %.c,$(OBJDIR)/%.o,$(SRCS))
LINKS       := -ltools
BIN         := $(BINDIR)/$(MODULE)

all: $(BIN)

$(BIN): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(OBJS) $(CFLAGS) $(LINKDIR) $(LINKS) -o $@

$(OBJDIR)/%.o: %c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

.PHONY: all
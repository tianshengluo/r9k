# tools/Makefile
MODULE      := tools
SRCS        := $(shell find ./ -type f -name '*.c' | sed 's|^\./||')
OBJDIR      := $(BUILDDIR)/obj/$(MODULE)
OBJS        := $(patsubst %.c,$(OBJDIR)/%.o,$(SRCS))
LIB         := $(LIBDIR)/libtools.a
AR          := ar
ARFLAGS     := rcs

PRINTF 		:= printf "\033[0;32m[%-5s] %s %s\033[0m\n"

all: $(LIB)

$(LIB): $(OBJS)
	@mkdir -p $(dir $@)
	@$(PRINTF) "$(MODULE)" "AR" "$(notdir $@)"
	@$(AR) $(ARFLAGS) $@ $^

$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@$(PRINTF) "$(MODULE)" "CC" "$(notdir $<)"
	@$(CC) $(CFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

-include $(OBJS:.o=.d)

.PHONY: all clean
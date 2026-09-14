# ---- Toolchain ----
CCACHE  := $(shell command -v ccache 2>/dev/null)
CC      := $(CCACHE) gcc

# ---- Parallel build ----
NPROC     := $(shell nproc)
MAKEFLAGS += -j$(NPROC) --output-sync=target

CFLAGS  = -Wall -Wextra -g
LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

TARGET  = particle
SRCS    = $(wildcard *.c)
OBJS    = $(SRCS:.c=.o)
DEPS    = $(SRCS:.c=.d)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)

clean:
	rm -f $(TARGET) $(OBJS) $(DEPS)
	rm -f *.o *.d *.out core vgcore.* *.gcda *.gcno perf.data*

.PHONY: all clean
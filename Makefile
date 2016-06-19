OUT := asllog
SRC := $(wildcard *.c)
OBJS := ${SRC:.c=.o}

CFLAGS :=
LDFLAGS := 

include atv2-flags.mk

# see readme for sample local.mk
-include local.mk

all: $(OUT)

%.o: %.c
	$(CC) -c $< $(CFLAGS)

$(OUT): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(OUT) $(OBJS)

CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -g
LDFLAGS = -lpng -lm

SRC = src/vishash.c src/random.c src/image.c
OBJ = $(patsubst src/%.c,obj/%.o,$(SRC))
BIN = bin/vishash

.PHONY: all clean

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(BIN)

CC=gcc
CFLAGS=--std=c99 -Wall
LFLAGS=-lsqlite3

all: clock

release: CFLAGS += -O3 -DNDEBUG
release: clock

debug: CFLAGS += -DDEBUG -g
debug: clock

clock: clock.c task_utils.c tasks.c
	$(CC) -o $@ $^ $(CFLAGS) $(LFLAGS)

clean:
	rm clock .?*.db *.db


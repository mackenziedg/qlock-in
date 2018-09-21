CC=gcc
CFLAGS=--std=c99 -Wall
LFLAGS=-lsqlite3

all: qlock

release: CFLAGS += -O3 -DNDEBUG
release: qlock

debug: CFLAGS += -DDEBUG -g
debug: qlock

qlock: main.c task_utils.c tasks.c project.c
	$(CC) -o $@ $^ $(CFLAGS) $(LFLAGS)

test: test.c task_utils.c tasks.c project.c
	$(CC) -o $@ $^ $(CFLAGS) $(LFLAGS)

clean:
	rm -f qlock test .?*.db *.db


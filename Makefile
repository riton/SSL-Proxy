.PHONY: all

CFLAGS=-Wall
LDFLAGS=-lssl
DEBUG=-O0 -ggdb
CC=g++

all: proxys

OBJ=IOSelect.o IOSocket.o IOSocketSSL.o

proxys: proxys.o $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

.cpp.o:
	$(CC) $(DEBUG) $(CFLAGS) -c $<

clean:
	rm -f *.o
	rm -f proxys



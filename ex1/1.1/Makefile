CC=gcc
CFLAGS=-g -Wall -m32

all: zing zing2 

zing: main.o zing.o
	$(CC) $(CFLAGS) main.o zing.o -o zing

zing2: main.o zing2.o
	$(CC) $(CFLAGS) main.o zing2.o -o zing2
clean:
	rm zing2.o zing2 zing





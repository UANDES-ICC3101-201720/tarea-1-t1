CC=gcc
CFLAGS=-std=gnu11 -Wall -lpthread -lm

all: binsearch datagen

datagen: datagen.c 
	$(CC) -o datagen datagen.c $(CFLAGS)

binsearch: binsearch.c util.o
	$(CC) -o binsearch binsearch.c util.o $(CFLAGS)

util.o:
	$(CC) -c -o util.o util.c $(CFLAGS)

clean:
	rm datagen binsearch *.o
 

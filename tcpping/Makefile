CC=gcc
CFLAGS= -Wall -march=native -O3 -pipe -fPIC -DPIC -g
LDFLAGS= -z now -lrt

all: tcpping orig_tcpping

tcpping: tcpping.o
	$(CC) -o tcpping tcpping.o $(LDFLAGS)
tcpping.o: tcpping.c
	$(CC) -c tcpping.c -o tcpping.o $(CFLAGS)

orig_tcpping: orig_tcpping.o
	$(CC) -o orig_tcpping orig_tcpping.o $(LDFLAGS)
orig_tcpping.o: orig_tcpping.c
	$(CC) -c orig_tcpping.c -o orig_tcpping.o $(CFLAGS)

clean:
	rm -f *.o tcpping orig_tcpping

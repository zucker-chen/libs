CC=gcc
CFLAGS= -Wall -march=native -O3 -pipe -fPIC -DPIC -g
LDFLAGS= -z now -lrt

tcpping: tcpping.o
	$(CC) -o tcpping tcpping.o $(LDFLAGS)
tcpping.o: tcpping.c
	$(CC) -c tcpping.c -o tcpping.o $(CFLAGS)

clean:
	rm -f *.o tcpping

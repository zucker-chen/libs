

CC	= $(CROSS_PREFIX)gcc
CXX	= $(CROSS_PREFIX)g++
LD	= $(CROSS_PREFIX)ld
AR	= $(CROSS_PREFIX)ar
STRIP	= $(CROSS_PREFIX)strip
CFLAGS	:= -g -fPIC -Wall

all: liba testlib so_dynsym_chg

liba:
	$(CC) -c $(CFLAGS) a.c -o a.o
	$(AR) rcs liba.a a.o
	$(LD) -o liba.so a.o -shared
	$(STRIP) liba.so
	
testlib:
	$(CC) -o $@ testlib.c liba.so
	export LD_LIBRARY_PATH=.
	readelf -s liba.so
	
so_dynsym_chg:
	$(CXX) -std=c++11 -I. so_dynsym_chg.cpp -o so_dynsym_chg

so_test:
	export LD_LIBRARY_PATH=.
	./so_dynsym_chg -i liba.so -d 1 -s func_world -b 0
	readelf -s liba.so

	
clean:
	rm -rf *.o *.a *.so testlib so_dynsym_chg


#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "hello_world.h"


int main (int argc, char *argv[])
{
	if (argv[1] != NULL) {
		print_hello(argv[1]);
	}

	if (argv[2] != NULL) {
		print_world(argv[2]);
	}

	return 0;	
}

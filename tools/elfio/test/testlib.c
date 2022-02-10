#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>


#define pri_dbg(format, args...) fprintf(stderr,"%s %d %s() " format, __FILE__, __LINE__, __func__, ## args)


extern int func_hello(void);
extern int func_world(void);

int main (int argc, char *argv[])
{
	func_hello();
	func_world();

	return 0;
}




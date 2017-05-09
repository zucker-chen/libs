
/*
 *	zucker.chen 2017.5.7
 *	kernel print cmd: cat /proc/kmsg
 *	This demo can redirect the /dev/console to current tty(like telnet.)
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>


#define __54321_TEST_TTYS0_DEBUG_12345__
#ifdef __54321_TEST_TTYS0_DEBUG_12345__
#define pri_dbg(M, ...) fprintf(stderr,"%s %d %s(): " M "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define pri_dbg(M, ...) do{}while(0)
#endif




/*
 * console2stdout:	redirect console to stdout
 * swich: 0, off; 1, on
 */
static int console2stdout(int swich)
{
    int tty = -1, ret = -1;
    char *tty_name = NULL;

    /* get current tty */
    tty_name = ttyname(STDOUT_FILENO);

    if(1 == swich) {
        /* redirect the console to the tty */
        tty = open(tty_name, O_RDONLY | O_WRONLY);
        ret = ioctl(tty, TIOCCONS);
        if (ret < 0) {
			pri_dbg("[ERROR] STDOUT_FILENO: ioctl(tty, TIOCCONS), %s", strerror(errno));
		}
    }
    else if(0 == swich) {
        /* reset the console */
        tty = open("/dev/console", O_RDONLY | O_WRONLY);
        ret = ioctl(tty, TIOCCONS);
        if (ret < 0) {
			pri_dbg("[ERROR] /dev/console: ioctl(tty, TIOCCONS), %s", strerror(errno));
		}
    }
    else
    {
        pri_dbg("Invalid argument.");
        return 0;
    }

    close(tty);	
	
	return 0;
}



int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        pri_dbg("miss argument");
        return 0;
    }

	if (!strcmp(argv[1], "on")) {
		console2stdout(1);
	} else if (!strcmp(argv[1], "off")) {
		console2stdout(0);
	}
	
	
    return 0;
}




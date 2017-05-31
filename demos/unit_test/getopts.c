#include <unistd.h>
#include <stdio.h>
#include <string.h>
#ifdef __UCLIBC__
#include <bits/getopt.h>
#else
#include <getopt.h>
#include <sched.h>
#endif



int pack_mode = 0;
char chip[16];
char sensor[16];
int misc = 0;
char version[16];


#define NO_ARG		0
#define HAS_ARG		1
static struct option long_options[] = {
	{"pack", HAS_ARG, 0, 'p'},
	{"chip", HAS_ARG, 0, 'c'},
	{"sensor", HAS_ARG, 0, 's'},
	{"misc", HAS_ARG, 0, 'm'},
	{"version", HAS_ARG, 0, 'v'},
	{"help",   NO_ARG,  0, 'h'},
	{0, 0, 0, 0}
};

static const char *short_options = "p:c:s:m:v:h";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"pack mode", "\tpack or unpack, 0:pack, 1:unpack"},
	{"chip", "\tsoc chip type, like: s2lxx, 3516cxx, fh8830..."},
	{"sensor", "\tsensor type, like: imxxx, ovxx, arxx..."},
	{"misc", "\tnormal, wifi, sdcard, box?"},
	{"version", "\tfirmware version number, like: 6.1.10.1"},
	{"help",  "\tusage print"},
};




void usage(void)
{
	int i;

	printf("test_vin usage:\n");
	for (i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1; i++) {
		if (isalpha(long_options[i].val))
			printf("-%c ", long_options[i].val);
		else
			printf("   ");
		printf("--%s", long_options[i].name);
		if (hint[i].arg[0] != 0)
			printf(" [%s]", hint[i].arg);
		printf("\t%s\n", hint[i].str);
	}
	printf("\n");
}




int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {

		case 'p':
			pack_mode = atoi(optarg);
			if (pack_mode < 0)
				return -1;
			break;

		case 'c':
			strcpy(chip, optarg);
			break;

		case 's':
			strcpy(sensor, optarg);
			break;

		case 'm':
			misc = atoi(optarg);
			if (misc < 0)
				return -1;
			break;

		case 'v':
			strcpy(version, optarg);
			break;

		case 'h':
			usage();
			break;

		default:
			printf("unknown option found: %c\n", ch);
			return -1;
		}
	}

	return 0;
}



int main (int argc, char *argv[])
{
	if (init_param(argc, argv) < 0)
		return -1;

	printf("pack mode = %d\n", pack_mode);
	printf("chip = %s\n", chip);
	printf("sensor = %s\n", sensor);
	printf("misc = %d\n", misc);
	printf("version = %s\n", version);


	
	return 0;
}

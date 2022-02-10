
#include <stdio.h>

#include <iostream>
#include "elfio/elfio.hpp"



// ----------------------------------------------------------
#ifdef __UCLIBC__
#include <bits/getopt.h>
#else
#include <getopt.h>
#include <sched.h>
#endif

static char arg_infile_name[128];
static char arg_outfile_name[128];
static char arg_symbol_name[64];
static int arg_bind;
static int arg_debug;

#define NO_ARG		0
#define HAS_ARG		1
static struct option long_options[] = {
	{"infile", HAS_ARG, 0, 'i'},
	{"outfile", HAS_ARG, 0, 'o'},
	{"symbol", HAS_ARG, 0, 's'},
	{"bind", HAS_ARG, 0, 'b'},
	{"debug", HAS_ARG, 0, 'd'},
	{"help",   NO_ARG,  0, 'h'},
	{0, 0, 0, 0}
};

static const char *short_options = "i:o:s:b:d:h";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"infile", "\tinput file name, like: a.so"},
	{"outfile", "\toutput file name, like: b.so"},
	{"symbol", "\tsymbol name for set"},
	{"bind", "\tset so .dynsym symbol bind, 0:LOCAL, 1:GLOBAL"},
	{"debug",  "\tdebug print on"},
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
		case 'i':
			strcpy(arg_infile_name, optarg);
			break;
		case 'o':
			strcpy(arg_outfile_name, optarg);
			break;
		case 's':
			strcpy(arg_symbol_name, optarg);
			break;
		case 'b':
			arg_bind = atoi(optarg);
			break;
		case 'd':
			arg_debug = atoi(optarg);
			break;
		case 'h':
			usage();
			break;
		default:
			printf("unknown option found: %c\n", ch);
			return -1;
		}
	}

	if (strlen(arg_infile_name) <= 0) {
	printf("1\n");
		usage();
		return -1;
	}
	if (strlen(arg_outfile_name) <= 0) {
		strcpy(arg_outfile_name, arg_infile_name);
	}
	if (arg_debug != 1) {
		if (strlen(arg_symbol_name) <= 0) {
	printf("2\n");
			usage();
			return -1;
		}
		if (arg_bind < 0 && arg_bind > 1) {
	printf("3\n");
			usage();
			return -1;
		}
	}
	
	return 0;
}

// ----------------------------------------------------------



using namespace ELFIO;

static void so_dynsym_print(elfio &reader)
{
	// '.dynsym'
    section* psec = reader.sections[".dynsym"];
	const symbol_section_accessor symbols( reader, psec );
	for ( unsigned int j = 0; j < symbols.get_symbols_num(); ++j ) 
	{
		std::string   name;
		Elf64_Addr    value;
		Elf_Xword     size;
		unsigned char bind;
		unsigned char type;
		Elf_Half      section_index;
		unsigned char other;

		// Read symbol properties
		symbols.get_symbol( j, name, value, size, bind, type, section_index, other );
		//std::cout << j << "\t" << name << "\tbind" << bind << std::endl;
		printf("%d,\t%s,\t%d\t%d\n", j, name.c_str(), bind, section_index);
	}
}

static bool so_dynsym_symbol_bind_set(elfio &reader, const std::string& name, int bind)
{
	Elf64_Addr    value;
	Elf_Xword     size;
	unsigned char cur_bind;
	unsigned char type;
	Elf_Half      section_index;
	unsigned char other;

    section* psec = reader.sections[".dynsym"];
	symbol_section_accessor symbols( reader, psec );
	symbols.get_symbol( name, value, size, cur_bind, type, section_index, other );
	
	string_section_accessor str_writer( psec );
	bool ret = symbols.set_symbol( name.c_str(), value, size, bind, type, section_index, other );
	//printf("%s,\t%d, %d\n", name.c_str(), bind, ret);
	//reader.save( "./a.so" );
	
	return ret;
}

int main( int argc, char** argv )
{
	
	if (init_param(argc, argv) < 0) {
		return -1;
	}
	
    // Create an elfio reader
    elfio reader;

    // Load ELF data
    if ( !reader.load( arg_infile_name ) ) {
        std::cout << "Can't find or process ELF file " << argv[1] << std::endl;
        return 2;
    }

	if (arg_debug) {
		// Print ELF file properties
		std::cout << "ELF file class    : ";
		if ( reader.get_class() == ELFCLASS32 ) {
			std::cout << "ELF32" << std::endl;
		} else {
			std::cout << "ELF64" << std::endl;
		}
		std::cout << "ELF file encoding : ";
		if ( reader.get_encoding() == ELFDATA2LSB ) {
			std::cout << "Little endian" << std::endl;
		} else {
			std::cout << "Big endian" << std::endl;
		}

		so_dynsym_print(reader);
	}
	
	
	bool ret = so_dynsym_symbol_bind_set(reader, arg_symbol_name, arg_bind);
	if (ret != true) {
		std::cout << "so_dynsym_symbol_bind_set error." << std::endl;
		return -1;
	}
	reader.save( arg_outfile_name );

	
	return 0;
}
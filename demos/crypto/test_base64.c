#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "base64.h"


#define BASE64_BLOCK_SIZE	12		// must be 3byte align


int main (int argc, char *argv[])
{
	int len, crypt_mode;
	unsigned char in_data[BASE64_BLOCK_SIZE], out_data[(BASE64_BLOCK_SIZE+3)*4/3 + 1];
	FILE *input_file, *output_file;

	if (argc != 4) {
		printf("Usage: %s <input file> <output file> e.g: %s 1.txt 2.txt 0\n", argv[0], argv[0]);
		return -1;
	}
	
	// Open input file
	input_file = fopen(argv[1], "rb");
	if (!input_file) {
		printf("Could not open input file to read data.");
		return 1;
	}
	// Open output file
	output_file = fopen(argv[2], "wb");
	if (!output_file) {
		printf("Could not open output file to write data.");
		return 1;
	}
	// Set crypt mode
	crypt_mode = atoi(argv[3]);

	while((len = fread(in_data, 1, BASE64_BLOCK_SIZE, input_file)) > 0) {
		//printf("len = %d crypt_mode = %d\n", len, crypt_mode);
		if (crypt_mode == 0) {
			base64_encode(&in_data[0], &out_data[0], &len);
		} else {
			base64_decode(&in_data[0], &out_data[0], &len);
		}		
		fwrite(out_data, 1, len, output_file);
	}

	fclose(input_file);
	fclose(output_file);

	return 0; 
}


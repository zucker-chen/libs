#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "des.h"

#define DES_KEY_SIZE 8
#define DES_BLOCK_SIZE	16		// must be 8byte align


int main (int argc, char *argv[])
{
	int ret, len, is_relay, crypt_mode;
	unsigned char in_data[DES_BLOCK_SIZE], out_data[DES_BLOCK_SIZE+8];
	FILE *input_file, *output_file;
	unsigned char des_key[DES_KEY_SIZE] = "12345678";

	if (argc != 4) {
		printf("Usage: %s <input file> <output file> <crypt mode>  e.g: %s 1.txt 2.txt 0\n", argv[0], argv[0]);
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
	
	//desb_generate_key(des_key);

	while((len = fread(in_data, 1, DES_BLOCK_SIZE, input_file)) > 0) {
		is_relay = len < DES_BLOCK_SIZE ? 0 : 1;
		//printf("len = %d is_relay = %d crypt_mode = %d\n", len, is_relay, crypt_mode);
		if (crypt_mode == 0) {
			ret = des_encode(&in_data[0], &out_data[0], des_key, &len, is_relay);
			if (ret < 0) {
				break;
			}
		} else {
			ret = des_decode(&in_data[0], &out_data[0], des_key, &len, is_relay);
			if (ret < 0) {
				break;
			}
		}		
		ret = fwrite(out_data, 1, len, output_file);
	}

	
	fclose(input_file);
	fclose(output_file);
	

	return 0; 
}


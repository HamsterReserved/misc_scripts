/**
 * XOR Decrypt
 *
 * Usage: ./xor-decrypt <HEX STRING IN UPPER CASE> <ASCII KEY>
 * Will treat output as string and printf it.
 *
 * Example: ./xor-decrypt A3B212A0 test
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* does not handle symbols other than [0-9A-Za-z], will produce errorneous output with those */
static unsigned char char2hex(const char* str) {
#define LOWER2HEX(digit) (((digit) >= 'a') ? (10 + ((digit) - 'a')) : ((digit) - '0'))
    const char digit0 = tolower(str[0]);
    const char digit1 = tolower(str[1]);

    if (digit1 == 0) {
        return LOWER2HEX(digit0);
    }

    return 16 * LOWER2HEX(digit0) + LOWER2HEX(digit1);
}

int hex_str_to_bin(const char* in_str, int str_len, unsigned char *out_buf, int max_convert_len)
{
	int converted_bytes = 0;
	for (int i = 0; i < str_len / 2 && i < max_convert_len; ++i) {
		unsigned char number = char2hex(in_str + 2 * i);
	    out_buf[i] = number;
	    ++converted_bytes;
	}
	return converted_bytes;
}

int main(int argc, char* argv[]) {
	if (argc < 3) {
		fprintf(stderr, "No enough parameters. Need at least 2.");
		return -1;
	}
	unsigned char encrypted_buf[512];
	char decrypted_buf[256];
	memset(encrypted_buf, 0, sizeof(encrypted_buf));
	memset(decrypted_buf, 0 ,sizeof(decrypted_buf));

	int actual_len = hex_str_to_bin(argv[1], strnlen(argv[1], 1024), encrypted_buf, sizeof(encrypted_buf));
	char* key = argv[2];
	for (int i = 0; i < actual_len; ++i)
        decrypted_buf[i] = encrypted_buf[i] ^ key[i % strlen(key)];
    printf("%s\n", decrypted_buf);
}
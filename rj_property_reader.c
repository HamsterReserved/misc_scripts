#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#define TRUE 1
#define FALSE 0

typedef struct _rj_property_header {
    uint8_t magic; // 0x1a
    uint8_t header_type; // guessed
    uint8_t magic_2[4]; // 0x00, 0x00, 0x13, 0x11
    uint8_t type;
    uint8_t len; // including type and len, thus content size is len - 2
} rj_prop_header;

//#define DEBUG
#ifdef DEBUG
#define PRINT_DBG(...) fprintf(stdout, __VA_ARGS__)
#else
#define PRINT_DBG(...)
#endif

#define PRINT_INFO(...) fprintf(stdout, __VA_ARGS__)
#define PRINT_ERR(...) fprintf(stderr, __VA_ARGS__)

static void format_hex_string(char* out, const uint8_t* src, int len, int bytes_per_line, int append_space) {
    int i = 0;
    char print_buf[4] = {0, 0, 0, 0};
    char digit;

    out[0] = 0;

    if (append_space)
        print_buf[2] = ' ';

    for (; i < len; ++i) {
        digit = (src[i] >> 4) & 0xf;
        print_buf[0] = digit > 9 ? 'a' + digit - 10 : '0' + digit;

        digit = src[i] & 0xf;
        print_buf[1] = digit > 9 ? 'a' + digit - 10 : '0' + digit;

        strcat(out, print_buf);
        if (i + 1 != len && (i + 1) % bytes_per_line == 0)
            strcat(out, "\n");
    }
}

static void print_header(const rj_prop_header* header) {
    char hex_buf[15] = {0};
    format_hex_string(hex_buf, header->magic_2, 4, 16, TRUE);
    PRINT_INFO("INFO:"
           "\theader->magic = 0x%02hhx\n"
           "\theader->header_type = 0x%02hhx\n"
           "\theader->magic_2 = %s\n"
           "\theader->type = 0x%02hhx\n"
           "\theader->len = 0x%02hhx (including type and len, actual size = 0x%02hhx)\n",
           header->magic,
           header->header_type,
           hex_buf,
           header->type,
           header->len, header->len - 2
           );
}

/*
 * Check if the header is valid by comparing the magics
 * Return 1 for true (valid), 0 for false (invalid)
 */
static int is_valid_header(const rj_prop_header* header) {
    uint8_t magic_2_exp[4] = {0, 0, 0x13, 0x11};

    if (header->magic != 0x1a) {
        PRINT_DBG("ERROR: not valid header: header[0] is 0x%02hhx instead of 0x1a\n", header->magic);
        goto err;
    }

    if (memcmp(magic_2_exp, header->magic_2, 4) != 0) {
        PRINT_DBG("ERROR: not valid header: header[2-5] is {0x%02hhx, 0x%02hhx, 0x%02hhx, 0x%02hhx}"
                    " instead of {0x00, 0x00, 0x13, 0x11}\n",
                    header->magic_2[0], header->magic_2[1], header->magic_2[2], header->magic_2[3]);
        goto err;
    }

    return 1;
err:
    return 0;
}

int main(int argc, char* argv[]) {
    uint8_t* file_buf; // Mind your file size...
    struct stat file_stat;
    FILE* fp;

    uint8_t* curr_pos;
    uint8_t* curr_max;
    char hex_buf[1000] = {0};

    if (argc < 2) {
        PRINT_ERR("ERROR: You must specify one file to analyze!\n");
        return EINVAL;
    }

    if (stat(argv[1], &file_stat) < 0) {
        perror("Unable to open file");
        return errno;
    }

    fp = fopen(argv[1], "rb");
    if (fp < 0) {
        perror("Unable to read file");
        return errno;
    }

    file_buf = (uint8_t*)malloc(file_stat.st_size);
    if (file_buf == NULL) {
        perror("Unable to allocate memory for the file. Maybe your file is too big.");
        return errno;
    }

    if (fread(file_buf, file_stat.st_size, 1, fp) < 1) {
        perror("Unable to read all data from file");
        return errno;
    }

    curr_pos = file_buf;
    curr_max = file_buf + file_stat.st_size - sizeof(rj_prop_header);

    for (;curr_pos <= curr_max; ++curr_pos) {
        PRINT_DBG("INFO: Trying 0x%x\n", curr_pos - file_buf);
        if (is_valid_header((rj_prop_header*)curr_pos)) {
            PRINT_INFO("INFO: A valid header found at 0x%x\n", curr_pos - file_buf);
            print_header((rj_prop_header*)curr_pos);

            hex_buf[0] = 0;
            format_hex_string(hex_buf, curr_pos + sizeof(rj_prop_header), ((rj_prop_header*)curr_pos)->len - 2, 16, TRUE);
            PRINT_INFO("\tcontent(hex) =\n%s\n", hex_buf);
            PRINT_INFO("\tcontent(ASCII) =\n%.*s\n", ((rj_prop_header*)curr_pos)->len - 2, curr_pos + sizeof(rj_prop_header));
            PRINT_INFO("\n");
        }
    }

    return 0;
}

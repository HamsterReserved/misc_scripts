//
//  main.c
//  KLineExtractor
//
//  Created by Hamster on 16/8/20.
//  Copyright © 2016年 Hamster. All rights reserved.
//

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define SIZE_1M (1024 * 1024)
#define BUF_SIZE SIZE_1M
#define PATTERN_LENGTH 4
#define PATTERN_OFFSET 0

// Little endian!
typedef struct _minute_data {
    uint32_t day;
    uint32_t time;
    uint32_t open_price;
    uint32_t high_price;
    uint32_t low_price;
    uint32_t close_price;
    uint32_t stub_1; //
    uint32_t VOL;
    uint32_t stub_2[5]; //
    uint32_t stub_value; //
    uint32_t stub_3;
    uint32_t OPI;
    uint32_t stub_4[5]; //
} minute_data_t;

typedef struct _config {
    const char* src_path;
    const char* out_path;
    const char* pattern_hex;
    int raw_output;
} config_t;

config_t g_config;

void write_header(FILE* fp) {
    fprintf(fp, "day,time,open,high,low,close,VOL,stub,OPI\n");
}

void write_date_time(FILE* fp, const char* delim, int value) {
    // 20160101 -> 2016 01 01
    // 180500 -> 18 05 00
    int part2 = value % 10000; // rough
    int part1 = (value - part2) / 10000;
    int part3 = part2 % 100;
    part2 = (part2 - part3) / 100;
    
    fprintf(fp, "%02d%s%02d%s%02d,", part1, delim, part2, delim, part3);
}

void write_decimal(FILE* fp, int value) {
    // 943500 -> 94.3500
    int part2 = value % 10000;
    int part1 = (value - part2) / 10000;
    
    fprintf(fp, "%d.%d,", part1, part2);
}

void dump_data(FILE* fp, minute_data_t* data) {
    if (data->day != 0xFFFFFFFF && data->day != 0x00000000) {
        if (g_config.raw_output) {
            fprintf(fp, "%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
                    data->day, data->time, data->open_price,
                   data->high_price, data->low_price, data->close_price,
                   data->VOL, data->stub_value, data->OPI);
        } else {
            // Format it
            write_date_time(fp, "/", data->day);
            write_date_time(fp, ":", data->time);
            write_decimal(fp, data->open_price);
            write_decimal(fp, data->high_price);
            write_decimal(fp, data->low_price);
            write_decimal(fp, data->close_price);
            fprintf(fp, "%d,%d,%d\n", data->VOL, data->stub_value, data->OPI);
        }
    }
}

size_t search_pattern_in_file(FILE* fp, uint8_t* pattern, int pattern_len) {
    // This is rather naiive! Don't do this as silly me!
    uint8_t* buf = malloc(BUF_SIZE);
    // Restore original postion
    size_t org_pos = ftell(fp);
    int i;
    
    size_t curr_pos = 0;
    size_t actual_read = 0;
    
    fseek(fp, 0, SEEK_SET);
    
    // Compare every byte until we find it
    while (!feof(fp)) {
        actual_read = fread(buf, 1, BUF_SIZE, fp);
        for (i = 0; i <= actual_read - pattern_len; i++) {
            if (memcmp(buf + i, pattern, pattern_len) == 0) {
                fseek(fp, org_pos, SEEK_SET);
                free(buf);
                return curr_pos + i;
            }
        }
        curr_pos += BUF_SIZE;
    }
    
    fseek(fp, org_pos, SEEK_SET);
    free(buf);
    return -1;
}

int alpha2num(char alpha) {
    if (alpha >= '0' && alpha <= '9')
        return alpha - '0';
    else if (alpha >= 'A' && alpha <= 'F')
        return alpha - 'A' + 10;
    else if (alpha >= 'a' && alpha <= 'f')
        return alpha - 'a' + 10;
    else
        return 0;
}

uint8_t* build_pattern() {
    uint8_t* pattern = malloc(PATTERN_LENGTH);
    int i;
    
    for (i = 0; i < PATTERN_LENGTH; i++) {
        pattern[i] = alpha2num(g_config.pattern_hex[i * 2]) * 16;
        pattern[i] += alpha2num(g_config.pattern_hex[i * 2 + 1]);
    }
    
    return pattern;
}

void process_file() {
    FILE* fp_in = fopen(g_config.src_path, "rb");
    FILE* fp_out = fopen(g_config.out_path, "w");
    if (fp_in == NULL) {
        perror("Error opening file for reading, exiting");
        exit(1);
    }
    
    if (fp_out == NULL) {
        perror("Error opening file for writing, writing to stdout");
        fp_out = stdout;
    }
    
    uint8_t* pattern = build_pattern();
    size_t pos = search_pattern_in_file(fp_in, pattern, PATTERN_LENGTH) - PATTERN_OFFSET;
    
    if (pos == -1) {
        // Not found
        fprintf(stderr, "Cannot found pattern data, exiting\n");
        return;
    }
    
    fseek(fp_in, pos, SEEK_SET);
    
    write_header(fp_out);
    
    minute_data_t minute_data;
    while (!feof(fp_in)) {
        size_t actual_read = fread(&minute_data, 1, sizeof(minute_data), fp_in);
        
        if (actual_read < sizeof(minute_data)) {
            fprintf(stderr, "Warning: could not read enough data to fill the struct. Skipped\n");
        } else {
            dump_data(fp_out, &minute_data);
        }
    }
}

int main(int argc, const char * argv[]) {
    int curr_arg;
    
    g_config.src_path = NULL;
    g_config.out_path = NULL;
    g_config.pattern_hex = NULL;
    g_config.raw_output = 0;
    
#define CURR_ARG_STR (argv[curr_arg])
    for (curr_arg = 1; curr_arg < argc; curr_arg++) {
        if (strcmp(CURR_ARG_STR, "-r") == 0) {
            g_config.raw_output = 1;
        } else if (CURR_ARG_STR[0] != '-') {
            // Not a parameter (not begin with -), treat as the file path
            // first one appeared is source file, second one is output file, third one is hex
            if (g_config.src_path == NULL)
                g_config.src_path = CURR_ARG_STR;
            else if (g_config.out_path == NULL)
                g_config.out_path = CURR_ARG_STR;
            else
                g_config.pattern_hex = CURR_ARG_STR;
        } else {
            printf("Error: unknown parameter: %s\n", CURR_ARG_STR);
            printf("Usage: this_program <source> <destination_csv> <start_hex (4bytes, uppercase)> [-r]\n");
            return 1;
        }
    }
    
    if (g_config.pattern_hex == NULL ||
            strnlen(g_config.pattern_hex, 8) != 8) {
        printf("Error: pattern should be 4 bytes long (8 characters without any space)\n");
        return 1;
    }
    
    process_file();
    
    return 0;
}

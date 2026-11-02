#pragma once

#include <stddef.h>

typedef struct {
    char  *buf;
    size_t len;
    size_t cap;
    int    err;
} md_out;

void cmd_help();
void mdparse(const char *src, md_out *o);
void cmd_version();
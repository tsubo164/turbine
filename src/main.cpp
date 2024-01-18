//#include <string_view>
//#include <iostream>
//#include <fstream>
//#include <string>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "interpreter.h"

typedef struct strbuf {
    char *data;
    int len;
    int cap;
} strbuf;

// calculate new alloc size based on the current alloc and
// requested new length.
// the new minimum alloc: 16 bytes
// grows 2x until 8k bytes e.g. 16 -> 32 -> ... -> 1024
// adds 1024 bytes until it covers new length
static int grow_cap(int old_cap, int new_len)
{
    int new_cap = old_cap < 16 ? 16 : old_cap;

    while (new_cap < new_len + 1 && new_cap < 1024 * 8)
        new_cap *= 2;
    while (new_cap < new_len + 1)
        new_cap += 1024;

    return new_cap;
}

static void grow(strbuf *sb, int new_len)
{
    if (!sb)
        return;

    if (sb->cap >= new_len + 1) {
        // no reallocation
        sb->len = new_len;
        sb->data[sb->len] = '\0';
        return;
    }

    int new_cap = grow_cap(sb->cap, new_len);
    char *new_data = (char *) realloc(sb->data, new_cap);
    if (!new_data)
        return;

    sb->data = new_data;
    sb->cap = new_cap;
    sb->len = new_len;
}

void strcat(strbuf *sb, const char *s)
{
    if (!sb || !s)
        return;

    int len = strlen(s);
    int old_len = sb->len;
    int new_len = sb->len + len;

    grow(sb, new_len);
    memcpy(sb->data + old_len, s, len + 1);
}

const char *read_file(const char *filename)
{
    FILE *fp = fopen(filename, "r");

    if (!fp)
        return NULL;

    char buf[1024] = {'\0'};
    strbuf sb = {0};
    while (fgets(buf, 1024, fp)) {
        strcat(&sb, buf);
    }
    strcat(&sb, "\n");

    return sb.data;
}

int main(int argc, char **argv)
{
    const char *filename = NULL;
    Option opt;

    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];

        if (!strcmp(arg, "--print-token") || !strcmp(arg, "-k")) {
            opt.print_token = true;
        }
        else if (!strcmp(arg, "--print-token-raw") || !strcmp(arg, "-K")) {
            opt.print_token = true;
            opt.print_token_raw = true;
        }
        else if (!strcmp(arg, "--print-tree") || !strcmp(arg, "-t")) {
            opt.print_tree = true;
        }
        else if (!strcmp(arg, "--print-symbols") || !strcmp(arg, "-y")) {
            opt.print_symbols = true;
        }
        else if (!strcmp(arg, "--print-symbols-all") || !strcmp(arg, "-Y")) {
            opt.print_symbols = true;
            opt.print_symbols_all = true;
        }
        else if (!strcmp(arg, "--print-bytecode") || !strcmp(arg, "-b")) {
            opt.print_bytecode = true;
        }
        else if (!strcmp(arg, "--print-stack") || !strcmp(arg, "-s")) {
            opt.print_stack = true;
        }
        else if (!strcmp(arg, "--disable-optimize") || !strcmp(arg, "-d")) {
            opt.enable_optimize = false;
        }
        else if (arg[0] == '-') {
            fprintf(stderr, "error: unknown option: %s\n", arg);
            exit(EXIT_FAILURE);
        }
        else {
            filename = arg;

            if (i != argc - 1) {
                fprintf(stderr, "error: unknown argument after filename\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    const char *src = read_file(filename);

    if (!src) {
        fprintf(stderr, "error: no such file: %s\n", filename);
        exit(EXIT_FAILURE);
    }

    int ret = Interpret(src, &opt);
    if (opt.print_token || opt.print_tree || opt.print_bytecode || opt.print_symbols)
        ret = 0;

    free((char *)src);

    return ret;
}

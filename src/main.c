#include "interpreter.h"
#include "strbuf.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

const char *read_file(const char *filename)
{
    FILE *fp = fopen(filename, "r");

    if (!fp)
        return NULL;

    char buf[1024] = {'\0'};
    Strbuf sb = {0};
    while (fgets(buf, 1024, fp)) {
        StrbufCat(&sb, buf);
    }
    StrbufCat(&sb, "\n");

    fclose(fp);
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

    int ret = Interpret(src, filename, &opt);
    if (opt.print_token || opt.print_tree || opt.print_bytecode || opt.print_symbols)
        ret = 0;

    free((char *)src);

    return ret;
}

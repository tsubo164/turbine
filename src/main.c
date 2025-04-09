#include "interpreter.h"
#include "data_strbuf.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

const char *read_file(const char *filename)
{
    FILE *fp = fopen(filename, "r");

    if (!fp)
        return NULL;

    char buf[1024] = {'\0'};
    struct data_strbuf sb = DATA_STRBUF_INIT;
    while (fgets(buf, 1024, fp)) {
        data_strbuf_cat(&sb, buf);
    }
    data_strbuf_cat(&sb, "\n");

    fclose(fp);
    return sb.data;
}

static void print_version(void)
{
    printf("Turbine 0.1.0\n");
}

int main(int argc, char **argv)
{
    struct interpreter_option opt = {0};
    struct interpreter_args args = {0};

    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];

        if (!strcmp(arg, "--version") || !strcmp(arg, "-v")) {
            print_version();
            return 0;
        }
        else if (!strcmp(arg, "--print-token") || !strcmp(arg, "-k")) {
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
        else if (arg[0] == '-') {
            fprintf(stderr, "error: unknown option: %s\n", arg);
            exit(EXIT_FAILURE);
        }
        else {
            args.filename = arg;
            args.values = (const char **)(argv + i);
            args.count = argc - i;
            break;
        }
    }

    const char *src = read_file(args.filename);

    if (!src) {
        fprintf(stderr, "error: no such file: %s\n", args.filename);
        exit(EXIT_FAILURE);
    }

    int64_t ret = interpret_source(src, &args, &opt);
    if (opt.print_token || opt.print_tree || opt.print_bytecode || opt.print_symbols)
        ret = 0;

    free((char *)src);

    return ret;
}

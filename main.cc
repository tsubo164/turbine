#include <iostream>
#include <sstream>
#include <cctype>

static void scan_number(std::istream &strm, int first_char, long &ival)
{
    static char buf[256] = {'\0'};
    char *pbuf = buf;

    for (int ch = first_char; isdigit(ch); ch = strm.get()) {
        *pbuf++ = ch;
    }
    strm.unget();

    *pbuf = '\0';
    pbuf = buf;

    char *end = NULL;
    ival = strtol(buf, &end, 10);
}

int main(int argc, char **argv)
{
    if (argc != 2)
        return -1;

    std::stringstream strm(argv[1]);

    long ival = 0;
    int ch = strm.get();

    scan_number(strm, ch, ival);
    printf("%ld\n", ival);

    return 0;
}

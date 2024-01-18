#include "test.h"
#include <stdlib.h>
#include <stdio.h>

static int test_count = 0;

void AssertI(int expected, int actual, int line)
{
    test_count++;

    if (expected != actual) {
        fprintf(stderr, "error:%d: expected: %d actual: %d\n", line, expected, actual);
        exit(1);
    }
}

void AssertL(long expected, long actual, int line)
{
    test_count++;

    if (expected != actual) {
        fprintf(stderr, "error:%d: expected: %ld actual: %ld\n", line, expected, actual);
        exit(1);
    }
}

void AssertS(const char *expected, const char *actual, int line)
{
    test_count++;

    if (expected != actual) {
        fprintf(stderr, "error:%d: expected: \"%s\" actual: \"%s\"\n",
                line, expected, actual);
        exit(1);
    }
}

int GetTestCount()
{
    return test_count;
}

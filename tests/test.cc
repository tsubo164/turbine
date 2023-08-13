#include "test.h"
#include <iostream>

static int test_count = 0;

void AssertI(int expected, int actual, int line)
{
    test_count++;

    if (expected != actual) {
        printf("error: expected: %d actual: %d [%d]\n", expected, actual, line);
        exit(1);
    }
}

void AssertL(long expected, long actual, int line)
{
    test_count++;

    if (expected != actual) {
        printf("error: expected: %ld actual: %ld [%d]\n", expected, actual, line);
        exit(1);
    }
}

void AssertS(const std::string &expected, const std::string &actual, int line)
{
    test_count++;

    if (expected != actual) {
        printf("error: expected: \"%s\" actual: \"%s\" [%d]\n",
                expected.c_str(), actual.c_str(), line);
        exit(1);
    }
}

int GetTestCount()
{
    return test_count;
}

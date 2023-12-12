#include "test.h"
#include <iostream>

static int test_count = 0;

void AssertI(int expected, int actual, int line)
{
    test_count++;

    if (expected != actual) {
        printf("error:%d: expected: %d actual: %d\n", line, expected, actual);
        exit(1);
    }
}

void AssertL(long expected, long actual, int line)
{
    test_count++;

    if (expected != actual) {
        printf("error:%d: expected: %ld actual: %ld\n", line, expected, actual);
        exit(1);
    }
}

void AssertS(const std::string &expected, const std::string &actual, int line)
{
    test_count++;

    if (expected != actual) {
        printf("error:%d: expected: \"%s\" actual: \"%s\"\n",
                line, expected.c_str(), actual.c_str());
        exit(1);
    }
}

int GetTestCount()
{
    return test_count;
}

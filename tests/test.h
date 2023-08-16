#ifndef TEST_H
#define TEST_H

#include <string>

void AssertI(int expected, int actual, int line);
void AssertL(long expected, long actual, int line);
void AssertS(const std::string &expected, const std::string &actual, int line);

#define ASSERTI(expected, actual) AssertI((expected), (actual), __LINE__)
#define ASSERTL(expected, actual) AssertL((expected), (actual), __LINE__)
#define ASSERTS(expected, actual) AssertS((expected), (actual), __LINE__)
#define ASSERTK(expected, actual) \
    AssertI(static_cast<int>(expected), static_cast<int>(actual), __LINE__)

int GetTestCount();

#endif // _H

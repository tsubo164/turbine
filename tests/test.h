#ifndef TEST_H
#define TEST_H

void AssertI(int expected, int actual, int line);
void AssertL(long expected, long actual, int line);
void AssertS(const char *expected, const char *actual, int line);

#define ASSERTI(expected, actual) AssertI((expected), (actual), __LINE__)
#define ASSERTL(expected, actual) AssertL((expected), (actual), __LINE__)
#define ASSERTS(expected, actual) AssertS((expected), (actual), __LINE__)

int GetTestCount();

#endif // _H

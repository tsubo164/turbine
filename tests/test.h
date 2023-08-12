#ifndef TEST_H
#define TEST_H

void AssertI(int expected, int actual, int line);
void AssertL(long expected, long actual, int line);

#define ASSERTI(expected, actual) AssertI((expected), (actual), __LINE__)
#define ASSERTL(expected, actual) AssertL((expected), (actual), __LINE__)

int GetTestCount();

#endif // _H

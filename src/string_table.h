#ifndef STRING_TABLE_H
#define STRING_TABLE_H

#include <string>
#include <set>

using SharedStr = const char*;

class StringTable {
public:
    StringTable();
    ~StringTable();

    SharedStr Insert(const std::string &str);

private:
    struct cstrcmp {
        bool operator()(const char *a, const char *b) const {
            return std::strcmp(a, b) < 0;
        }
    };

    std::set<const char*, cstrcmp> cstrset_;
};

#endif // _H

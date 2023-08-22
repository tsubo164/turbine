#include "string_table.h"

StringTable::StringTable()
{
    // emptry string for ID 0
    Insert("");
}

StringTable::~StringTable()
{
    for (auto key: cstrset_)
        delete key;
}

const char *duplicate(const char *s)
{
    const size_t size = std::strlen(s) + 1;
    char *str = new char[size];
    std::memcpy(str, s, size);
    return str;
};

SharedStr StringTable::Insert(const std::string &str)
{
    const auto it = cstrset_.find(str.c_str());
    if (it != cstrset_.end())
        return *it;

    const char *key = duplicate(str.c_str());
    cstrset_.insert(key);

    return key;
}

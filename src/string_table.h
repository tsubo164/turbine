#ifndef STRING_TABLE_H
#define STRING_TABLE_H

#include <unordered_map>
#include <string>
#include <vector>

class StringTable {
public:
    StringTable();
    ~StringTable();

    int Insert(const std::string &name);
    const std::string &Lookup(int id) const;

private:
    std::unordered_map<std::string,int> map_;
    std::vector<std::string> vec_;
    int id_ = 0;
};

#endif // _H

#include "string_table.h"

StringTable::StringTable()
{
    // emptry string for ID 0
    Insert("");
}

StringTable::~StringTable()
{
}

int StringTable::Insert(const std::string &name)
{
    const auto found = map_.find(name);
    if (found != map_.end())
        return found->second;

    map_.insert({name, id_});
    vec_.push_back(name);

    return id_++;
}

const std::string &StringTable::Lookup(int id) const
{
    if (id < 0 || id >= static_cast<int>(vec_.size()))
        return vec_[0];

    return vec_[id];
}

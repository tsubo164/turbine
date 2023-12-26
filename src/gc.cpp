#include "gc.h"
#include <iostream>

void StringObj::Print() const
{
    std::cout << "[StringObj] => " << str << std::endl;
}

StringObj *GC::NewString(const std::string &s)
{
    StringObj *obj = new StringObj(s);
    obj->next = root;
    root = obj;

    return obj;
}

void GC::PrintObjs() const
{
    for (Obj *obj = root; obj; obj = obj->next) {
        obj->Print();
    }
}

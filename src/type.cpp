#include "type.h"
#include "scope.h"

int Type::Size() const
{
    if (kind == TY::ClassType)
        return clss->Size();
    else
        return 1;
}

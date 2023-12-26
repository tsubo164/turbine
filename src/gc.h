#ifndef GC_H
#define GC_H

#include <string>

struct Obj {
    Obj() {}
    virtual ~Obj() {}

    Obj *next = nullptr;
    bool marked = false;
    virtual void Print() const = 0;
};

struct StringObj : public Obj {
    StringObj(const std::string &s) : str(s) {}
    ~StringObj() {}
    std::string str;

    void Print() const;
};

class GC {
public:
    GC() {}
    ~GC() {}

    StringObj *NewString(const std::string &s);

    void PrintObjs() const;

private:
    Obj *root = nullptr;
};

#endif // _H

#ifndef VEC_H
#define VEC_H

struct Vec {
    void **data;
    int cap;
    int len;
};

void VecPush(struct Vec *v, void *data);
void VecFree(struct Vec *v);

#endif // _H

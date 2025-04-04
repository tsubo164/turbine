#ifndef DATA_HASHMAP_H
#define DATA_HASHMAP_H

struct data_hashmap_entry {
    const char *key;
    void *val;
};

struct data_hashmap {
    struct data_hashmap_entry *buckets;
    int cap;
    int used;
};

struct data_hashmap_entry *data_hashmap_insert(struct data_hashmap *map,
        const char *key, void *data);
struct data_hashmap_entry *data_hashmap_lookup(const struct data_hashmap *map,
        const char *key);
int data_hashmap_get_count(const struct data_hashmap *map);

void data_hashmap_print(const struct data_hashmap *map);

void data_hashmap_free(struct data_hashmap *map);

#endif /* _H */

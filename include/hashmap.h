#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint32_t (*hash_fn_t)(const void *key, void *user);
typedef bool (*eq_fn_t)(const void *a, const void *b, void *user);

typedef struct {
  uint8_t *keys;
  uint8_t *values;
  uint8_t *state;
  size_t capacity;
  size_t length;
  size_t key_size;
  size_t value_size;
  hash_fn_t hash;
  eq_fn_t eq;
  void *user;
} hashmap_t;

void hashmap_init(hashmap_t *m, size_t key_size, size_t value_size,
                  hash_fn_t hash, eq_fn_t eq, void *user);

void hashmap_destroy(hashmap_t *m);
void hashmap_clear(hashmap_t *m);

void *hashmap_get(const hashmap_t *m, const void *key);
void *hashmap_put(hashmap_t *m, const void *key, const void *value,
                  bool *inserted);

typedef void (*hashmap_iter_fn_t)(const void *key, void *value, void *user);
void hashmap_foreach(hashmap_t *m, hashmap_iter_fn_t fn, void *user);

#endif

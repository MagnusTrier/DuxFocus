#include "hashmap.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define HM_MIN_CAP 64
#define HM_LOAD_NUM 7
#define HM_LOAD_DEN 10

static bool hm_rehash(hashmap_t *m, size_t new_cap) {
  uint8_t *keys = malloc(new_cap * m->key_size);
  uint8_t *values = malloc(new_cap * m->value_size);
  uint8_t *state = calloc(new_cap, 1);
  if (!keys || !values || !state) {
    free(keys);
    free(values);
    free(state);
    return false;
  }

  size_t old_cap = m->capacity;
  uint8_t *ok = m->keys;
  uint8_t *ov = m->values;
  uint8_t *os = m->state;

  m->keys = keys;
  m->values = values;
  m->state = state;
  m->capacity = new_cap;
  m->length = 0;

  for (size_t i = 0; i < old_cap; ++i) {
    if (!os[i])
      continue;
    const uint8_t *k = ok + i * m->key_size;
    const uint8_t *v = ov + i * m->value_size;
    size_t idx = m->hash(k, m->user) & (new_cap - 1);
    while (m->state[idx]) {
      idx = (idx + 1) & (new_cap - 1);
    }
    m->state[idx] = 1;
    memcpy(m->keys + idx * m->key_size, k, m->key_size);
    memcpy(m->values + idx * m->value_size, v, m->value_size);
    m->length++;
  }

  free(ok);
  free(ov);
  free(os);
  return true;
}

void hashmap_init(hashmap_t *m, size_t key_size, size_t value_size,
                  hash_fn_t hash, eq_fn_t eq, void *user) {
  memset(m, 0, sizeof(*m));
  m->key_size = key_size;
  m->value_size = value_size;
  m->hash = hash;
  m->eq = eq;
  m->user = user;
  hm_rehash(m, HM_MIN_CAP);
}

void hashmap_destroy(hashmap_t *m) {
  free(m->keys);
  free(m->values);
  free(m->state);
  memset(m, 0, sizeof(*m));
}

void hashmap_clear(hashmap_t *m) {
  if (m->state) {
    memset(m->state, 0, m->capacity);
  }
  m->length = 0;
}

static size_t hm_find(const hashmap_t *m, const void *key, uint32_t h,
                      bool *found) {
  size_t mask = m->capacity - 1;
  size_t idx = h & mask;

  while (m->state[idx]) {
    if (m->eq(m->keys + idx * m->key_size, key, m->user)) {
      *found = true;
      return idx;
    }
    idx = (idx + 1) & mask;
  }
  *found = false;
  return idx;
}

void *hashmap_get(const hashmap_t *m, const void *key) {
  if (!m->capacity)
    return NULL;
  bool found;
  size_t idx = hm_find(m, key, m->hash(key, m->user), &found);
  return found ? m->values + idx * m->value_size : NULL;
}

void *hashmap_put(hashmap_t *m, const void *key, const void *value,
                  bool *inserted) {
  if ((m->length + 1) * HM_LOAD_DEN > m->capacity * HM_LOAD_NUM) {
    if (!hm_rehash(m, m->capacity * 2)) {
      return NULL;
    }
  }

  bool found;
  size_t idx = hm_find(m, key, m->hash(key, m->user), &found);
  if (!found) {
    memcpy(m->keys + idx * m->key_size, key, m->key_size);
    m->state[idx] = 1;
    m->length++;
  }
  memcpy(m->values + idx * m->value_size, value, m->value_size);
  if (inserted) {
    *inserted = !found;
  }
  return m->values + idx * m->value_size;
}

void hashmap_foreach(hashmap_t *m, hashmap_iter_fn_t fn, void *user) {
  for (size_t i = 0; i < m->capacity; ++i) {
    if (m->state[i]) {
      fn(m->keys + i * m->key_size, m->values + i * m->value_size, user);
    }
  }
}

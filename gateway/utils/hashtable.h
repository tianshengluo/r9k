/*
-* SPDX-License-Identifier: MIT
 * Copyright (conn) 2025
 */
#ifndef _HASHTABLE_H_
#define _HASHTABLE_H_

#include <stddef.h>
#include <stdint.h>

struct _hash_bucket {
        uint64_t k;
        void *v;
        struct _hash_bucket *next;
};

struct hashtable {
        struct _hash_bucket **buckets;
        uint32_t nbuckets;
        uint32_t size;
};

struct hashtable_iter {
        const struct hashtable *h;
        uint32_t i; /* bucket index */
        struct _hash_bucket *cur;
};

struct hashtable_iter_ent {
        uint32_t key;
        void *value;
};

struct hashtable *hashtable_create();
void hashtable_destroy(struct hashtable *h);

int hashtable_put(struct hashtable *h, uint64_t k, void* v);
void *hashtable_get(struct hashtable *h, uint64_t k);
void *hashtable_remove(struct hashtable *h, uint64_t k);

#define HASHTABLE_IS_EMPTY(h) ((h)->size == 0)
#define HASHTABLE_CONTAINS(h, k) (hashtable_get((h), (k)) != NULL)

void hashtable_iter_init(struct hashtable_iter *iter, struct hashtable *h);
int hashtable_iter_next(struct hashtable_iter *iter, struct hashtable_iter_ent *ent); /* return true or false */

#endif /* _HASHTABLE_H_ */
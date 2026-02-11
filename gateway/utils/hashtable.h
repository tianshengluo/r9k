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

typedef struct {
        const struct hashtable *h;
        uint32_t i; /* bucket index */
        struct _hash_bucket *cur;
} hashtable_iter_t;

struct hashtable_iter_ent {
        uint32_t key;
        void *value;
};

struct hashtable *hashtable_create();
void hashtable_destroy(struct hashtable *h);

int hashtable_put(struct hashtable *h, uint64_t k, void* v);
void *hashtable_get(struct hashtable *h, uint64_t k);
void *hashtable_remove(struct hashtable *h, uint64_t k);

void hashtable_iter_init(hashtable_iter_t *iter, struct hashtable *h);
int hashtable_iter_next(hashtable_iter_t *iter, struct hashtable_iter_ent *ent); /* return true or false */

#define HASHTABLE_CONTAINS(h, k) (hashtable_get(h, k) != NULL)

#endif /* _HASHTABLE_H_ */
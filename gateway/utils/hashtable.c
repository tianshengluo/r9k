/*
-* SPDX-License-Identifier: MIT
 * Copyright (conn) 2025
 */
#include "hashtable.h"

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#define HASHTABLE_LOAD_FACTOR_NUM 3
#define HASHTABLE_LOAD_FACTOR_DEN 4
#define HASHTABLE_RESERVE_CAPCITY 32

static inline uint32_t _hash_u64(uint64_t k)
{
        k ^= k >> 33;
        k *= 0xff51afd7ed558ccdULL;
        k ^= k >> 33;
        k *= 0xc4ceb9fe1a85ec53ULL;
        k ^= k >> 33;
        return (uint32_t)k;
}

#define HASH64(k, n) (_hash_u64(k) % n)

static void _hashtable_ensure_capcity(struct hashtable *h)
{
        if (h->size * HASHTABLE_LOAD_FACTOR_DEN <
            h->nbuckets * HASHTABLE_LOAD_FACTOR_NUM)
                return;

        size_t newcap = h->nbuckets << 1;
        struct _hash_bucket **new_buckets, *n;

        new_buckets = calloc(newcap, sizeof(struct _hash_bucket *));

        if (!new_buckets) {
                perror("hashtable ensure capcity calloc");
                return;
        }


        for (uint32_t i = 0; i < h->nbuckets; i++) {
                n = h->buckets[i];
                
                while (n) {
                        struct _hash_bucket *next = n->next;
                        uint32_t idx = HASH64(n->k, newcap);

                        n->next = new_buckets[idx];
                        new_buckets[idx] = n;

                        n = next;
                }
        }

        free(h->buckets);

        h->buckets = new_buckets;
        h->nbuckets = newcap;
}

struct hashtable *hashtable_create()
{
        struct hashtable *ht;

        ht = calloc(1, sizeof(struct hashtable));

        if (!ht)
                return NULL;

        ht->nbuckets = HASHTABLE_RESERVE_CAPCITY;
        ht->size = 0;
        ht->buckets = calloc(ht->nbuckets, sizeof(struct _hash_bucket *));

        if (!ht->buckets) {
                free(ht);
                return NULL;
        }

        return ht;
}

void hashtable_destroy(struct hashtable *h)
{
        if (!h)
                return;

        if (h->buckets)
                free(h->buckets);

        free(h);
}

int hashtable_put(struct hashtable *h, uint64_t k, void* v)
{
        uint32_t i = HASH64(k, h->nbuckets);
        struct _hash_bucket *n;

        for (n = h->buckets[i]; n; n = n->next) {
                if (n->k == k) {
                        n->v = v;
                        return 0;
                }
        }

        n = malloc(sizeof(struct _hash_bucket));

        if (!n)
                return -1;

        n->k = k;
        n->v = v;
        n->next = h->buckets[i];

        h->buckets[i] = n;
        h->size++;

        _hashtable_ensure_capcity(h);

        return 0;
}

void *hashtable_get(struct hashtable *h, uint64_t k)
{
        uint32_t i = HASH64(k, h->nbuckets);
        struct _hash_bucket *n;

        for (n = h->buckets[i]; n; n = n->next) {
                if (n->k == k)
                        return n->v;
        }

        return NULL;
}

void *hashtable_remove(struct hashtable *h, uint64_t k)
{
        struct _hash_bucket **pp =
                &h->buckets[HASH64(k, h->nbuckets)];

        while (*pp) {
                struct _hash_bucket *n = *pp;

                if (n->k == k) {
                        void *v = n->v;

                        *pp = n->next;
                        free(n);
                        h->size--;

                        return v;
                }

                pp = &n->next;
        }

        return NULL;
}

void hashtable_iter_init(struct hashtable_iter *iter, struct hashtable *h)
{
        iter->h = h;
        iter->i = 0;
        iter->cur = NULL;

        if (h->size == 0)
                return;

        while (iter->i < h->nbuckets) {
                if (h->buckets[iter->i]) {
                        iter->cur = h->buckets[iter->i];
                        break;
                }
                iter->i++;
        }
}

int hashtable_iter_next(struct hashtable_iter *iter, struct hashtable_iter_ent *ent)
{
        if (iter->i >= iter->h->nbuckets)
                return 0;

        while (iter->i < iter->h->nbuckets) {
                while (iter->cur) {
                        ent->key = iter->cur->k;
                        ent->value = iter->cur->v;
                        iter->cur = iter->cur->next;
                        return 1;
                }

                iter->i++;

                if (iter->i < iter->h->nbuckets)
                        iter->cur = iter->h->buckets[iter->i];
        }

        return 0;
}
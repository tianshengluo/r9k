/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025
 */
#include <r9k/list.h>
#include <assert.h>

#define LIST_INIT_CAP 8

struct list
{
        void **items;
        size_t size;
        size_t cap;
};

struct list *list_new()
{
        struct list *list = calloc(1, sizeof(struct list));
        if (!list)
                return NULL;

        list->cap  = LIST_INIT_CAP;
        list->size = 0;

        list->items = malloc(list->cap * sizeof(void *));
        if (!list->items) {
                free(list);
                return NULL;
        }

        return list;
}

void list_free(struct list *list, list_free_func_t fn_free)
{
        if (!list)
                return;

        if (fn_free) {
                for (size_t i = 0; i < list->size; i++)
                        fn_free(list->items[i]);
        }

        free(list->items);
        free(list);
}

void list_push_back(struct list *list, void *item)
{
        assert(list);

        if (list->size >= list->cap) {
                if (list->cap >= SIZE_MAX / 2)
                        abort();

                list->cap *= 2;
                void **tmp = realloc(list->items, list->cap * sizeof(void *));
                if (!tmp)
                        abort();

                list->items = tmp;
        }

        list->items[list->size++] = item;
}

void *list_at(struct list *list, size_t index)
{
        return list->items[index];
}

size_t list_size(struct list *list)
{
        return list->size;
}
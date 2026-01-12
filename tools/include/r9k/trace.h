/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 Varketh Nockrath
 */
#ifndef TRACE_H_
#define TRACE_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <r9k/compiler_attrs.h>

#ifndef N_TRACE

__attr_unused
static ssize_t _read_trace(int fd, void *data, size_t size, const char *filename, int line)
{
        ssize_t n = read(fd, data, size);
        printf("[io ] %s:%d _read_trace fd -> %d buf: %p, sz: %zu, n: %ld\n", filename, line, fd, data, size, n);
        return n;
}

__attr_unused
static ssize_t _write_trace(int fd, const void *data, size_t size, const char *filename, int line)
{
        ssize_t n = write(fd, data, size);
        printf("[io ] %s:%d _write_trace fd -> %d buf: %p, sz: %zu, n: %ld\n", filename, line, fd, data, size, n);
        return n;
}

__attr_unused
static void *_malloc_trace(size_t size, const char *filename, int line)
{
        void *ptr = malloc(size);
        printf("[mem] %s:%d _malloc_trace %p %zu\n", filename, line, ptr, size);
        return ptr;
}

__attr_unused
static void *_calloc_trace(size_t count, size_t size, const char *filename, int line)
{
        void *ptr = calloc(count, size);
        printf("[mem] %s:%d _calloc_trace %p %zu\n", filename, line, ptr, size);
        return ptr;
}

__attr_unused
static void *_realloc_trace(void *ptr, size_t size, const char *filename, int line)
{
        void *reptr = realloc(ptr, size);
        printf("[mem] %s:%d _realloc_trace %p -> %p %zu\n", filename, line, ptr, reptr, size);
        return reptr;
}

__attr_unused
static void _free_trace(void *ptr, const char *filename, int line)
{
        if (ptr == NULL)
                return;
        printf("[mem] %s:%d _free_trace %p\n", filename, line, ptr);
        free(ptr);
}

// unistd.h
#define read(fd, data, size) _read_trace(fd, data, size, __FILE__, __LINE__)
#define write(fd, data, size) _write_trace(fd, data, size, __FILE__, __LINE__)

// stdlib.h
#define malloc(size) _malloc_trace(size, __FILE__, __LINE__)
#define calloc(count, size) _calloc_trace(count, size, __FILE__, __LINE__)
#define realloc(ptr, size) _realloc_trace(ptr, size, __FILE__, __LINE__)
#define free(ptr) _free_trace(ptr, __FILE__, __LINE__)

#endif

#endif /* TRACE_H_ */
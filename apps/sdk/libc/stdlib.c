#include "stdlib.h"

#include <stdint.h>

#include "errno.h"
#include "string.h"
#include "unistd.h"

typedef struct MHeader {
    size_t size;
    struct MHeader *next;
} MHeader;

#define MHDR_SZ (sizeof(MHeader))
#define MIN_SPLIT (MHDR_SZ + 16)

static MHeader *g_free;

static MHeader *morecore(size_t total) {
    char *p = (char *)sbrk((intptr_t)total);
    if (p == (char *)-1) return NULL;
    MHeader *h = (MHeader *)p;
    h->size = total;
    h->next = NULL;
    return h;
}

void *malloc(size_t n) {
    if (n == 0) return NULL;
    size_t need = (n + 7u) & ~7u;
    size_t total = MHDR_SZ + need;
    if (total < need) {
        errno = ENOMEM;
        return NULL;
    }

    MHeader **pp = &g_free;
    while (*pp) {
        MHeader *h = *pp;
        if (h->size >= total) {
            if (h->size >= total + MIN_SPLIT) {
                MHeader *sp = (MHeader *)((char *)h + total);
                sp->size = h->size - total;
                sp->next = h->next;
                h->size = total;
                *pp = sp;
            } else {
                *pp = h->next;
            }
            return (char *)h + MHDR_SZ;
        }
        pp = &h->next;
    }

    MHeader *h = morecore(total);
    if (!h) {
        errno = ENOMEM;
        return NULL;
    }
    if (h->size >= total + MIN_SPLIT) {
        MHeader *sp = (MHeader *)((char *)h + total);
        sp->size = h->size - total;
        sp->next = g_free;
        g_free = sp;
        h->size = total;
    }
    return (char *)h + MHDR_SZ;
}

void free(void *p) {
    if (!p) return;
    MHeader *h = (MHeader *)((char *)p - MHDR_SZ);
    h->next = g_free;
    g_free = h;
}

void *calloc(size_t nmemb, size_t size) {
    if (nmemb != 0 && size > (size_t)-1 / nmemb) {
        errno = ENOMEM;
        return NULL;
    }
    size_t t = nmemb * size;
    void *p = malloc(t);
    if (p) memset(p, 0, t);
    return p;
}

void *realloc(void *ptr, size_t size) {
    if (!ptr) return malloc(size);
    if (size == 0) {
        free(ptr);
        return NULL;
    }
    MHeader *h = (MHeader *)((char *)ptr - MHDR_SZ);
    size_t oldsz = h->size - MHDR_SZ;
    size_t need = (size + 7u) & ~7u;
    if (need <= oldsz) return ptr;
    void *q = malloc(size);
    if (!q) return NULL;
    memcpy(q, ptr, oldsz < size ? oldsz : size);
    free(ptr);
    return q;
}

int atoi(const char *s) {
    int sign = 1;
    int value = 0;
    while (*s == ' ' || *s == '\t' || *s == '\n') s++;
    if (*s == '-') {
        sign = -1;
        s++;
    } else if (*s == '+') {
        s++;
    }
    while (*s >= '0' && *s <= '9') {
        value = value * 10 + (*s - '0');
        s++;
    }
    return sign * value;
}

#ifndef _BUDDY_H_
#define _BUDDY_H_

#include "list.h"

struct free_list {
    struct list_link    list;
    unsigned long       *map;
};

struct buddy_sys {
    // Exponent for the min block size (e.g. 12 for 4K blocks)
    unsigned int        order_bit;
    // Maximum order
    unsigned int        order_max;
    // Memory base pointer
    char                *base;
    // Free list
    struct free_list    *free_area;
};

int buddy_init(struct buddy_sys *ctx, void *buf, size_t nunits,
               size_t unitsize, int busy);

void buddy_rlse(struct buddy_sys *ctx);

void *buddy_alloc(struct buddy_sys *ctx, unsigned int order);

void buddy_free(struct buddy_sys *ctx, void *ptr, unsigned int order);

void buddy_dump(struct buddy_sys *ctx);

#endif

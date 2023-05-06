#include "list.h"
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


struct free_list
{
    struct list_link    list;
    unsigned long       *map;
};

struct buddy_sys
{
    unsigned int        order_bit;  // e.g. exponent for the min block size (e.g. 12 for 4K blocks)
    unsigned int        order_max;  // maximum order
    char                *base;      // base memory address
    struct free_list    *free_area;
};

static inline unsigned long next_pow2(unsigned long v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

static inline unsigned int fnzb(unsigned long v)
{
    unsigned int n = 0;
    if (v >> 16) { v >>= 16; n += 16;}
    if (v >> 8)  { v >>= 8;  n += 8;}
    if (v >> 4)  { v >>= 4;  n += 4;}
    if (v >> 2)  { v >>= 2;  n += 2;}
    if (v >> 1)  { v >>= 1;  n += 1;}
    return n;
}

static int toggle_bit(struct buddy_sys *ctx, struct list_link *block, 
        unsigned int order)
{
    // the address of the block relative to the base is aligned to minblk size
    ptrdiff_t rel = (char *)block - ctx->base;
    // get the order block index 
    unsigned int i = rel >> (ctx->order_bit + order + 1);    // we use a bit for each couple (+1)
    unsigned long *word = &ctx->free_area[order].map[i/(8*sizeof(unsigned long))];
    unsigned long bit = 1 << i%(8*sizeof(unsigned long));
    *word ^= bit;           // toggle the bit
    return *word & bit;     // return the current value
}

static struct list_link *buddy_get(struct buddy_sys *ctx, 
        struct list_link *block, size_t order)
{
    ptrdiff_t rel = (char *)block - ctx->base;
    rel ^= (1 << (ctx->order_bit+order));
    return (struct list_link *) (ctx->base + rel);
}


void buddy_free(struct buddy_sys *ctx, void *ptr, unsigned int order)
{
    struct list_link *block = (struct list_link *)ptr;
    struct list_link *buddy;
    
    while (order != ctx->order_max)
    {
        /* Check if there is any buddy in the list of the same order */
        buddy = buddy_get(ctx, block, order);
        if (toggle_bit(ctx, buddy, order))
            break;

        /* Remove the buddy from its free list */
        list_del(buddy);
        /* Coalesce into one bigger block */
        order++;

        block = (block < buddy) ? block : buddy;
    }
    
    /* Insert the block at the end of the proper list */
    list_insert_before(&ctx->free_area[order].list, block);
}

void *buddy_alloc(struct buddy_sys *ctx, unsigned int order)
{
    unsigned int i;
    struct list_link *left = NULL, *right;
    
    for (i = order; i <= ctx->order_max; i++)
    {
        if (ctx->free_area[i].list.next != &ctx->free_area[i].list)   // Not empty
        {
            left = ctx->free_area[i].list.next;
            break;  // i is the order
        }
    }
    if (!left)
        return left;

    list_del(left);
    if (i != ctx->order_max)
        toggle_bit(ctx, left, i);   // order max doesn't have this map

    /* eventually split */
    while (i > order)
    {
        i--;
        right = (struct list_link *)((char *)left + (1 << (ctx->order_bit+i)));
        list_insert_before(&ctx->free_area[i].list, right);
        toggle_bit(ctx, right, i);
    }

    return left;
}

int buddy_init(struct buddy_sys *ctx, void *buf, size_t nunits,
        size_t unitsize, int busy)
{
    unsigned int i;
    char *ptr;

    ctx->base = buf;
    ctx->order_bit = fnzb(unitsize);
    ctx->order_max = fnzb(nunits);

    ctx->free_area = (struct free_list *) malloc(sizeof(struct free_list)
                                                  * (ctx->order_max+1));

    for (i = 0; i < ctx->order_max; i++)
    {
        int count = (nunits >> (i+1)); // divide by the number of blocks of order i by 2^(i+1) 
        count = (count-1)/(8*sizeof(unsigned long)) + 1;    // number of required unsigned longs to hold the map
        ctx->free_area[i].map = (unsigned long *) malloc(sizeof(unsigned long) * count);
        memset(ctx->free_area[i].map, 0, sizeof(unsigned long) * count);
        list_link_init(&ctx->free_area[i].list);
    }

    // Initialize the order_max entry with a NULL buddy
    list_link_init(&ctx->free_area[i].list);
    ctx->free_area[i].map = NULL;

    ptr = ctx->base;
    for (i = 0; i < nunits; i++)
    {
        buddy_free(ctx, ptr, 0); 
        ptr += (1 << ctx->order_bit);
    }

    return 0;
}


void buddy_rlse(struct buddy_sys *ctx)
{
    unsigned int i;
    for (i = 0; i < ctx->order_max; i++)
        free(ctx->free_area[i].map);
    free(ctx->free_area);
}

void buddy_dump(struct buddy_sys *ctx)
{
    unsigned int i;
    struct list_link *block;
    struct list_link *buddy;
    printf("-----------------------------------------\n");
    for (i = 0; i <= ctx->order_max; i++)
    {
        printf("order: %d", i);
        
        if (list_is_empty(&ctx->free_area[i].list))
            printf("   [ empty ]\n");
        else
        {
            printf("\n");
            for (block = ctx->free_area[i].list.next; 
                 block != &ctx->free_area[i].list; 
                 block = block->next)
            {
                buddy = buddy_get(ctx, block, i);
                printf("    free [0x%p : 0x%p)", block, (char *)block + (1 << (ctx->order_bit+i)));
                if (i < ctx->order_max)
                    printf("    busy [0x%p : 0x%p)\n", buddy, (char *)buddy + (1 << (ctx->order_bit+i)));
                else
                    printf("    no buddies\n");
            }
        }
    }
}

#define BUFSIZE     65000
#define UNITSIZE    4096
#define NUNITS      (BUFSIZE/UNITSIZE)

int main(int argc, char **argv)
{
    struct buddy_sys b;
    void *buf = malloc(BUFSIZE);
    
    buddy_init(&b, buf, NUNITS, UNITSIZE, 0);
    buddy_dump(&b);
    
    void *p1 = buddy_alloc(&b, 0);
    buddy_dump(&b);
    
    void *p2 = buddy_alloc(&b, 0);
    buddy_dump(&b);
    
    void *p3 = buddy_alloc(&b, 0);
    buddy_dump(&b);
    
    buddy_free(&b, p1, 0);
    buddy_dump(&b);
    
    buddy_free(&b, p2, 0);
    buddy_dump(&b);
    
    buddy_free(&b, p3, 0);
    buddy_dump(&b);
    
    buddy_rlse(&b);

    return 0;
}

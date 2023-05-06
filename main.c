#include "buddy.h"
#include <stdlib.h>

#define BUFSIZE     65000
#define UNITSIZE    4096
#define NUNITS      (BUFSIZE/UNITSIZE)

int main(int argc, char **argv) {
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

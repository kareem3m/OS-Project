
#include <stdio.h>
#include <math.h>
#include "circular.h"
#include <stddef.h>
#include <stdlib.h>
#include <sys/queue.h>

struct slice
{
    int size;
    int start;
    int end;
    int allocated;
    struct processData *p;
    TAILQ_ENTRY(slice)
    ptrs;
};

TAILQ_HEAD(TAILQhead, slice);
struct TAILQhead head; /* TAILQ head. */

int allocation(int size)
{
    struct slice *np;
    int find = 0;
    int min = 2000;
    struct slice *npmin;
    if (TAILQ_EMPTY(&head) == 0)
    {
        TAILQ_FOREACH(np, &head, ptrs)
        if ((np->size) >= size && np->allocated == 0)
        {
            if (np->size < min)
            {
                npmin = np;
                min = np->size;
            }
            find = 1;
        }
    }
    if (find)
    {
        printf("starts%d %d\n", npmin->start, npmin->size);
        np = npmin;
        while (np->size != size)
        {
            struct slice *npfirst = (struct slice *)malloc(sizeof(struct slice));
            struct slice *npsecond = (struct slice *)malloc(sizeof(struct slice));

            npfirst->allocated = 0;
            npfirst->start = np->start;
            npfirst->end = np->start + np->size / 2;
            npfirst->size = np->size / 2;
            printf("start: %d, end: %d, size: %d, allocated: %d\n", npfirst->start, npfirst->end, npfirst->size, npfirst->allocated);
            npsecond->allocated = 0;
            npsecond->start = np->end - np->size / 2;
            npsecond->end = np->end;
            npsecond->size = np->size / 2;
            printf("start: %d, end: %d, size: %d, allocated: %d\n", npsecond->start, npsecond->end, npsecond->size, npsecond->allocated);
            TAILQ_INSERT_BEFORE(np, npfirst, ptrs);
            TAILQ_INSERT_BEFORE(np, npsecond, ptrs);
            TAILQ_REMOVE(&head, np, ptrs);
            np = npfirst;
        }
        np->allocated = 1;
        return 1;
    }
    return 0;
}
int deallocation(int start, int end)
{
    struct slice *np, *next, *prev;
    TAILQ_FOREACH(np, &head, ptrs)
    if ((np->start) == start && np->end == end)
    {
        np->allocated = 0;
        break;
    }
   
    next = TAILQ_NEXT(np, ptrs);
    prev = TAILQ_PREV(np, TAILQhead, ptrs);
    while ((np->start != 0 && prev->allocated == 0 && prev->size == np->size) || (np->end != 1024 && next->allocated == 0 && next->size == np->size))
    {
        next = TAILQ_NEXT(np, ptrs);
        prev = TAILQ_PREV(np, TAILQhead, ptrs);
        if (np->end == 1024 && np->start == 0)
            return 1;
        //  printf("cond:%d,  \n",np->end != 1024 && (np->start / np->size) % 2 == 0 && next->allocated == 0 && next->size == np->size);
        if (np->end != 1024 && (np->start / np->size) % 2 == 0 && next->allocated == 0 && next->size == np->size)
        {
            struct slice *npfirst = (struct slice *)malloc(sizeof(struct slice));
            npfirst->allocated = 0;
            npfirst->start = np->start;
            npfirst->end = next->end;
            npfirst->size = np->size * 2;
           
            TAILQ_INSERT_BEFORE(np, npfirst, ptrs);
            TAILQ_REMOVE(&head, np, ptrs);
            TAILQ_REMOVE(&head, next, ptrs);
            np=TAILQ_FIRST(&head);  
            while (np!=NULL)
            {
                printf("%d\n", np->start);
                np=TAILQ_NEXT(np, ptrs);
                /* code */
            }
            np = npfirst;
        }
        next = TAILQ_NEXT(np, ptrs);
        prev = TAILQ_PREV(np, TAILQhead, ptrs);
        if (np->start != 0 && (np->start / np->size) % 2 == 1 && prev->allocated == 0 && np->size == np->size)
        {
            struct slice *npfirst = (struct slice *)malloc(sizeof(struct slice));
            npfirst->allocated = 0;
            npfirst->start = prev->start;
            npfirst->end = np->end;
            npfirst->size = np->size * 2;
            TAILQ_INSERT_AFTER(&head, np, npfirst, ptrs);
            TAILQ_REMOVE(&head, prev, ptrs);
            TAILQ_REMOVE(&head, np, ptrs);
            np = npfirst;
        }
        //printf("start:%d end: %d\n", np->start, np->end);
    }
    return 1;
}

//insert& remove fe 7eta msh shrt el head
//traverse
int main(void)
{
    struct slice *n1, *n2, *n3, *np;
    int i;

    TAILQ_INIT(&head); /* Initialize the TAILQ. */
    //

    n1 = malloc(sizeof(struct slice)); /* Insert at the head. */
    n1->start = 0;
    n1->end = 1024;
    n1->size = 1024;
    n1->allocated = 0;

    TAILQ_INSERT_HEAD(&head, n1, ptrs);
    allocation(32);
    allocation(16);
    allocation(16);
    allocation(32);
    deallocation(0, 32);
    deallocation(32 + 16, 64);
    allocation(8);
    allocation(32);
    deallocation(32, 48);
    allocation(16);
    deallocation(48, 56);
    deallocation(0, 32);
    deallocation(32, 48);  
    deallocation(64,96);

    exit(EXIT_SUCCESS);
}




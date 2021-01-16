#pragma once
#include <stdio.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/queue.h>
#include "headers.h"

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
int heighestPowerOfTwo(int num){
    return (int)pow(2,(int)ceil(log2(num)));

}
FILE *logptrbuddy;

void log_status_buddy(struct processData *p, char *allocaed, int start, int end)
{
    logptrbuddy = fopen("memory.log", "a");
    int ret = fprintf(logptrbuddy, "At time %d %s %d bytes for process %d from %d to %d\n", getClk(), allocaed, p->size, p->id, start, end);
    fclose(logptrbuddy);
}
TAILQ_HEAD(TAILQhead, slice);
struct TAILQhead head; /* TAILQ head. */
int allocation(struct processData* process)
{
    
    int size=heighestPowerOfTwo(process->size);
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
        //printf("starts%d %d\n", npmin->start, npmin->size);
        np = npmin;
        while (np->size != size)
        {
            struct slice *npfirst = (struct slice *)malloc(sizeof(struct slice));
            struct slice *npsecond = (struct slice *)malloc(sizeof(struct slice));

            npfirst->allocated = 0;
            npfirst->start = np->start;
            npfirst->end = np->start + np->size / 2;
            npfirst->size = np->size / 2;
            //printf("start: %d, end: %d, size: %d, allocated: %d\n", npfirst->start, npfirst->end, npfirst->size, npfirst->allocated);
            npsecond->allocated = 0;
            npsecond->start = np->end - np->size / 2;
            npsecond->end = np->end;
            npsecond->size = np->size / 2;
            //printf("start: %d, end: %d, size: %d, allocated: %d\n", npsecond->start, npsecond->end, npsecond->size, npsecond->allocated);
            TAILQ_INSERT_BEFORE(np, npfirst, ptrs);
            TAILQ_INSERT_BEFORE(np, npsecond, ptrs);
            TAILQ_REMOVE(&head, np, ptrs);
            np = npfirst;
        }
        np->p=process;
        np->allocated = 1;
        printf("start: %d, end: %d, size: %d, allocated: %d\n", np->start, np->end, np->size, np->allocated);
        log_status_buddy(np->p, "allocated", np->start, np->end-1);
        return 1;
    }
    return 0;
}
int deallocation(int id)
{
    struct slice *np, *next, *prev;
    TAILQ_FOREACH(np, &head, ptrs)
    if (np->p->id==id)
    {
        log_status_buddy(np->p, "freed", np->start, np->end-1);
        printf("start: %d, end: %d, size: %d, deallocated: %d\n", np->start, np->end, np->size, np->allocated);
        np->allocated = 0;
        np->p=NULL;
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
    }
    return 1;
}

void initialise(){
    TAILQ_INIT(&head); /* Initialize the TAILQ. */
    //

    struct slice *n1 = (struct slice *)malloc(sizeof(struct slice)); /* Insert at the head. */
    n1->start = 0;
    n1->end = 1024;
    n1->size = 1024;
    n1->allocated = 0;
    TAILQ_INSERT_HEAD(&head, n1, ptrs);
}
//insert& remove fe 7eta msh shrt el head
//traverse


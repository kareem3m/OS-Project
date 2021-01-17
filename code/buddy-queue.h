#pragma once
#include <stdio.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/queue.h>
#include "headers.h"

//a  slice is one entity of the buddy table
struct slice
{   
    int size;
    int start;
    int end;
    int allocated;
    struct processData *p;
    TAILQ_ENTRY(slice)
    ptrs;
    int id;
};

// to get the nearst power of 2 to a number larger than or equal to it 
int heighestPowerOfTwo(int num)
{  
    return (int)pow(2,(int)ceil(log2(num)));
}

FILE *logptrbuddy;

//to write on memory.log file 
void LogStatusBuddy(struct processData *p, char *allocaed, int start, int end,int id)
{
    logptrbuddy = fopen("memory.log", "a");
    int ret = fprintf(logptrbuddy, "At time %d %s %d bytes for process %d from %d to %d\n", getClk(), allocaed, p->size,id, start, end);
    fclose(logptrbuddy);
}

//initalize buddy table head
TAILQ_HEAD(TAILQhead, slice);
struct TAILQhead head; /* TAILQ head. */

//to allocate one process in buddy system memory if there was space in memory
int allocation(struct processData* process)
{
 
    int size = heighestPowerOfTwo(process->size);
    struct slice *np;
    int find = 0;
    int min = 2000;
    struct slice *npmin;

    //search for the smallest free place of power of 2 size larger than or equal the process size in the memory
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
    //divide the slice into smaller slices if it is larger than the nearst power of 2 of the process size
    if (find)
    {
        np = npmin;

        //iterate untill the sizes are equal  
        while (np->size != size)
        {

            struct slice *npfirst = (struct slice *)malloc(sizeof(struct slice));
            struct slice *npsecond = (struct slice *)malloc(sizeof(struct slice));

            //first part
            npfirst->allocated = 0;
            npfirst->start = np->start;
            npfirst->end = np->start + np->size / 2;
            npfirst->size = np->size / 2;

            //second part
            npsecond->allocated = 0;
            npsecond->start = np->end - np->size / 2;
            npsecond->end = np->end;
            npsecond->size = np->size / 2;
            
            TAILQ_INSERT_BEFORE(np, npfirst, ptrs);
            TAILQ_INSERT_BEFORE(np, npsecond, ptrs);
            TAILQ_REMOVE(&head, np, ptrs);
            np = npfirst;
        }

        np->p=process;
        np->allocated = 1;
        np->id=process->id;

        LogStatusBuddy(np->p, "allocated", np->start, np->end-1,np->id);

        return 1;
    }
    return 0;
}

//deallocate a process from the memory with its id when it finishes
int deallocation(int id)
{  
    struct slice *np, *next, *prev;
    TAILQ_FOREACH(np, &head, ptrs)
    {
      
        if (np->id==id)
        {
            LogStatusBuddy(np->p, "freed", np->start, np->end-1,np->id);
            np->allocated = 0;
            np->p=NULL;
            np->id=-1;
            break;
        }

    }

    //next slice and previous slice
    next = TAILQ_NEXT(np, ptrs);
    prev = TAILQ_PREV(np, TAILQhead, ptrs);

    //iterate until buddy merging conditions fails
    while ((np->start != 0 && prev->allocated == 0 && prev->size == np->size) || (np->end != 1024 && next->allocated == 0 && next->size == np->size))
    {
        next = TAILQ_NEXT(np, ptrs);
        prev = TAILQ_PREV(np, TAILQhead, ptrs);

        if (np->end == 1024 && np->start == 0)
            return 1;

        //merging two slices into one slice  when it is in even position  
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

        //merging two slices into one slice  when it is in odd position 
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

//intialize buddy system table
void initialise()
{
    TAILQ_INIT(&head); /* Initialize the TAILQ. */
    struct slice *n1 = (struct slice *)malloc(sizeof(struct slice)); /* Insert at the head. */
    n1->start = 0;
    n1->end = 1024;
    n1->size = 1024;
    n1->allocated = 0;
    TAILQ_INSERT_HEAD(&head, n1, ptrs);
}



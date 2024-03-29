#include <stdio.h>
#include <stdlib.h>
 
// Data structure for queue
struct processData
{
    int arrivaltime;
    int priority;
    int runningtime;
    int id;
};
struct queue
{
    struct processData *items;        // array to store queue elements
    int maxsize;    // maximum capacity of the queue
    int front;        // front points to front element in the queue (if any)
    int rear;        // rear points to last element in the queue
    int size;        // current capacity of the queue
};
 
// Utility function to initialize queue
struct queue* newQueue(int size)
{
    struct queue *pt = NULL;
    pt = (struct queue*)malloc(sizeof(struct queue));
    
    pt->items = (struct processData*)malloc(size * sizeof(struct processData));
    pt->maxsize = size;
    pt->front = 0;
    pt->rear = -1;
    pt->size = 0;
 
    return pt;
}
 
// Utility function to return the size of the queue
int size(struct queue *pt)
{
    return pt->size;
}
 
// Utility function to check if the queue is empty or not
int isEmpty(struct queue *pt)
{
    return !size(pt);
}
 
// Utility function to return front element in queue
struct processData front(struct queue *pt)
{
    if (isEmpty(pt))
    {
        printf("UnderFlow\nProgram Terminated\n");
        exit(EXIT_FAILURE);
    }
 
    return pt->items[pt->front];
}
 
// Utility function to add an element x in the queue
void enqueue(struct queue *pt, struct processData x)
{
    if (size(pt) == pt->maxsize)
    {
        printf("OverFlow\nProgram Terminated\n");
        exit(EXIT_FAILURE);
    }
 
    printf("Inserting %d\t", x.id);
 
    pt->front = (pt->front + 1) % pt->maxsize;    // circular queue
    pt->items[pt->front] = x;
    pt->size++;
 
    printf("front = %d, rear = %d\n", pt->front, pt->rear);
}
 
// Utility function to remove element from the queue
void dequeue(struct queue *pt)
{
    if (isEmpty(pt)) // front == rear
    {
        printf("UnderFlow\nProgram Terminated\n");
        exit(EXIT_FAILURE);
    }
 
    printf("Removing  %d\t", front(pt).id);
 
    pt->front = (pt->front + 1) % pt->maxsize;    // circular queue
    pt->size--;
 
    printf("front = %d, rear = %d\n", pt->front, pt->rear);
}
 


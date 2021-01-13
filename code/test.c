#include <sys/queue.h>
#include <stdio.h>
#include <unistd.h>

struct node
{
    int data;
    CIRCLEQ_ENTRY(node) ptrs;
};

CIRCLEQ_HEAD(circlehead, node);

struct circlehead ready_queue; 

int main(){
    CIRCLEQ_INIT(&ready_queue);
    struct node n1;
    n1.data = 10;
    CIRCLEQ_INSERT_TAIL(&ready_queue, &n1, ptrs);

    struct node n2;
    n2.data = 20;
    CIRCLEQ_INSERT_TAIL(&ready_queue, &n2, ptrs);

    struct node n3;
    n3.data = 30;
    CIRCLEQ_INSERT_TAIL(&ready_queue, &n3, ptrs);

    struct node* current_node = &ready_queue;
    
    while(1){
        current_node = CIRCLEQ_LOOP_NEXT(&ready_queue, current_node, ptrs);
        printf("%d\n", current_node->data);
        fflush(stdout);
        sleep(1);
    }
    return 0;
}
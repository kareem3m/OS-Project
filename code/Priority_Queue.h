#include <stdio.h>
#include <stdlib.h>
#include "circular.h"
struct Priority_Queue{
struct processData* process=NULL;
int Size=0;
int Front=-1;
int Rear=-1;

//struct Process* Prev=NULL;
};
int Get_Size(struct  Priority_Queue *pt_q)
{
    return pt_q->Size;
}
bool Is_Empty(struct Priority_Queue *pt_q)
{
    if(Get_Size(pt_q)==0)
    {
        return true;
    }
    else
    {
        return false;
    }
        
}
struct processData Get_Front(struct Priority_Queue *pt_q)
{
    if (Is_Empty(pt_q))
    {
       return NULL;
    }
 
    return pt_q->process[pt_q->Front];
}
void Enqueue(struct Priority_Queue *pt_q,struct processData pr){
    if(Is_Empty(pt_q)){
        pt_q->Front++;
        pt_q->Rear++;
        //pt_q->Next=NULL;
        pt_q->process[pt_q->Rear]=pr;
    }
    else {
        int x=0;
        while(x<=pt_q->Rear ){
            if(pt_q->process[x]->priority>pr->priority){
                if(x>0){
                pr->Next=pt_q->process[x];
                pt_q->process[x-1]->Next=pr;
                pt_q->Rear++;
                break;
                }
                else
                {
                    pr->Next=pt_q->process[x];
                    pt_q->process=pr;
                    pt_q->Rear++;
                    break;
                }
            }
            else
            {
                x++;
            }
        }
        if(x>pt_q->Rear){
            pt_q->process[pt_q->Rear]->Next=pr;
            pt_q->Rear++;
        }
    }

    
    pt_q->Size++;
}
void Dequeue(struct Priority_Queue *pt_q){
    if (Is_Empty){
        return;
    }
    else{  
        pt_q->process=process[pt_q->Front+1];
        pt_q->Front++;
        pt_q->Size--;
    }
}

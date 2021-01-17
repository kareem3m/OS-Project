#include <stdio.h>
#include <stdlib.h>
#include "circular.h"

 struct processData* New_Process(int arrivalTime,int priority,int runningTime,int id) 
{ 
	struct processData* temp = (struct processData*)malloc(sizeof(struct processData)); 
	temp->arrivalTime = arrivalTime; 
	temp->priority = priority; 
    temp->runningTime =runningTime;
    temp->remainingTime =runningTime;
    temp-> wait=0;
    temp-> id=id;
    temp-> status=READY;
    temp->pid =0;
    temp-> lastRun=0;
	temp->next = NULL; 
	return temp; 
} 

int Is_Empty(struct processData** head) 
{ 
	return (*head) == NULL; 
} 

void Enqueue(struct processData** head,struct processData** process) 
{ 
	struct processData* start = (*head); 

	struct processData* temp = (*process); 
    if(Is_Empty(*head)){
        (*head) = temp; 
    }
 
	else if ((*head)->priority > temp->priority) { 

		temp->next = (*head); 
		(*head) = temp; 
	} 
	else { 

		while (start->next != NULL && start->next->priority < temp-> priority) 
		{ 
			start = start->next; 
		} 

		temp->next = start->next; 
		start->next = temp; 
	} 
} 

void Dequeue(struct processData** head) 
{ 
	struct processData* temp = (*head);  
	(*head) = (*head)->next; 
	free(temp);

}
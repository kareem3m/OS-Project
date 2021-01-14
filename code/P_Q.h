#include <stdio.h>
#include <stdlib.h>
#include "circular.h"
// struct processData
// {
//     int arrivaltime;
//     int priority;
//     int runningtime;
//     int remainingtime;
//     int wait;
//     int id;
//     int status;
//     int pid;
//     int last_run;
//     struct processData* Next;
// };
 struct processData* New_Process(int arrivaltime,int priority,int runningtime,int id) 
{ 
	struct processData* temp = (struct processData*)malloc(sizeof(struct processData)); 
	temp->arrivaltime = arrivaltime; 
	temp->priority = priority; 
    temp->runningtime =runningtime;
    temp->remainingtime =runningtime;
    temp-> wait=0;
    temp-> id=id;
    temp-> status=READY;
    temp->pid =0;
    temp-> last_run=0;
	temp->Next = NULL; 
	return temp; 
} 
// struct processData* Dequeue(struct processData** head) 
// { 
// 	struct processData* temp = (*head);  //assignmemt operator
// 	// struct processData* temp_l = temp;
// 	(*head) = (*head)->Next; 
// 	//free(temp);
// 	 return temp;

// }
void Enqueue(struct processData** head,struct processData** process) 
{ 
	struct processData* start = (*head); 

	// Create new Node 
	struct processData* temp = (*process); 
    if(Is_Empty(*head)){
        (*head) = temp; 
    }
	// Special Case: The head of list has lesser 
	// priority than new node. So insert new 
	// node before head node and change head node. 
	else if ((*head)->priority > temp->priority) { 

		// Insert New Node before head 
		temp->Next = (*head); 
		(*head) = temp; 
	} 
	else { 

		// Traverse the list and find a 
		// position to insert new node 
		while (start->Next != NULL && start->Next->priority < temp-> priority) 
		{ 
			start = start->Next; 
		} 

		// Either at the ends of the list 
		// or at required position 
		temp->Next = start->Next; 
		start->Next = temp; 
	} 
} 
int Is_Empty(struct processData** head) 
{ 
	return (*head) == NULL; 
} 
void Dequeue(struct processData** head) 
{ 
	struct processData* temp = (*head);  //assignmemt operator
	// struct processData* temp_l = temp;
	(*head) = (*head)->Next; 
	free(temp);
	 //return temp;

}
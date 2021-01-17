#include "headers.h"
#include "P_Q.h"
#include <time.h>
#include "buddy-queue.h"
#include <sys/queue.h>

//functions declerations
void start_process(struct processData *process);
void check_for_new_processes();
void wait_next_clk();
void remove_process(struct processData *p);
void log_status(struct processData *p, char *status);

FILE *logptr;

//decleration for the waiting queue
struct process
{
    struct processData data;
    CIRCLEQ_ENTRY(process)
    ptrs;
};
CIRCLEQ_HEAD(circlehead, process);
struct circlehead headWait;
int waitQueueSize = 0;
struct circlehead *waitQueue;

//Head of priority queue and the process that is running now 
struct processData *Head;
struct processData *currentProcess;

//boolean to indicate that there is no more processes from the process generator
bool finishedProcesses = false;

int main(int argc, char *argv[])
{
    //initialize the wait queue
    CIRCLEQ_INIT(&headWait);
    waitQueue = &headWait;
    initialise();
    initialize_ipc();

    //intitialize the head of the priority queue
    currentProcess = New_Process(0, 0, 0, 0);
    Head = New_Process(0, 0, 0, 0);


    initClk();
    while (1)
    {
        //looking for new processes
        if (finishedProcesses == false)
        {
            check_for_new_processes();
        }
        //starts a process if no process is running right now
        if (currentProcess->priority == 0 && Head->priority != 0)
        {
            start_process(Head);
            down(sem1_process);
            currentProcess->remainingTime--;
            useful_seconds++;
        }

        //decrease the remaning time of the running process
        else if (currentProcess->status == STARTED && currentProcess->remainingTime > 0)
        {
            down(sem1_process);
            currentProcess->remainingTime--;
            useful_seconds++;
        }
        //for synchronization on remaning time  between scheduler and process
        up(sem2_process);

        //remove process when it finishes
        if (currentProcess->status == STARTED && currentProcess->remainingTime == 0)
        {
            remove_process(currentProcess);
        }

        //exit after finishing all processes
        if (finishedProcesses == true && Head->priority == 0 && currentProcess->priority == 0)
        {
            generate_perf_file();
            exit(0);
        }
    }

    return 0;
}

void start_process(struct processData *pr)
{
    //copy info of the process to current process that will run
    currentProcess->id = pr->id;
    currentProcess->arrivalTime = pr->arrivalTime;
    currentProcess->lastRun = pr->lastRun;
    currentProcess->priority = pr->priority;
    currentProcess->remainingTime = pr->remainingTime;
    currentProcess->responseTime = pr->responseTime;
    currentProcess->runningTime = pr->runningTime;
    
    //remove the process from priority queue
    Dequeue(&pr);

    Head = *&pr;
    if (Is_Empty(&Head))
    {
        Head = New_Process(0, 0, 0, 0);
    }
    currentProcess->wait += getClk() - currentProcess->arrivalTime;
    int pid = fork();

    if (pid == 0)
    {
        char arg[10];
        sprintf(arg, "%d", currentProcess->runningTime);
        int ret = execl("process", "process", arg, NULL);
        perror("Error in execl: ");
        exit(-1);
    }

    else
    {
        currentProcess->pid = pid;
        currentProcess->status = STARTED;
        log_status(currentProcess, "started");
    }
}

void remove_process(struct processData *Pr)
{

    if (kill(Pr->pid, SIGSTOP) == -1)
    {
        perror("Error in kill: ");
        exit(-1);
    }
    Pr->status = FINISHED;
    Pr->lastRun = getClk();
    log_status(Pr, "finished");
    struct processData *temp = (struct processData *)malloc(sizeof(struct processData));

    //removing process from memory and check if any process in the wait queue can take that memory
    deallocation(Pr->id);
    struct process *np = (struct process *)malloc(sizeof(struct process));
    CIRCLEQ_FOREACH(np, &headWait, ptrs)
    {
        if (allocation(&np->data))
        {
            np->data.wait = getClk() - np->data.arrivalTime;
            // fadel el SIZE wel WAIT
            if (Head->priority == 0)
            {
                Head = New_Process(np->data.arrivalTime, np->data.priority, np->data.runningTime, np->data.id);
            }
            else
            {
                temp = New_Process(np->data.arrivalTime, np->data.priority, np->data.runningTime, np->data.id);
                Enqueue(&Head, &temp);
            }

            CIRCLEQ_REMOVE(&headWait, np, ptrs);
            waitQueueSize -= 1;
        }
    }
    currentProcess = New_Process(0, 0, 0, 0);
}

void check_for_new_processes()
{
    while (1)
    {
        down(sem1);
        struct processData *pr = (struct processData *)malloc(sizeof(struct processData));
        if (shmaddr_pg->id == -2)
        {
            up(sem2);
            finishedProcesses = true;
            return;
        }
        if (shmaddr_pg->id == -1)
        {
            up(sem2);
            return;
        }

        //if there is a space in memory put process in priority queue els put it in wait queue
        if (allocation(shmaddr_pg))
        {
            if (Head->priority == 0)
            {
                Head = New_Process(shmaddr_pg->arrivalTime, shmaddr_pg->priority, shmaddr_pg->runningTime, shmaddr_pg->id);
            }
            else
            {
                pr = New_Process(shmaddr_pg->arrivalTime, shmaddr_pg->priority, shmaddr_pg->runningTime, shmaddr_pg->id);
                Enqueue(&Head, &pr);
            }
        }
        else
        {
            struct process *p = (struct process *)malloc(sizeof(struct process));
            p->data.id = shmaddr_pg->id;
            p->data.arrivalTime = shmaddr_pg->arrivalTime;
            p->data.lastRun = getClk();
            p->data.priority = shmaddr_pg->priority;
            p->data.runningTime = shmaddr_pg->runningTime;
            p->data.remainingTime = shmaddr_pg->runningTime;
            p->data.status = READY;
            p->data.wait = 0;
            p->data.size = shmaddr_pg->size;
            CIRCLEQ_INSERT_TAIL(waitQueue, p, ptrs);
            waitQueueSize += 1;
        }

        up(sem2);
    }
}

void wait_next_clk()
{
    int start = getClk();
    while (start == getClk())
        ;
}
#include "headers.h"
#include "P_Q.h"
#include <time.h>
#include <sys/queue.h>
#include"buddy-queue.h"
struct process
{
    struct processData data;
    CIRCLEQ_ENTRY(process)
    ptrs;
};

CIRCLEQ_HEAD(circlehead, process);
struct circlehead *waitQueue;

void start_process(struct processData *process);
void check_for_new_processes();
void wait_next_clk();
void resume_process(struct processData *p);
void stop_process(struct processData *p);
void remove_process(struct processData *p);

FILE *logptr;
struct circlehead headWait;
struct processData *Head;
struct processData *currentProcess;

bool finishedProcesses = false;
int waitQueueSize = 0;

int main(int argc, char *argv[])
{
    initialise();
    initialize_ipc();
    CIRCLEQ_INIT(&headWait);
    waitQueue = &headWait;

    Head = New_Process(0, 0, 0, 0);
    currentProcess = New_Process(0, 0, 0, 0);

    initClk();
    while (1)
    {
        if (finishedProcesses == false)
        {
            check_for_new_processes();
        }

        up(sem2_process);

        if (currentProcess->priority == 0 && Head->priority != 0 && Head->status == READY)
        {
            reset(sem2_process);
            start_process(Head);
            down(sem1_process);
            useful_seconds++;
            currentProcess->remainingTime--;
            currentProcess->priority--;
        }
        else if ((currentProcess->status == STARTED || currentProcess->status == RESUMED) && currentProcess->remainingTime != 0)
        {
            down(sem1_process);
            useful_seconds++;
            currentProcess->remainingTime--;
            currentProcess->priority--;
        }
        else if (Head->status == STOPPED && Head->remainingTime != 0)
        {
            reset(sem2_process);
            resume_process(Head);
            useful_seconds++;
            down(sem1_process);
            currentProcess->remainingTime--;
            currentProcess->priority--;
        }

        if ((currentProcess->status == STARTED || currentProcess->status == RESUMED) && currentProcess->remainingTime == 0)
        {
            up(sem2_process);
            remove_process(currentProcess);
        }
        if (currentProcess == true && Head->priority == 0 && currentProcess->priority == 0)
        {
            generate_perf_file();
            exit(0);
        }
    }

    return 0;
}

void start_process(struct processData *pr)
{
    currentProcess->id = pr->id;
    currentProcess->priority = pr->remainingTime;
    currentProcess->arrivalTime = pr->arrivalTime;
    currentProcess->runningTime = pr->runningTime;
    currentProcess->remainingTime = pr->remainingTime;
    currentProcess->lastRun = pr->lastRun;
    currentProcess->pid = pr->pid;
    currentProcess->status = pr->status;

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

void resume_process(struct processData *p)
{
    currentProcess->id = p->id;
    currentProcess->priority = p->remainingTime;
    currentProcess->arrivalTime = p->arrivalTime;
    currentProcess->runningTime = p->runningTime;
    currentProcess->remainingTime = p->remainingTime;
    currentProcess->lastRun = p->lastRun;
    currentProcess->pid = p->pid;
    currentProcess->status = p->status;

    Dequeue(&p);
    Head = *&p;

    if (Is_Empty(&Head))
    {
        Head = New_Process(0, 0, 0, 0);

    }
    if (kill(currentProcess->pid, SIGCONT) == -1)
    {
        perror("Error in kill: ");
        exit(-1);
    }

    currentProcess->status = RESUMED;
    currentProcess->wait += getClk() - currentProcess->lastRun;
    log_status(currentProcess, "resumed");
}

void stop_process(struct processData *p)
{
    if (kill(p->pid, SIGSTOP) == -1)
    {
        perror("Error in kill: ");
        exit(-1);
    }
    p->status = STOPPED;
    p->lastRun = getClk();

    if (p->remainingTime != 0)
    {
        log_status(p, "stopped");
    }
    else
    {
        log_status(p, "finished");
        remove_process(p);
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

    //removing process from memory after finishing and look if there is a process in wait queue and allocate it if there is enough space in memory
    deallocation(Pr->id);
    struct processData *temp = (struct processData *)malloc(sizeof(struct processData));
    struct process *np = (struct process *)malloc(sizeof(struct process));
    CIRCLEQ_FOREACH(np, &headWait, ptrs)
    {
        if (allocation(&np->data))
        {
            np->data.wait = getClk() - np->data.arrivalTime;
            if (Head->priority == 0)
            {
                Head = New_Process(np->data.arrivalTime, np->data.runningTime, np->data.runningTime, np->data.id);
            }
            else
            {
                temp = New_Process(np->data.arrivalTime, np->data.runningTime, np->data.runningTime, np->data.id);
                Enqueue(&Head, &temp);
            }
            if (Head->priority < currentProcess->priority)
            {
            stop_process(currentProcess);
            if (currentProcess->remainingTime != 0)
            {
                currentProcess->priority = currentProcess->remainingTime;
                Enqueue(&Head, &currentProcess);
            }
            currentProcess = New_Process(0, 0, 0, 0);
        }

            CIRCLEQ_REMOVE(&headWait, np, ptrs);
            waitQueueSize -= 1;
        }
    }
    free(currentProcess);
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
        //checking if there is a space in memory then allocate the process and put it in priority queue else put the process in wait queue
        if (allocation(shmaddr_pg))
        {
            if (Head->priority == 0)
            {
                Head = New_Process(shmaddr_pg->arrivalTime, shmaddr_pg->runningTime, shmaddr_pg->runningTime, shmaddr_pg->id);
            }
            else
            {
                pr = New_Process(shmaddr_pg->arrivalTime, shmaddr_pg->runningTime, shmaddr_pg->runningTime, shmaddr_pg->id);
                Enqueue(&Head, &pr);
            }
        
            if (Head->priority < currentProcess->priority)
            {
                stop_process(currentProcess);
                if (currentProcess->remainingTime != 0)
                {
                    currentProcess->priority = currentProcess->remainingTime;
                    Enqueue(&Head, &currentProcess);
                }
                currentProcess = New_Process(0, 0, 0, 0);
            }
        }
        else
        {
            struct process *p = (struct process *)malloc(sizeof(struct process));
            p->data.id = shmaddr_pg->id;
            p->data.arrivalTime = shmaddr_pg->arrivalTime;
            p->data.lastRun = getClk();
            p->data.priority = shmaddr_pg->runningTime;
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

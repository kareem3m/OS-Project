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
struct circlehead *wait_queue;

void start_process(struct processData *process);
void check_for_new_processes();
void wait_next_clk();
void resume_process(struct processData *p);
void stop_process(struct processData *p);
void remove_process(struct processData *p);

FILE *logptr;
struct circlehead head_wait;
struct processData *Head;
struct processData *Current_Process;

bool Finished_Processes = false;
int wait_queue_size = 0;
int main(int argc, char *argv[])
{
    initialise();
    initialize_ipc();
    CIRCLEQ_INIT(&head_wait);
    wait_queue = &head_wait;

    Head = New_Process(0, 0, 0, 0);
    Current_Process = New_Process(0, 0, 0, 0);

    initClk();
    while (1)
    {
        if (Finished_Processes == false)
        {
            check_for_new_processes();
        }

        up(sem2_process);

        if (Current_Process->priority == 0 && Head->priority != 0 && Head->status == READY)
        {
            reset(sem2_process);
            start_process(Head);
            down(sem1_process);
            useful_seconds++;
            Current_Process->remainingTime--;
            Current_Process->priority--;
        }
        else if ((Current_Process->status == STARTED || Current_Process->status == RESUMED) && Current_Process->remainingTime != 0)
        {
            down(sem1_process);
            useful_seconds++;
            Current_Process->remainingTime--;
            Current_Process->priority--;
        }
        else if (Head->status == STOPPED && Head->remainingTime != 0)
        {
            reset(sem2_process);
            resume_process(Head);
            useful_seconds++;
            down(sem1_process);
            Current_Process->remainingTime--;
            Current_Process->priority--;
        }

        if ((Current_Process->status == STARTED || Current_Process->status == RESUMED) && Current_Process->remainingTime == 0)
        {
            up(sem2_process);
            remove_process(Current_Process);
        }
        if (Finished_Processes == true && Head->priority == 0 && Current_Process->priority == 0)
        {
            generate_perf_file();
            exit(0);
        }
    }

    return 0;
}

void start_process(struct processData *pr)
{
    Current_Process->id = pr->id;
    Current_Process->priority = pr->remainingTime;
    Current_Process->arrivalTime = pr->arrivalTime;
    Current_Process->runningTime = pr->runningTime;
    Current_Process->remainingTime = pr->remainingTime;
    Current_Process->lastRun = pr->lastRun;
    Current_Process->pid = pr->pid;
    Current_Process->status = pr->status;

    Dequeue(&pr);
    Head = *&pr;

    if (Is_Empty(&Head))
    {
        Head = New_Process(0, 0, 0, 0);
    }

    Current_Process->wait += getClk() - Current_Process->arrivalTime;
    int pid = fork();
    if (pid == 0)
    {
        char arg[10];
        sprintf(arg, "%d", Current_Process->runningTime);
        int ret = execl("process", "process", arg, NULL);
        perror("Error in execl: ");
        exit(-1);
    }
    else
    {
        Current_Process->pid = pid;
        Current_Process->status = STARTED;
        log_status(Current_Process, "started");
    }
}

void resume_process(struct processData *p)
{
    Current_Process->id = p->id;
    Current_Process->priority = p->remainingTime;
    Current_Process->arrivalTime = p->arrivalTime;
    Current_Process->runningTime = p->runningTime;
    Current_Process->remainingTime = p->remainingTime;
    Current_Process->lastRun = p->lastRun;
    Current_Process->pid = p->pid;
    Current_Process->status = p->status;

    Dequeue(&p);
    Head = *&p;

    if (Is_Empty(&Head))
    {
        Head = New_Process(0, 0, 0, 0);

    }
    if (kill(Current_Process->pid, SIGCONT) == -1)
    {
        perror("Error in kill: ");
        exit(-1);
    }

    Current_Process->status = RESUMED;
    Current_Process->wait += getClk() - Current_Process->lastRun;
    log_status(Current_Process, "resumed");
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
    CIRCLEQ_FOREACH(np, &head_wait, ptrs)
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
            if (Head->priority < Current_Process->priority)
            {
            stop_process(Current_Process);
            if (Current_Process->remainingTime != 0)
            {
                Current_Process->priority = Current_Process->remainingTime;
                Enqueue(&Head, &Current_Process);
            }
            Current_Process = New_Process(0, 0, 0, 0);
        }

            CIRCLEQ_REMOVE(&head_wait, np, ptrs);
            wait_queue_size -= 1;
        }
    }
    free(Current_Process);
    Current_Process = New_Process(0, 0, 0, 0);
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
            Finished_Processes = true;
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
        
            if (Head->priority < Current_Process->priority)
            {
                stop_process(Current_Process);
                if (Current_Process->remainingTime != 0)
                {
                    Current_Process->priority = Current_Process->remainingTime;
                    Enqueue(&Head, &Current_Process);
                }
                Current_Process = New_Process(0, 0, 0, 0);
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
            CIRCLEQ_INSERT_TAIL(wait_queue, p, ptrs);
            wait_queue_size += 1;
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

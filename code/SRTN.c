#include "headers.h"
// #include "circular.h"
#include "P_Q.h"
#include <time.h>

void start_process(struct processData *process);
void check_for_new_processes();
void wait_next_clk();
void resume_process(struct processData *p);
void stop_process(struct processData *p);
void remove_process(struct processData *p);
FILE *logptr;

struct processData *head;
struct processData *currentProcess;
bool finishedProcesses = false;
int main(int argc, char *argv[])
{

    initialize_ipc();

    head = New_Process(0, 0, 0, 0);
    currentProcess = New_Process(0, 0, 0, 0);

    initClk();
    while (1)
    {
        if (finishedProcesses == false)
        {
            check_for_new_processes();
        }

        up(sem2Process);

        if (currentProcess->priority == 0 && head->priority != 0 && head->status == READY)
        {
            reset(sem2Process);
            start_process(head);
            down(sem1Process);
            usefulSeconds++;
            currentProcess->remainingTime--;
            currentProcess->priority--;
        }
        else if ((currentProcess->status == STARTED || currentProcess->status == RESUMED) && currentProcess->remainingTime != 0)
        {
            down(sem1Process);
            usefulSeconds++;
            currentProcess->remainingTime--;
            currentProcess->priority--;
        }
        else if (head->status == STOPPED && head->remainingTime != 0)
        {
            reset(sem2Process);
            resume_process(head);
            usefulSeconds++;
            down(sem1Process);
            currentProcess->remainingTime--;
            currentProcess->priority--;
        }

        if ((currentProcess->status == STARTED || currentProcess->status == RESUMED) && currentProcess->remainingTime == 0)
        {
            up(sem2Process);
            remove_process(currentProcess);
        }
        if (finishedProcesses == true && head->priority == 0 && currentProcess->priority == 0)
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
    head = *&pr;

    if (Is_Empty(&head))
    {
        head = New_Process(0, 0, 0, 0);
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
    head = *&p;

    if (Is_Empty(&head))
    {
        head = New_Process(0, 0, 0, 0);
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
    free(currentProcess);
    currentProcess = New_Process(0, 0, 0, 0);

}
void check_for_new_processes()
{
    while (1)
    {
        down(sem1);
        struct processData *pr = (struct processData *)malloc(sizeof(struct processData));
        if (shmaddrPG->id == -2)
        {
            up(sem2);
            finishedProcesses = true;
            return;
        }
        if (shmaddrPG->id == -1)
        {
            up(sem2);
            return;
        }
        if (head->priority == 0)
        {
            head = New_Process(shmaddrPG->arrivalTime, shmaddrPG->runningTime, shmaddrPG->runningTime, shmaddrPG->id);
        }
        else
        {
            pr = New_Process(shmaddrPG->arrivalTime, shmaddrPG->runningTime, shmaddrPG->runningTime, shmaddrPG->id);
            Enqueue(&head, &pr);
        }

        if (head->priority < currentProcess->priority)
        {
            stop_process(currentProcess);
            if (currentProcess->remainingTime != 0)
            {
                currentProcess->priority = currentProcess->remainingTime;
                Enqueue(&head, &currentProcess);
            }
            currentProcess = New_Process(0, 0, 0, 0);
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

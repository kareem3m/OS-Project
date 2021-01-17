#include "headers.h"
// #include "circular.h"
#include "P_Q.h"
#include <time.h>
//functions declerations
void start_process(struct processData *process);
void check_for_new_processes();
void wait_next_clk();
void remove_process(struct processData *p);
void log_status(struct processData *p, char *status);

FILE *logptr;

//Head of priority queue and the process that is running now 
struct processData *head;
struct processData *currentProcess;
//boolean to indicate that there is no more processes from the process generator
bool finishedProcesses = false;
int main(int argc, char *argv[])
{
    initialize_ipc();
    //intitialize the head of the priority queue
    currentProcess = New_Process(0, 0, 0, 0);
    head = New_Process(0, 0, 0, 0);
    //Current_Process=NULL;


    initClk();
    while (1)
    {
         //looking for new processes    
        if (finishedProcesses == false)
        {
            check_for_new_processes();
        }
        //starts a process if no process is running right now
        if (currentProcess->priority == 0 && head->priority != 0)
        {
            start_process(head);
            down(sem1Process);
            currentProcess->remainingTime--;
            usefulSeconds++;
        }
        //decrease the remaning time of the running process
        else if (currentProcess->status == STARTED && currentProcess->remainingTime > 0)
        {
            down(sem1Process);
            currentProcess->remainingTime--;
            usefulSeconds++;
        }
        //for synchronization on remaning time  between scheduler and process
        up(sem2Process);
        //remove process when it finishes
        if (currentProcess->status == STARTED && currentProcess->remainingTime == 0)
        {
            remove_process(currentProcess);
        }
        //exit after finishing all processes
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
            head = New_Process(shmaddrPG->arrivalTime, shmaddrPG->priority, shmaddrPG->runningTime, shmaddrPG->id);
        }
        else
        {
            pr = New_Process(shmaddrPG->arrivalTime, shmaddrPG->priority, shmaddrPG->runningTime, shmaddrPG->id);
            Enqueue(&head, &pr);
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
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

struct processData *Head;
struct processData *Current_Process;
bool Finished_Processes = false;
int main(int argc, char *argv[])
{

    initialize_ipc();

    Head = New_Process(0, 0, 0, 0);
    Current_Process = New_Process(0, 0, 0, 0);
    //Current_Process=NULL;

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
            Current_Process->remainingtime--;
            Current_Process->priority--;
        }
        else if ((Current_Process->status == STARTED || Current_Process->status == RESUMED) && Current_Process->remainingtime != 0)
        {
            down(sem1_process);
            useful_seconds++;
            Current_Process->remainingtime--;
            Current_Process->priority--;
        }
        else if (Head->status == STOPPED && Head->remainingtime != 0)
        {
            printf("here \n");
            fflush(stdout);
            reset(sem2_process);
            resume_process(Head);
            useful_seconds++;
            down(sem1_process);
            Current_Process->remainingtime--;
            Current_Process->priority--;
        }

        if ((Current_Process->status == STARTED || Current_Process->status == RESUMED) && Current_Process->remainingtime == 0)
        {
            up(sem2_process);
            remove_process(Current_Process);
            printf("Finished_Processes %d Head->priority %d Current_Process->priority %d \n", Finished_Processes, Head->priority, Current_Process->priority);
            fflush(stdout);
        }
        if (Finished_Processes == true && Head->priority == 0 && Current_Process->priority == 0)
        {
            printf("5ls");
            generate_perf_file();
            exit(0);
        }
    }

    return 0;
}

void start_process(struct processData *pr)
{
    Current_Process->id = pr->id;
    Current_Process->priority = pr->remainingtime;
    Current_Process->arrivaltime = pr->arrivaltime;
    Current_Process->runningtime = pr->runningtime;
    Current_Process->remainingtime = pr->remainingtime;
    Current_Process->last_run = pr->last_run;
    Current_Process->pid = pr->pid;
    Current_Process->status = pr->status;
    // Current_Process = &pr;
    Dequeue(&pr);
    Head = *&pr;

    if (Is_Empty(&Head))
    {
        Head = New_Process(0, 0, 0, 0);
    }

    Current_Process->wait += getClk() - Current_Process->arrivaltime;
    int pid = fork();
    if (pid == 0)
    {
        char arg[10];
        sprintf(arg, "%d", Current_Process->runningtime);
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
    Current_Process->priority = p->remainingtime;
    Current_Process->arrivaltime = p->arrivaltime;
    Current_Process->runningtime = p->runningtime;
    Current_Process->remainingtime = p->remainingtime;
    Current_Process->last_run = p->last_run;
    Current_Process->pid = p->pid;
    Current_Process->status = p->status;
    //Current_Process = &p;
    Dequeue(&p);
    Head = *&p;

    if (Is_Empty(&Head))
    {
        Head = New_Process(0, 0, 0, 0);
        printf("head priority in resume empty %d \n", Head->priority);
        fflush(stdout);
    }
    if (kill(Current_Process->pid, SIGCONT) == -1)
    {
        perror("Error in kill: ");
        exit(-1);
    }

    Current_Process->status = RESUMED;
    Current_Process->wait += getClk() - Current_Process->last_run;
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
    p->last_run = getClk();
    // p->remainingtime--;

    if (p->remainingtime != 0)
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
    //printf("batee5 fl remove\n");
    Pr->status = FINISHED;
    Pr->last_run = getClk();
    log_status(Pr, "finished");
    printf("%d in delation\n", Pr->priority);
    //Dequeue(&Pr);
    free(Current_Process);
    Current_Process = New_Process(0, 0, 0, 0);

    fflush(stdout);
    //free(temp);
}
void check_for_new_processes()
{
    while (1)
    {
        down(sem1);
        printf("SRTN: downed sem1\n");
        fflush(stdout);
        struct processData *pr = (struct processData *)malloc(sizeof(struct processData));
        if (shmaddr_pg->id == -2)
        {
            up(sem2);
            printf("SRTN: upped sem2 id = -2\n");
            fflush(stdout);
            Finished_Processes = true;
            return;
        }
        if (shmaddr_pg->id == -1)
        {
            up(sem2);
            printf("SRTN: upped sem2 id = -1\n");
            fflush(stdout);
            return;
        }
        printf("Process %d arrived at CLK = %d \n", shmaddr_pg->id, getClk());
        fflush(stdout);
        if (Head->priority == 0)
        {
            Head = New_Process(shmaddr_pg->arrivaltime, shmaddr_pg->runningtime, shmaddr_pg->runningtime, shmaddr_pg->id);
        }
        else
        {
            pr = New_Process(shmaddr_pg->arrivaltime, shmaddr_pg->runningtime, shmaddr_pg->runningtime, shmaddr_pg->id);
            Enqueue(&Head, &pr);
        }

        if (Head->priority < Current_Process->priority)
        {
            // printf("current status %d \n",Current_Process->status);
            stop_process(Current_Process);
            // printf("current status %d current remaning time %d \n",Current_Process->status,Current_Process->remainingtime);
            // fflush(stdout);
            if (Current_Process->remainingtime != 0)
            {
                printf("current status %d current remaning time %d \n", Current_Process->status, Current_Process->remainingtime);
                fflush(stdout);
                Current_Process->priority = Current_Process->remainingtime;
                Enqueue(&Head, &Current_Process);
                printf("Head status %dHead remaning time %d \n", Head->status, Head->remainingtime);
                fflush(stdout);
                printf("Next status %d Next remaning time %d \n", Head->Next->status, Head->Next->priority);
                fflush(stdout);
            }
            Current_Process = New_Process(0, 0, 0, 0);
        }

        printf("%d in creation\n", Head->priority);
        up(sem2);
        printf("SRTN: upped sem2\n");
        fflush(stdout);
    }
}

void wait_next_clk()
{
    int start = getClk();
    while (start == getClk())
        ;
}

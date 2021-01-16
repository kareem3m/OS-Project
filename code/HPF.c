#include "headers.h"
// #include "circular.h"
#include "P_Q.h"
#include <time.h>
#include "buddy-queue.h"
#include <sys/queue.h>

void start_process(struct processData *process);
void check_for_new_processes();
void wait_next_clk();
void remove_process(struct processData *p);
void log_status(struct processData *p, char *status);

FILE *logptr;

struct process
{
    struct processData data;
    CIRCLEQ_ENTRY(process)
    ptrs;
};

CIRCLEQ_HEAD(circlehead, process);

struct circlehead head_wait;

int wait_queue_size = 0;
struct circlehead *wait_queue;

struct processData *Head;
struct processData *Current_Process;
bool Finished_Processes = false;
int main(int argc, char *argv[])
{
    CIRCLEQ_INIT(&head_wait);
    wait_queue = &head_wait;
    initialise();

    initialize_ipc();
    //Head=(struct processData*)malloc(sizeof(struct processData));
    //Current_Process=(struct processData*)malloc(sizeof(struct processData));
    Current_Process = New_Process(0, 0, 0, 0);
    Head = New_Process(0, 0, 0, 0);
    //Current_Process=NULL;

    initClk();
    while (1)
    {
        if (Finished_Processes == false)
        {
            check_for_new_processes();
        }
        //if (!Is_Empty(&Head))
        //{
        if (Current_Process->priority == 0 && Head->priority != 0)
        {
            start_process(Head);
            down(sem1_process);
            Current_Process->remainingtime--;
            useful_seconds++;
        }
        else if (Current_Process->status == STARTED && Current_Process->remainingtime > 0)
        {
            down(sem1_process);
            Current_Process->remainingtime--;
            useful_seconds++;
            printf("remaningtime %d ", Current_Process->remainingtime);
        }
        up(sem2_process);
        if (Current_Process->status == STARTED && Current_Process->remainingtime == 0)
        {
            remove_process(Current_Process);
        }

        if (Finished_Processes == true && Head->priority == 0 && Current_Process->priority == 0)
        {
            printf("5ls");
            generate_perf_file();
            exit(0);
        }
        // }
    }

    return 0;
}

void start_process(struct processData *pr)
{
    Current_Process->id = pr->id;
    Current_Process->arrivaltime = pr->arrivaltime;
    Current_Process->last_run = pr->last_run;
    Current_Process->priority = pr->priority;
    Current_Process->remainingtime = pr->remainingtime;
    Current_Process->responsetime = pr->responsetime;
    Current_Process->runningtime = pr->runningtime;
    printf("%d %d %d %d %d", pr->id, pr->priority, pr->remainingtime, pr->arrivaltime, pr->status);
    //Current_Process=Dequeue(&pr);
    Dequeue(&pr);
    Head = *&pr;
    if (Is_Empty(&Head))
    {
        Head = New_Process(0, 0, 0, 0);
    }
    printf("start process %d %d %d %d %d \n", Current_Process->id, Current_Process->priority, Current_Process->remainingtime, Current_Process->arrivaltime, Current_Process->status);
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
        //Current_Process->remainingtime--;
    }
    printf("start process status %d \n", Current_Process->status);
    printf("start process remaningtime %d \n", Current_Process->remainingtime);
}
void remove_process(struct processData *Pr)
{

    if (kill(Pr->pid, SIGSTOP) == -1)
    {
        perror("Error in kill: ");
        exit(-1);
    }
    Pr->status = FINISHED;
    Pr->last_run = getClk();
    log_status(Pr, "finished");
    struct processData *temp = (struct processData *)malloc(sizeof(struct processData));
    printf("%d in delation\n", Pr->priority);
    deallocation(Pr->id);
    struct process *np = (struct process *)malloc(sizeof(struct process));
    CIRCLEQ_FOREACH(np, &head_wait, ptrs)
    {
        if (allocation(&np->data))
        {
            np->data.wait = getClk() - np->data.arrivaltime;
            // fadel el SIZE wel WAIT
            if (Head->priority == 0)
            {
                Head = New_Process(np->data.arrivaltime, np->data.priority, np->data.runningtime, np->data.id);
                printf("after enqueue %d \n", Head->id);
            }
            else
            {
                temp = New_Process(np->data.arrivaltime, np->data.priority, np->data.runningtime, np->data.id);
                printf("before enque %d \n", np->data.id);
                Enqueue(&Head, &temp);
                printf("%d in creation\n", temp->id);
            }

            CIRCLEQ_REMOVE(&head_wait, np, ptrs);
            wait_queue_size -= 1;
        }
    }
    //Dequeue(&Pr);
    //free(Current_Process);
    Current_Process = New_Process(0, 0, 0, 0);
    //free(temp);
}
void check_for_new_processes()
{
    while (1)
    {
        ///enbehar
        down(sem1);
        printf("HPF: downed sem1\n");
        fflush(stdout);
        struct processData *pr = (struct processData *)malloc(sizeof(struct processData));
        if (shmaddr_pg->id == -2)
        {
            up(sem2);
            printf("HPF: upped sem2 id = %d\n", shmaddr_pg->id);
            fflush(stdout);
            Finished_Processes = true;
            return;
        }
        if (shmaddr_pg->id == -1)
        {
            up(sem2);
            printf("HPF: upped sem2 id = %d\n", shmaddr_pg->id);
            fflush(stdout);
            return;
        }

        if (allocation(shmaddr_pg))
        {
            if (Head->priority == 0)
            {
                Head = New_Process(shmaddr_pg->arrivaltime, shmaddr_pg->priority, shmaddr_pg->runningtime, shmaddr_pg->id);
                printf("after enqueue %d \n", Head->id);
            }
            else
            {
                pr = New_Process(shmaddr_pg->arrivaltime, shmaddr_pg->priority, shmaddr_pg->runningtime, shmaddr_pg->id);
                printf("before enque %d \n", pr->id);
                Enqueue(&Head, &pr);
                printf("%d in creation\n", Head->id);
            }
        }
        else
        {
            struct process *p = (struct process *)malloc(sizeof(struct process));
            p->data.id = shmaddr_pg->id;
            p->data.arrivaltime = shmaddr_pg->arrivaltime;
            p->data.last_run = getClk();
            p->data.priority = shmaddr_pg->priority;
            p->data.runningtime = shmaddr_pg->runningtime;
            p->data.remainingtime = shmaddr_pg->runningtime;
            p->data.status = READY;
            p->data.wait = 0;
            p->data.size = shmaddr_pg->size;
            CIRCLEQ_INSERT_TAIL(wait_queue, p, ptrs);
            printf("at T = %d process %d arrived and waited\n", getClk(), p->data.id);
            fflush(stdout);
            wait_queue_size += 1;
        }

        up(sem2);
        printf("HPF: upped sem2\n");
        fflush(stdout);
    }
}

void wait_next_clk()
{
    int start = getClk();
    while (start == getClk())
        ;
}
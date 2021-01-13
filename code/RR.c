#include "headers.h"
#include "circular.h"
#include <sys/queue.h>
#include <time.h>

///////////////////////////////

int *count;
int sem1, sem2, sem3;

struct processData *shmaddrr;
////////////////////////////

struct process
{
    struct processData data;
    CIRCLEQ_ENTRY(process)
    ptrs;
};

CIRCLEQ_HEAD(circlehead, process);

struct circlehead *ready_queue;

struct process *current_process = NULL;

void start_process(struct process *process);
void resume_process(struct process *process);
void stop_process(struct process *p);
void remove_process(struct process *p);
void check_for_new_processes();
void wait_next_clk();
void log_status(struct process *p, char *status);

FILE *logptr;

int quantum = 1;

int main(int argc, char *argv[])
{
    struct circlehead head;
    CIRCLEQ_INIT(&head);
    ready_queue = &head;
    //////////////////////////////////////
    key_t key_id1 = ftok("keyfile", 68);
    key_t key_id2 = ftok("keyfile", 66);
    key_t key_id3 = ftok("keyfile", 67);
    key_t key_id4 = ftok("keyfile", 69);
    key_t key_id5 = ftok("keyfile", 70);
    int shmid = shmget(key_id2, sizeof(struct processData), IPC_CREAT | 0666);
    shmaddrr = (struct processData *)shmat(shmid, (void *)0, 0);

    if (shmaddrr == (void *)-1)
    {
        perror("Error in attach in reader");
        exit(-1);
    }

    sem1 = semget(key_id3, 1, 0666 | IPC_CREAT);
    sem2 = semget(key_id1, 1, 0666 | IPC_CREAT);
    sem3 = semget(key_id4, 1, 0666 | IPC_CREAT);
    if (sem1 == -1 || sem2 == -1 || sem3 == -1)
    {
        perror("Error in create sem");
        exit(-1);
    }
    /////////////////////////////////////
    quantum = 1; //atoi(argv[1]);

    initClk();

    while (1)
    {
        check_for_new_processes();
        if (!CIRCLEQ_EMPTY(ready_queue))
        {
            if (current_process->data.status == READY)
            {
                start_process(current_process);
            }
            else if (current_process->data.status == STOPPED)
            {
                resume_process(current_process);
            }

            wait_next_clk();

            if (current_process != CIRCLEQ_LOOP_NEXT(ready_queue, current_process, ptrs))
            {
                stop_process(current_process);
                current_process = CIRCLEQ_LOOP_NEXT(ready_queue, current_process, ptrs);
            }
        }
    }

    return 0;
}

void start_process(struct process *p)
{
    p->data.wait += getClk() - p->data.arrivaltime;
    int pid = fork();
    if (pid == 0)
    {
        char arg[10];
        sprintf(arg, "%d", p->data.runningtime);
        int ret = execl("process", "process", arg, NULL);
        perror("Error in execl: ");
        exit(-1);
    }
    else
    {
        p->data.pid = pid;
        p->data.status = STARTED;
        log_status(p, "started");
    }
}

void resume_process(struct process *p)
{
    if (kill(p->data.pid, SIGCONT) == -1)
    {
        perror("Error in kill: ");
        exit(-1);
    }
    p->data.status = RESUMED;
    p->data.wait += getClk() - p->data.last_run;
    log_status(p, "resumed");
}

void stop_process(struct process *p)
{
    if (kill(p->data.pid, SIGSTOP) == -1)
    {
        perror("Error in kill: ");
        exit(-1);
    }
    p->data.status = STOPPED;
    p->data.last_run = getClk();
    p->data.remainingtime--;

    if (p->data.remainingtime != 0)
    {
        log_status(p, "stopped");
    }
    else
    {
        log_status(p, "finished");
        remove_process(p);
    }
}

void remove_process(struct process *p)
{
    current_process = CIRCLEQ_LOOP_NEXT(ready_queue, current_process, ptrs);
    CIRCLEQ_REMOVE(ready_queue, p, ptrs);
}

void check_for_new_processes()
{
    while (1)
    {
        down(sem1);
        printf("RR: downed sem1\n");
        fflush(stdout);
        struct process *p = (struct process *)malloc(sizeof(struct process));
        if (shmaddrr->id == -1)
        {
            up(sem2);
            printf("RR: upped sem2\n");
            fflush(stdout);
            return;
        }

        p->data.id = shmaddrr->id;
        p->data.arrivaltime = shmaddrr->arrivaltime;
        p->data.last_run = getClk();
        p->data.priority = shmaddrr->priority;
        p->data.runningtime = shmaddrr->runningtime;
        p->data.remainingtime = shmaddrr->runningtime;
        p->data.status = READY;
        p->data.wait = 0;

        if (CIRCLEQ_EMPTY(ready_queue))
        {
            CIRCLEQ_INSERT_HEAD(ready_queue, p, ptrs);
            current_process = p;
        }
        else
        {
            CIRCLEQ_INSERT_BEFORE(ready_queue, current_process, p, ptrs);
        }
        up(sem2);
        printf("RR: upped sem2\n");
        fflush(stdout);
    }
}

void wait_next_clk()
{
    int start = getClk();
    while (start == getClk())
        ;
}

void log_status(struct process *p, char *status)
{
    logptr = fopen("scheduler.log", "a");
    int ret = fprintf(logptr, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), p->data.id, status, p->data.arrivaltime, p->data.runningtime, p->data.remainingtime, p->data.wait);
    fclose(logptr);
}
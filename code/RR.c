#include "headers.h"
#include <sys/queue.h>
#include <time.h>

struct process
{
    struct processData data;
    CIRCLEQ_ENTRY(process)
    ptrs;
};

CIRCLEQ_HEAD(circlehead, process);

struct circlehead *ready_queue;

struct process *current_process = NULL;

void schedule_process();
void start_process(struct process *process);
void resume_process(struct process *process);
void stop_process(struct process *p);
void remove_process(struct process *p);
void get_new_processes();
void log_status(struct process *p, char *status);
void wait_on_process(struct process *p);

int quantum = 1;
int remaining_quantum_time = 1;
int ready_queue_size = 0;



int main(int argc, char *argv[])
{
    initialize_ipc();
    struct circlehead head;
    CIRCLEQ_INIT(&head);
    ready_queue = &head;

    quantum = 1; //atoi(argv[1]);
    remaining_quantum_time = quantum;

    up(scheduler_ready);

    initClk();

    while (1)
    {

        if (ready_queue_size > 0)
        {
            schedule_process();
        }
        else
        {
            get_new_processes();
        }
    }

    return 0;
}

void schedule_process()
{
    remaining_quantum_time--;

    int status = current_process->data.status;

    printf("T = %d , current process = %d\n", getClk(), current_process->data.id);
    if (status == READY)
    {
        start_process(current_process);
    }
    else if (status == STOPPED)
    {
        resume_process(current_process);
    }

    down(sem1_process);

    current_process->data.remainingtime--;

    get_new_processes();

    if (current_process->data.remainingtime == 0)
    {
        up(sem2_process);
        remove_process(current_process);
    }
    else
    {
        if (remaining_quantum_time == 0 && ready_queue_size > 1)
        {
            stop_process(current_process);
            current_process = CIRCLEQ_LOOP_NEXT(ready_queue, current_process, ptrs);
        }
        else
        {
            up(sem2_process);
        }
    }
    if (remaining_quantum_time == 0)
    {
        remaining_quantum_time = quantum;
    }
}

void start_process(struct process *p)
{
    printf("starting process %d\n", p->data.id);
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
        p->data.status = RUNNING;
        log_status(p, "started");
    }
}

void resume_process(struct process *p)
{
    printf("resuming process %d\n", p->data.id);
    p->data.wait += getClk() - p->data.last_run;
    if (p->data.status != RUNNING)
    {
        p->data.status = RUNNING;
        kill(p->data.pid, SIGCONT) == -1 ? perror("Error in resume_process: ") : 0;
    }

    wait_on_process(p);
}

void stop_process(struct process *p)
{
    printf("stopping process %d\n", p->data.id);
    kill(p->data.pid, SIGSTOP) == -1 ? perror("Error in stop_process: ") : 0;
    p->data.status = STOPPED;
    p->data.last_run = getClk();
    wait_on_process(p);
}

void remove_process(struct process *p)
{
    printf("removing process %d\n", p->data.id);
    fflush(stdout);
    wait_on_process(p);
    if (ready_queue_size > 1)
    {
        current_process = CIRCLEQ_LOOP_NEXT(ready_queue, current_process, ptrs);
    }
    else
    {
        current_process = NULL;
    }
    CIRCLEQ_REMOVE(ready_queue, p, ptrs);
    ready_queue_size--;
}

void get_new_processes()
{
    while (1)
    {
        down(sem1);
        struct process *p = (struct process *)malloc(sizeof(struct process));
        if (shmaddr_pg->id == -1)
        {
            up(sem2);
            return;
        }

        p->data.id = shmaddr_pg->id;
        p->data.arrivaltime = shmaddr_pg->arrivaltime;
        p->data.last_run = getClk();
        p->data.priority = shmaddr_pg->priority;
        p->data.runningtime = shmaddr_pg->runningtime;
        p->data.remainingtime = shmaddr_pg->runningtime;
        p->data.status = READY;
        p->data.wait = 0;


        if (CIRCLEQ_EMPTY(ready_queue))
        {
            CIRCLEQ_INSERT_HEAD(ready_queue, p, ptrs);
            current_process = p;
            printf("at T = %d process %d arrived\n", getClk(), p->data.id);
            fflush(stdout);
        }
        else
        {
            CIRCLEQ_INSERT_BEFORE(ready_queue, current_process, p, ptrs);
            printf("at T = %d process %d arrived\n", getClk(), p->data.id);
            fflush(stdout);
        }

        ready_queue_size += 1;
        up(sem2);
        printf("RR: upped sem2\n");
        fflush(stdout);
    }
}

void wait_on_process(struct process *p)
{
    siginfo_t siginfo;
    waitid(P_PID, p->data.pid, &siginfo, WCONTINUED | WSTOPPED | WEXITED);
    char *status;
    switch (siginfo.si_code)
    {
    case CLD_CONTINUED:
        status = "resumed";
        break;
    case CLD_STOPPED:
        status = "stopped";
        break;
    case CLD_EXITED:
        status = "finished";
        break;
    default:
        printf(status, "%d", siginfo.si_code);
        break;
    }
    log_status(p, status);
}

FILE *logptr;
void log_status(struct process *p, char *status)
{
    logptr = fopen("scheduler.log", "a");
    int ret = fprintf(logptr, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), p->data.id, status, p->data.arrivaltime, p->data.runningtime, p->data.remainingtime, p->data.wait);
    fflush(NULL);
    fclose(logptr);
}

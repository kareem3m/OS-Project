
#include <sys/queue.h>
#include <time.h>
#include "buddy-queue.h"
struct process
{
    struct processData data;
    CIRCLEQ_ENTRY(process)
    ptrs;
};

CIRCLEQ_HEAD(circlehead, process);

struct circlehead *ready_queue;
struct circlehead *wait_queue;

struct process *current_process = NULL;

void schedule_process();
void start_process(struct process *process);
void resume_process(struct process *process);
void stop_process(struct process *p);
void remove_process(struct process *p);
void get_new_processes();
void wait_on_process(struct process *p);

int quantum = 1;
int remaining_quantum_time = 1;
int ready_queue_size = 0;
int wait_queue_size = 0;
bool no_more_new_processes = false;
struct circlehead head_wait;

int main(int argc, char *argv[])
{
    initialize_ipc();
    struct circlehead head;
    CIRCLEQ_INIT(&head);
    ready_queue = &head;

    CIRCLEQ_INIT(&head_wait);
    wait_queue = &head_wait;

    quantum = atoi(argv[1]);
    remaining_quantum_time = quantum;

    initialise();
    initClk();

    while (1)
    {
        if (ready_queue_size > 0)
        {
            schedule_process();
        }
        else if (no_more_new_processes)
        {
            break;
        }
        else
        {
            get_new_processes();
        }
    }

    generate_perf_file();
    return 0;
}

void schedule_process()
{
    remaining_quantum_time--;

    int status = current_process->data.status;

    if (status == READY)
    {
        start_process(current_process);
    }
    else if (status == STOPPED)
    {
        resume_process(current_process);
    }

    useful_seconds++;
    down(sem1_process);
    current_process->data.remainingTime--;

    no_more_new_processes ? 0 : get_new_processes();

    if (current_process->data.remainingTime == 0)
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
    p->data.wait += getClk() - p->data.arrivalTime;
    int pid = fork();
    if (pid == 0)
    {
        char arg[10];
        sprintf(arg, "%d", p->data.runningTime);
        int ret = execl("process", "process", arg, NULL);
        perror("Error in execl: ");
        exit(-1);
    }
    else
    {
        p->data.pid = pid;
        p->data.status = RUNNING;
        log_status(&(p->data), "started");
    }
}

void resume_process(struct process *p)
{
    p->data.wait += getClk() - p->data.lastRun;
    if (p->data.status != RUNNING)
    {
        p->data.status = RUNNING;
        kill(p->data.pid, SIGCONT) == -1 ? perror("Error in resume_process: ") : 0;
    }

    wait_on_process(p);
}

void stop_process(struct process *p)
{
    kill(p->data.pid, SIGSTOP) == -1 ? perror("Error in stop_process: ") : 0;
    p->data.status = STOPPED;
    p->data.lastRun = getClk();
    wait_on_process(p);
}

void remove_process(struct process *p)
{
    p->data.status = FINISHED;
    wait_on_process(p);

    //remove process from memory and queue and check if there is a process in wait queue allocate it if the space in memory allowing
    deallocation(p->data.id);
    struct process *np = (struct process *)malloc(sizeof(struct process));
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
    if (wait_queue_size > 0)
    {
        CIRCLEQ_FOREACH(np, &head_wait, ptrs)
        {
            if (allocation(&np->data))
            {
                struct process *p = (struct process *)malloc(sizeof(struct process));
                p->data.id = np->data.id;
                p->data.arrivalTime = np->data.arrivalTime;
                p->data.lastRun = getClk();
                p->data.priority = np->data.priority;
                p->data.runningTime = np->data.runningTime;
                p->data.remainingTime = np->data.runningTime;
                p->data.status = READY;
                p->data.wait = 0;
                p->data.size = np->data.size;
                p->data.wait = getClk() - np->data.arrivalTime;
                
                if (CIRCLEQ_EMPTY(ready_queue))
                {
                    CIRCLEQ_INSERT_HEAD(ready_queue, p, ptrs);
                    current_process = p;

                }
                else
                {
                    CIRCLEQ_INSERT_BEFORE(ready_queue, current_process, p, ptrs);
                }
                ready_queue_size += 1;
                CIRCLEQ_REMOVE(&head_wait, np, ptrs);
                wait_queue_size -= 1;
            }
        }
    }
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
        else if (shmaddr_pg->id == -2)
        {
            no_more_new_processes = true;
            up(sem2);
            return;
        }

        p->data.id = shmaddr_pg->id;
        p->data.arrivalTime = shmaddr_pg->arrivalTime;
        p->data.lastRun = getClk();
        p->data.priority = shmaddr_pg->priority;
        p->data.runningTime = shmaddr_pg->runningTime;
        p->data.remainingTime = shmaddr_pg->runningTime;
        p->data.status = READY;
        p->data.wait = 0;
        p->data.size = shmaddr_pg->size;

        //check if there is a space in memory allocate it and put in in the ready queue else put it in wait queue
        if (allocation(&p->data))
        {
            if (CIRCLEQ_EMPTY(ready_queue))
            {
                CIRCLEQ_INSERT_HEAD(ready_queue, p, ptrs);
                current_process = p;
            }
            else
            {
                CIRCLEQ_INSERT_BEFORE(ready_queue, current_process, p, ptrs);
            }
            ready_queue_size += 1;
        }
        else
        {

            CIRCLEQ_INSERT_TAIL(wait_queue, p, ptrs);
            wait_queue_size += 1;
        }
        up(sem2);
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
    log_status(&(p->data), status);
}
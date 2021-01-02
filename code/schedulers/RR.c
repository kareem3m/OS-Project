#include "../headers.h"
#include "../circular.h"
#include <sys/queue.h>
#include <time.h>

void start(struct processData *process);
void resume(struct processData *process);
void stop(struct processData *process);
void check_for_new_process();
void child_exit_handler(int signum);
void sleep_a_quantum();

int quantum = 1;

struct queue *ready_queue;

int main(int argc, char *argv[])
{
    quantum = 1; //atoi(argv[1]);
    signal(SIGCHLD, child_exit_handler);

    initClk();

    struct queue *ready_queue = newQueue(100);

    struct processData p1;
    p1.id = -1;
    p1.status = READY;
    p1.runningtime = 10;
    enqueue(ready_queue, p1);

    struct processData p2;
    p2.id = -1;
    p2.status = READY;
    p2.runningtime = 5;
    enqueue(ready_queue, p2);

    struct processData process;
    while (1)
    {
        if (size(ready_queue) > 0)
        {
            check_for_new_process();

            process = front(ready_queue);
            dequeue(ready_queue);

            if (process.status == READY)
            {
                start(&process);
            }
            else if (process.status == STOPPED)
            {
                resume(&process);
            }

            sleep_a_quantum();

            stop(&process);

            enqueue(ready_queue, process);
        }
        else
        {
            check_for_new_process();
            sleep_a_quantum();
        }
    }

    return 0;
}

void start(struct processData *process)
{
    int pid = fork();
    if (pid == 0)
    {
        char arg[10];
        sprintf(arg, "%d", process->runningtime);
        int ret = execl("process", "process", arg, NULL);
        perror("Error in execl: ");
        exit(-1);
    }
    else
    {
        process->id = pid;
        process->status = STARTED;
        printf("process %d running..\n", process->id);
        fflush(stdout);
    }
}

void resume(struct processData *process)
{
    kill(process->id, SIGCONT);
    process->status = RESUMED;
    printf("process %d resumed\n", process->id);
    fflush(stdout);
}

void stop(struct processData *process)
{
    kill(process->id, SIGSTOP);
    process->status = STOPPED;
    printf("process %d stopped\n", process->id);
    fflush(stdout);
}

void child_exit_handler(int signum)
{
    int pid = waitpid(-1, NULL, WNOHANG);
    if (pid != 0 && pid != -1)
    {
        printf("prcoess %d finished\n", pid);
        fflush(stdout);
    }
}

void check_for_new_process()
{
}

void sleep_a_quantum()
{
    int start = getClk();
    while (getClk() - start < quantum)
    {
        sleep(quantum - (getClk() - start));
    }
}

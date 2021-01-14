#include "headers.h"
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <string.h>

void clearResources(int);
void on_clock_tick(int signum);

struct queue *readFile(struct queue *pt, char path[])
{

    FILE *filePointer;
    int bufferLength = 255;
    int lines = 0;
    char buffer[bufferLength];
    int ch = 0;
    filePointer = fopen(path, "r");
    while (!feof(filePointer))
    {
        ch = fgetc(filePointer);
        if (ch == '\n')
        {
            lines++;
        }
    }
    struct processData p;
    fclose(filePointer);
    pt = newQueue((lines - 1));
    filePointer = fopen(path, "r");
    fgets(buffer, bufferLength, filePointer);
    while (fgets(buffer, bufferLength, filePointer))
    {
        int id, arrivaltime, runningtime, priority;
        sscanf(buffer, "%d\t%d\t%d\t%d\n", &p.id, &p.arrivaltime, &p.runningtime, &p.priority);
        enqueue(pt, p);
        printf("%d\t%d\t%d\t%d\n", p.id, p.arrivaltime, p.runningtime, p.priority);
    }
    fclose(filePointer);
    return pt;
}

struct msgbuff;
/* arg for semctl system calls. */
union Semun
{
    int val;               /* value for SETVAL */
    struct semid_ds *buf;  /* buffer for IPC_STAT & IPC_SET */
    ushort *array;         /* array for GETALL & SETALL */
    struct seminfo *__buf; /* buffer for IPC_INFO */
    void *__pad;
};

pid_t scheduler_id, clk_id;

struct queue *pData;

int main(int argc, char *argv[])
{
    signal(SIGUSR1, on_clock_tick);
    signal(SIGINT, clearResources);
    // TODO Initialization
    // 1. Read the input files.
    char path[] = "processes.txt";

    pData = readFile(pData, path);
    int schedulingAlgorithm;
    int quantam;
    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    printf("Please enter the scheduling algorithm number #1 for HPF, #2 for SRTN, #3 for RR:\n");
    scanf("%d", &schedulingAlgorithm);
    printf("Please enter the quantum for RR or 0 if not:\n");
    scanf("%d", &quantam);
    printf("pid: %d\n", getpid());

    initialize_ipc();

    union Semun semun;

    semun.val = 0; /* initial value of the semaphore, Binary semaphore */
    if (semctl(sem1, 0, SETVAL, semun) == -1)
    {
        perror("Error in semctl");
        exit(-1);
    }
    semun.val = 0; /* initial value of the semaphore, Binary semaphore */
    if (semctl(sem2, 0, SETVAL, semun) == -1)
    {
        perror("Error in semctl");
        exit(-1);
    }
    semun.val = 0;
    if (semctl(scheduler_ready, 0, SETVAL, semun) == -1)
    {
        perror("Error in semctl");
        exit(-1);
    }
    semun.val = 0;
    if (semctl(sem1_process, 0, SETVAL, semun) == -1)
    {
        perror("Error in semctl");
        exit(-1);
    }
    semun.val = 0;
    if (semctl(sem2_process, 0, SETVAL, semun) == -1)
    {
        perror("Error in semctl");
        exit(-1);
    }

    printf("\nReader: Shared memory attached at address %d\n", scheduler_id);

    scheduler_id = fork();
    if (scheduler_id == -1)
    {
        printf("There is an error while calling fork()");
    }
    if (scheduler_id == 0)
    {
        printf("pid: %d ppid: %d\n", getpid(), getppid());
        char *args[] = {"2", "C", "Programming", NULL};
        int ret = execv("./RR", args);
        printf("%d\n", ret);
    }
    else
    {
        down(scheduler_ready);
        clk_id = fork();
        if (clk_id == -1)
        {
            printf("There is an error while calling fork()");
        }
        if (clk_id == 0)
        {
            printf("pid: %d ppid: %d\n", getpid(), getppid());
            char *args[] = {"clock", "C", "Programming", NULL};
            execv("./clk", args);
        }
    }
    // 3. Initiate and create the scheduler and clock processes.
    // 4. Use this function after creating the clock process to initialize clock
    initClk();
    // To get time use this
    int x = getClk();
    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.
    // 6. Send the information to the scheduler at the appropriate time.

   // on_clock_tick(SIGUSR1); // intial call at T = 0, later on the clock will be triggering it every sec

    while (1)
    {
        pause();
    }

    // 7. Clear clock resources
    destroyClk(true);
}

void on_clock_tick(int signum)
{
    initClk();
    while (!isEmpty(pData) && front(pData).arrivaltime == getClk())
    {
        *shmaddr_pg = front(pData);
        up(sem1);
        down(sem2);
        dequeue(pData);
    }

    shmaddr_pg->id = -1;
    up(sem1);
    down(sem2);
}

void clearResources(int signum)
{
    //TODO Clears all resources in case of interruption
    kill(clk_id, SIGKILL);
    kill(scheduler_id, SIGKILL);
    exit(0);
}

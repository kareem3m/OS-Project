#include "headers.h"
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>
#include "circular.h"
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <string.h>

void clearResources(int);

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
    pt = newQueue((lines));
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

void down(int sem)
{
    struct sembuf p_op;

    p_op.sem_num = 0;
    p_op.sem_op = -1;
    p_op.sem_flg = !IPC_NOWAIT;

    if (semop(sem, &p_op, 1) == -1)
    {
        //perror("Error in down()");
        //exit(-1);
    }
    else
    {
        //printf("\nData found = %s\n", (char *)shmaddr);
    }
}

void up(int sem)
{
    struct sembuf v_op;

    v_op.sem_num = 0;
    v_op.sem_op = 1;
    v_op.sem_flg = !IPC_NOWAIT;

    if (semop(sem, &v_op, 1) == -1)
    {
        //perror("Error in up()");
        //exit(-1);
    }
}

pid_t scheduler_id, clk_id;

int main(int argc, char *argv[])
{
    signal(SIGINT, clearResources);
    // TODO Initialization
    // 1. Read the input files.
    struct queue *pData;
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

    //scheduler_id = fork();
    //if (scheduler_id == -1)
    //{
    //    printf("There is an error while calling fork()");
    //}
    //if (scheduler_id == 0)
    //{
    //    printf("pid: %d ppid: %d\n", getpid(), getppid());
    //    char *args[] = {"2", "C", "Programming", NULL};
    //    int ret = execv("./scheduler", args);
    //    printf("%d\n", ret);
    //}
    //else
    //{
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
    //}
    // 3. Initiate and create the scheduler and clock processes.
    // 4. Use this function after creating the clock process to initialize clock
    initClk();
    // To get time use this
    int x = getClk();
    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.
    // 6. Send the information to the scheduler at the appropriate time.
    int shmid;
    key_t key_id1, key_id2, key_id3, key_id4;
    key_id2 = ftok("keyfile", 66);
    key_id4 = ftok("keyfile", 69);
    shmid = shmget(key_id2, sizeof(struct processData), IPC_CREAT | 0666);
    struct processData *shmaddr = (struct processData *)shmat(shmid, (void *)0, 0);
    if (shmaddr == -1)
    {
        perror("Error in attach in reader");
        exit(-1);
    }

    union Semun semun;
    key_id3 = ftok("keyfile", 67);
    key_id1 = ftok("keyfile", 68);

    int sem1 = semget(key_id3, 1, 0666 | IPC_CREAT);
    int sem2 = semget(key_id1, 1, 0666 | IPC_CREAT);
    int sem3 = semget(key_id4, 1, 0666 | IPC_CREAT);
    if (sem1 == -1 || sem2 == -1 || sem3 == -1)
    {
        perror("Error in create sem");
        exit(-1);
    }

    semun.val = 0; /* initial value of the semaphore, Binary semaphore */
    if (semctl(sem1, 0, SETVAL, semun) == -1)
    {
        perror("Error in semctl");
        exit(-1);
    }
    semun.val = 1; /* initial value of the semaphore, Binary semaphore */
    if (semctl(sem2, 0, SETVAL, semun) == -1)
    {
        perror("Error in semctl");
        exit(-1);
    }
    semun.val = 0; /* initial value of the semaphore, Binary semaphore */
    if (semctl(sem3, 0, SETVAL, semun) == -1)
    {
        perror("Error in semctl");
        exit(-1);
    }
    printf("\nReader: Shared memory attached at address %d\n", scheduler_id);
    struct processData process;
    while (!isEmpty(pData))
    {
        x = getClk();
        process = front(pData);
        if (process.arrivaltime == x)
        {
            down(sem2);
            shmaddr[0] = process;
            printf("%d\n", shmaddr[0].arrivaltime);
            dequeue(pData);
            up(sem1);
        }
        //else
        //{
        //    up(sem3);
        //}
    }
    // 7. Clear clock resources
    destroyClk(true);
}

void clearResources(int signum)
{
    //TODO Clears all resources in case of interruption
    kill(clk_id, SIGKILL);
    kill(scheduler_id, SIGKILL);
    exit(0);
}

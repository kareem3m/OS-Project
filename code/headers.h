#include <stdio.h> //if you don't use scanf/printf change this include
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "circular.h"
#include <math.h>
#include <time.h>

typedef short bool;
#define true 1
#define false 0

#define SHKEY 300

///==============================
//don't mess with this variable//
int *shmaddr; //
//===============================

int getClk()
{
    return *shmaddr;
}

/*
 * All process call this function at the beginning to establish communication between them and the clock module.
 * Again, remember that the clock is only emulation!
*/

int shmid_clk;
void initClk()
{
    shmid_clk = shmget(SHKEY, 4, 0444);
    while ((int)shmid_clk == -1)
    {
        //Make sure that the clock exists
        printf("%d: Wait! The clock not initialized yet!\n", getpid());
        usleep(250000);
        shmid_clk = shmget(SHKEY, 4, 0444);
    }
    shmaddr = (int *)shmat(shmid_clk, (void *)0, 0);
}

/*
 * All process call this function at the end to release the communication
 * resources between them and the clock module.
 * Again, Remember that the clock is only emulation!
 * Input: terminateAll: a flag to indicate whether that this is the end of simulation.
 *                      It terminates the whole system and releases resources.
*/

void destroyClk(bool terminateAll)
{
    shmdt(shmaddr);
    if (terminateAll)
    {
        killpg(getpgrp(), SIGINT);
    }
}

int sem1, sem2, scheduler_ready, sem1_process, sem2_process;
int shmid_pg;
struct processData *shmaddr_pg;

void initialize_ipc()
{
    key_t key_id1 = ftok("keyfile", 68);
    key_t key_id2 = ftok("keyfile", 66);
    key_t key_id3 = ftok("keyfile", 67);
    key_t key_id4 = ftok("keyfile", 69);
    key_t key_id5 = ftok("keyfile", 70);
    key_t key_id10 = ftok("keyfile", 90);
    key_t key_id11 = ftok("keyfile", 91);
    key_t key_id12 = ftok("keyfile", 92);

    shmid_pg = shmget(key_id2, sizeof(struct processData), IPC_CREAT | 0666);
    shmaddr_pg = (struct processData *)shmat(shmid_pg, (void *)0, 0);

    if (shmaddr_pg == (void *)-1)
    {
        perror("Error in attach in reader");
        exit(-1);
    }

    sem1 = semget(key_id3, 1, 0666 | IPC_CREAT);
    sem2 = semget(key_id1, 1, 0666 | IPC_CREAT);
    sem1_process = semget(key_id11, 1, 0666 | IPC_CREAT);
    sem2_process = semget(key_id12, 1, 0666 | IPC_CREAT);

    scheduler_ready = semget(key_id10, 1, 0666 | IPC_CREAT);

    if (sem1 == -1 || sem2 == -1 || scheduler_ready == -1)
    {
        perror("Error in create sem");
        exit(-1);
    }
}

void remove_ipc()
{
    shmdt(shmaddr);
    shmdt(shmaddr_pg);
    shmctl(shmid_clk, IPC_RMID, (struct shmid_ds *)0);
    shmctl(shmid_pg, IPC_RMID, (struct shmid_ds *)0);

    semctl(sem1, 0, IPC_RMID);
    semctl(sem2, 0, IPC_RMID);
    semctl(sem1_process, 0, IPC_RMID);
    semctl(sem2_process, 0, IPC_RMID);
    semctl(scheduler_ready, 0, IPC_RMID);
}

void up(int sem)
{
    struct sembuf v_op;

    v_op.sem_num = 0;
    v_op.sem_op = 1;
    v_op.sem_flg = !IPC_NOWAIT;

    if (semop(sem, &v_op, 1) == -1)
    {
        printf("pid = %d ", getpid());
        fflush(NULL);
        perror(" Error in up(): ");
        exit(-1);
    }
}

void down(int sem)
{
    struct sembuf p_op;

    p_op.sem_num = 0;
    p_op.sem_op = -1;
    p_op.sem_flg = !IPC_NOWAIT;

    if (semop(sem, &p_op, 1) == -1)
    {
        printf("pid = %d ", getpid());
        fflush(NULL);
        perror("Error in down()");
        //exit(-1);
    }
}

char* getRealTime() // For debugging purposes..
{
    long ms;  // Milliseconds
    time_t s; // Seconds
    struct timespec spec;

    clock_gettime(CLOCK_REALTIME, &spec);

    s = spec.tv_sec;
    ms = round(spec.tv_nsec / 1.0e6); // Convert nanoseconds to milliseconds
    if (ms > 999)
    {
        s++;
        ms = 0;
    }
    char* time = (char*)malloc(10);
    sprintf(time, "%lds %ldms", s, ms);
    return time;
}

// schedulers common

float wta[1000];
float waitsum = 0;
int n = -1;
float useful_seconds = 0;

FILE *logptr;
void log_status(struct processData *p, char *status)
{
    logptr = fopen("scheduler.log", "a");

    int ret = fprintf(logptr, "At time %d process %d %s arr %d total %d remain %d wait %d", getClk(), p->id, status, p->arrivaltime, p->runningtime, p->remainingtime, p->wait);

    if (p->status == FINISHED)
    {
        float TA = getClk() - p->arrivaltime;
        float WTA = TA / p->runningtime;
        fprintf(logptr, " TA = %d WTA %.2f \n", (int)TA, WTA);
        n++;
        wta[n] = WTA;
        waitsum += p->wait;
    }
    else
    {
        fprintf(logptr, "\n");
    }
    fflush(NULL);
    fclose(logptr);
}

float wtasum()
{
    float sum = 0;
    for (int i = 0; i <= n; i++)
    {
        sum += wta[i];
    }
    return sum;
}

float stdWTA()
{
    float mean = wtasum() / (n + 1);
    float sum = 0;
    for (int i = 0; i <= n; i++)
    {
        sum += (wta[i] - mean) * (wta[i] - mean);
    }
    return sqrt(sum);
}

void generate_perf_file()
{
    FILE *fptr = fopen("scheduler.perf", "w+");
    float cpu_util = useful_seconds * 100 / getClk();
    fprintf(fptr, "CPU utilization = %.2f%%\n", cpu_util);
    fprintf(fptr, "Avg WTA = %.2f\n", wtasum() / (n + 1));
    fprintf(fptr, "Avg Waiting = %.2f\n", waitsum / (n + 1));
    fprintf(fptr, "Std WTA = %.2f", stdWTA());
    //printf("n = %d\n", n);
    fclose(fptr);
}
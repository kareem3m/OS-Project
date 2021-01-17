#pragma once
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

int shmidClk;
void initClk()
{
    shmidClk = shmget(SHKEY, 4, 0444);
    while ((int)shmidClk == -1)
    {
        //Make sure that the clock exists
        printf("%d: Wait! The clock not initialized yet!\n", getpid());
        usleep(250000);
        shmidClk = shmget(SHKEY, 4, 0444);
    }
    shmaddr = (int *)shmat(shmidClk, (void *)0, 0);
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

int sem1, sem2, sem1Process, sem2Process;
int shmidPG;
struct processData *shmaddrPG;

void initialize_ipc()
{
    key_t keyID1 = ftok("keyfile", 68);
    key_t keyID2 = ftok("keyfile", 66);
    key_t keyID3 = ftok("keyfile", 67);
    key_t keyID4 = ftok("keyfile", 69);
    key_t keyID5 = ftok("keyfile", 70);
    key_t keyID10 = ftok("keyfile", 90);
    key_t keyID11 = ftok("keyfile", 91);
    key_t keyID12 = ftok("keyfile", 92);

    shmidPG = shmget(keyID2, sizeof(struct processData), IPC_CREAT | 0666);
    shmaddrPG = (struct processData *)shmat(shmidPG, (void *)0, 0);

    if (shmaddrPG == (void *)-1)
    {
        perror("Error in attach in reader");
        exit(-1);
    }

    sem1 = semget(keyID3, 1, 0666 | IPC_CREAT);
    sem2 = semget(keyID1, 1, 0666 | IPC_CREAT);
    sem1Process = semget(keyID11, 1, 0666 | IPC_CREAT);
    sem2Process = semget(keyID12, 1, 0666 | IPC_CREAT);

    if (sem1 == -1 || sem2 == -1 || sem1Process == -1 || sem2Process == -1)
    {
        perror("Error in create sem");
        exit(-1);
    }
}

void remove_ipc()
{
    shmdt(shmaddr);
    shmdt(shmaddrPG);
    shmctl(shmidClk, IPC_RMID, (struct shmid_ds *)0);
    shmctl(shmidPG, IPC_RMID, (struct shmid_ds *)0);

    semctl(sem1, 0, IPC_RMID);
    semctl(sem2, 0, IPC_RMID);
    semctl(sem1Process, 0, IPC_RMID);
    semctl(sem2Process, 0, IPC_RMID);
}

void up(int sem)
{
    struct sembuf v_op;

    v_op.sem_num = 0;
    v_op.sem_op = 1;
    v_op.sem_flg = !IPC_NOWAIT;

    semop(sem, &v_op, 1);
}

void down(int sem)
{
    struct sembuf p_op;

    p_op.sem_num = 0;
    p_op.sem_op = -1;
    p_op.sem_flg = !IPC_NOWAIT;

    semop(sem, &p_op, 1);
}

union Semun
{
    int val;               /* value for SETVAL */
    struct semid_ds *buf;  /* buffer for IPC_STAT & IPC_SET */
    ushort *array;         /* array for GETALL & SETALL */
    struct seminfo *__buf; /* buffer for IPC_INFO */
    void *__pad;
};

void reset(int sem){
    union Semun semun;
    semun.val = 0; /* initial value of the semaphore, Binary semaphore */
    if (semctl(sem, 0, SETVAL, semun) == -1)
    {
        perror("Error in semctl");
        exit(-1);
    }
}

// schedulers common

float wta[1000];
float waitSum = 0;
int n = -1;
float usefulSeconds = 0;

FILE *logptr;
void log_status(struct processData *p, char *status)
{
    logptr = fopen("scheduler.log", "a");

    int ret = fprintf(logptr, "At time %d process %d %s arr %d total %d remain %d wait %d", getClk(), p->id, status, p->arrivalTime, p->runningTime, p->remainingTime, p->wait);

    if (p->status == FINISHED)
    {
        float TA = getClk() - p->arrivalTime;
        float WTA = TA / p->runningTime;
        fprintf(logptr, " TA = %d WTA %.2f \n", (int)TA, WTA);
        n++;
        wta[n] = WTA;
        waitSum += p->wait;
    }
    else
    {
        fprintf(logptr, "\n");
    }
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
    float cpuUtil = usefulSeconds * 100 / getClk();
    fprintf(fptr, "CPU utilization = %.2f%%\n", cpuUtil);
    fprintf(fptr, "Avg WTA = %.2f\n", wtasum() / (n + 1));
    fprintf(fptr, "Avg Waiting = %.2f\n", waitSum / (n + 1));
    fprintf(fptr, "Std WTA = %.2f", stdWTA());
    fclose(fptr);
}
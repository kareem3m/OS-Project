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
void initClk()
{
    int shmid = shmget(SHKEY, 4, 0444);
    while ((int)shmid == -1)
    {
        //Make sure that the clock exists
        printf("%d: Wait! The clock not initialized yet!\n", getpid());
        sleep(1);
        shmid = shmget(SHKEY, 4, 0444);
    }
    shmaddr = (int *)shmat(shmid, (void *)0, 0);
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

    int shmid = shmget(key_id2, sizeof(struct processData), IPC_CREAT | 0666);
    shmaddr_pg = (struct processData *)shmat(shmid, (void *)0, 0);

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

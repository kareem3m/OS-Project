#include <ctype.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/sem.h>
#include <sys/shm.h>


/* arg for semctl system calls. */
union Semun
{
    int val;               /* value for SETVAL */
    struct semid_ds *buf;  /* buffer for IPC_STAT & IPC_SET */
    ushort *array;         /* array for GETALL & SETALL */
    struct seminfo *__buf; /* buffer for IPC_INFO */
    void *__pad;
};

int buffsize =3;
int *shmaddr;

void down(int sem)
{
    struct sembuf p_op;
     
    p_op.sem_num = 0;
    p_op.sem_op = -1;
    p_op.sem_flg = !IPC_NOWAIT;

    if (semop(sem, &p_op, 1) == -1)
    {
        perror("Error in down()");
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
        perror("Error in up()");
        exit(-1);
    }
}

int main(int argc, char** argv)
{ 
    key_t keyIdSemFull, keyIdSemEmpty, keyIdSemMutex, keyIdSemIndex;
    union Semun semun;

    keyIdSemFull = ftok("keyfile", 1);
    keyIdSemEmpty = ftok("keyfile", 2);
    keyIdSemMutex = ftok("keyfile", 3);
    keyIdSemIndex = ftok("keyfile", 4);
    int full = semget(keyIdSemFull, 1, 0666 | IPC_CREAT | IPC_EXCL);
    int empty = semget(keyIdSemEmpty, 1, 0666 | IPC_CREAT | IPC_EXCL);
    int mutexLock = semget(keyIdSemMutex, 1, 0666 | IPC_CREAT | IPC_EXCL);
    // the index of the buffer where the producer will put the current process 
    int index = semget(keyIdSemIndex, 1, 0666 | IPC_CREAT | IPC_EXCL);

    if(full!=-1||empty!=-1||mutexLock!=-1)
    {
        semun.val = 0; /* initial value of the semaphore, Binary semaphore */
        if (semctl(full, 0, SETVAL, semun) == -1)
        {
            perror("Error in semctl");
            exit(-1);
        }
        semun.val = 1; /* initial value of the semaphore, Binary semaphore */
        if (semctl(mutexLock, 0, SETVAL, semun) == -1)
        {
            perror("Error in semctl");
            exit(-1);
        }
        semun.val = buffsize; /* initial value of the semaphore, Binary semaphore */
        if (semctl(empty, 0, SETVAL, semun) == -1)
        {
            perror("Error in semctl");
            exit(-1);
        }
    }
    else
    {
        full = semget(keyIdSemFull, 1, 0666 | IPC_CREAT);
        empty = semget(keyIdSemEmpty, 1, 0666 | IPC_CREAT);
        mutexLock = semget(keyIdSemMutex, 1, 0666 | IPC_CREAT);
    }

    // for multi producers if the index is already created it will take its current value otherwise it will create index
    if(index != -1)
    {
        semun.val = 0; /* initial value of the semaphore, Binary semaphore */
        if (semctl(index, 0, SETVAL, semun) == -1)
        {
            perror("Error in semctl");
            exit(-1);
        } 
    }
    else
    {
        index = semget(keyIdSemIndex, 1, 0666 | IPC_CREAT);
    }

    //shared memory initialization
    int shmid;
    key_t keyIdShm;
    keyIdShm = ftok("keyfile", 6);
    shmid = shmget(keyIdShm, buffsize*sizeof(int), IPC_CREAT | 0666);
    shmaddr = (int*)shmat(shmid, (void *)0, 0);

    if (shmaddr == -1)
    {
        perror("Error in attach in reader");
        exit(-1);
    }
    int i=0;
    int add=100;
    while(1)
    {       
        down(empty);          
        down(mutexLock);
        i=semctl(index, 0, GETVAL, semun);
        up(index);

        // if the index reach buffer size it will wraparound again
        if(i==buffsize-1)
        {
            semun.val=0;
            semctl(index, 0, SETVAL, semun);
        }

        printf("%d: producer has produced an item\n",add);
        up(mutexLock);
        shmaddr[i]=add;
        add=add+1;
        up(full);
    }
    return 0;
}



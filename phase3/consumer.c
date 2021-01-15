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
struct msgbuff
{
    long mtype;
    char mtext[256];
};
int buffsize =3;


/* arg for semctl system calls. */
union Semun
{
    int val;               /* value for SETVAL */
    struct semid_ds *buf;  /* buffer for IPC_STAT & IPC_SET */
    ushort *array;         /* array for GETALL & SETALL */
    struct seminfo *__buf; /* buffer for IPC_INFO */
    void *__pad;
};
int msgq_id_down, msgq_id_up;

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
    key_t key_id_sem_full, key_id_sem_empty, key_id_sem_mutex, key_id_sem_index;
    union Semun semun;

    key_id_sem_full = ftok("keyfile", 1);
    key_id_sem_empty = ftok("keyfile", 2);
    key_id_sem_mutex = ftok("keyfile", 3);
    key_id_sem_index = ftok("keyfile", 5);
    int full = semget(key_id_sem_full, 1, 0666 | IPC_CREAT | IPC_EXCL);
    int empty = semget(key_id_sem_empty, 1, 0666 | IPC_CREAT | IPC_EXCL);
    int mutex_lock = semget(key_id_sem_mutex, 1, 0666 | IPC_CREAT | IPC_EXCL);
    int index = semget(key_id_sem_index, 1, 0666 | IPC_CREAT | IPC_EXCL);

    if(full!=-1||empty!=-1|mutex_lock!=-1){
            semun.val = 0; /* initial value of the semaphore, Binary semaphore */
            if (semctl(full, 0, SETVAL, semun) == -1)
            {
                perror("Error in semctl");
                exit(-1);
            }
            semun.val = 1; /* initial value of the semaphore, Binary semaphore */
            if (semctl(mutex_lock, 0, SETVAL, semun) == -1)
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
    else{
            full = semget(key_id_sem_full, 1, 0666 | IPC_CREAT);
            empty = semget(key_id_sem_empty, 1, 0666 | IPC_CREAT);
            mutex_lock = semget(key_id_sem_mutex, 1, 0666 | IPC_CREAT);
    }
    if(index != -1){
        semun.val = 0; /* initial value of the semaphore, Binary semaphore */
        if (semctl(index, 0, SETVAL, semun) == -1)
        {
            perror("Error in semctl");
            exit(-1);
        }
       
    }
    else
    {
        index = semget(key_id_sem_index, 1, 0666 | IPC_CREAT);
    }
    

    int shmid;
    key_t key_id_shm;
    key_id_shm = ftok("keyfile", 6);

    shmid = shmget(key_id_shm, buffsize*sizeof(int), IPC_CREAT | 0666);
    shmaddr = (int*)shmat(shmid, (void *)0, 0);
    if (shmaddr == -1)
    {
        perror("Error in attach in reader");
        exit(-1);
    }
    printf("\nReader: Shared memory attached at address %x\n", shmaddr);


    key_t key_id_msq;

    //message queue initialization
    key_id_msq = ftok("keyfile", 6);              
    int msgq_id = msgget(key_id_msq, 0666 | IPC_CREAT);
    if (msgq_id == -1)
    {
        perror("Error in create");
        exit(-1);
    }
    printf("Message Queue ID down = %d\n", msgq_id);
    
    struct msgbuff message;
    int rec_val, send_val;
    message.mtype = getpid()%10000; /* arbitrary value */
    strcpy(message.mtext,"producer it's time to wake up");

    int i=2;
    // int rem=0;
    while(1)
    {

	        down(full);  
            down(mutex_lock);
            i=semctl(index, 0, GETVAL, semun);
            up(index);
            if(i==buffsize-1){
                semun.val=0;
                semctl(index, 0, SETVAL, semun);
            }
            printf("%d: consumer has consumed an item\n",shmaddr[i]);
            // rem=(rem+1)%buffsize;
            up(mutex_lock);
            up(empty);
            sleep(7);
    }

    return 0;
}

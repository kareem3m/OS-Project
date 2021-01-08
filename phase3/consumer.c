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

int buffsize;





/////

/* arg for semctl system calls. */
union Semun
{
    int val;               /* value for SETVAL */
    struct semid_ds *buf;  /* buffer for IPC_STAT & IPC_SET */
    ushort *array;         /* array for GETALL & SETALL */
    struct seminfo *__buf; /* buffer for IPC_INFO */
    void *__pad;
};
int *shmaddr;
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
    else{
        
        //read from memory
        //up sem2
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












/////////
void handler(int signum);
int msgq_id_down, msgq_id_up;

int main(int argc, char** argv)
{


    buffsize=atoi(argv[1]);
///////////////
    int pid, shmid;
    key_t key_id2,key_id_sem2,key_id_sem1;
    key_id2 = ftok("keyfile", 66);
    shmid = shmget(key_id2, buffsize*sizeof(int), IPC_CREAT | 0666);

    shmaddr = (int*)shmat(shmid, (void *)0, 0);
    if (shmaddr == -1)
    {
        perror("Error in attach in reader");
        exit(-1);
    }
    printf("\nReader: Shared memory attached at address %x\n", shmaddr);
    union Semun semun;
    key_id_sem1 = ftok("keyfile", 1);
    key_id_sem2 = ftok("keyfile", 2);
    int sem1 = semget(key_id_sem1, 1, 0666 | IPC_CREAT);
    int sem2 = semget(key_id_sem2, 1, 0666 | IPC_CREAT);

    if (sem1 == -1 || sem2 == -1)
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
    semun.val = buffsize; /* initial value of the semaphore, Binary semaphore */
    if (semctl(sem2, 0, SETVAL, semun) == -1)
    {
        perror("Error in semctl");
        exit(-1);
    }












//////////////








    signal(SIGINT,handler);
    
    key_t key_id;
    int rec_val, send_val;

    //message queue up & down initialization
    key_id = ftok("keyfile", 65);               //create unique key
    msgq_id_up = msgget(key_id, 0666 | IPC_CREAT); //create message queue and return id
    key_id = ftok("keyfile", 77);
    msgq_id_down = msgget(key_id, 0666 | IPC_CREAT);

    if (msgq_id_down == -1 || msgq_id_up == -1)
    {
        perror("Error in create");
        exit(-1);
    }
    printf("Message Queue ID down = %d\nMessage Queue ID up = %d\n", msgq_id_down, msgq_id_up);
    
    struct msgbuff message;
    message.mtype = getpid()%10000; /* arbitrary value */
    strcpy(message.mtext,"producer it's time to wake up");
    int i;
    int add;
    while(1)
    {
           
            up(sem1);
	    int size=semctl(sem2, 0, GETVAL, semun);
	    if(size==0){
	            
	            up(sem2);
	            i=shmaddr[add];
	            add=(add+1)%buffsize;
                    printf("%d: consumer has consumed an item\n",i);
	    	    send_val = msgsnd(msgq_id_up, &message, sizeof(message.mtext), !IPC_NOWAIT);
		    if (send_val == -1)
			perror("Errror in send");
		    down(sem1);
	    }
	    else if(size==buffsize)
	    {       
	            //receive converted message from server
	            down(sem1);
		    rec_val = msgrcv(msgq_id_up, &message, sizeof(message.mtext), 0, !IPC_NOWAIT);
		    if (rec_val == -1)
		          perror("Errror in receive");
		    //else 
		          //printf("\nMessage received: %s\n", message.mtext);
            }
            else{
            	    up(sem2);
	            i=shmaddr[add];
	            add=(add+1)%buffsize;
	            printf("%d: consumer has consumed an item\n",i);
	            i++;
		    down(sem1);
                   
            }
    

    }
    return 0;
}
void handler(int signum)
{

   msgctl(msgq_id_up, IPC_RMID, (struct msqid_ds *)0);
   msgctl(msgq_id_down, IPC_RMID, (struct msqid_ds *)0);
   exit(0);
}

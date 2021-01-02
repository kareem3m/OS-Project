#include "../headers.h"
#include "../circular.h"
#include <sys/queue.h>
#include <time.h>

///////////////////////////////
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
        perror("Error in up()");
        exit(-1);
    }
}

int sem1, sem2, sem3;
struct processData *shmaddrr;
////////////////////////////

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
    //////////////////////////////////////
    int shmid;
    key_t key_id1, key_id2, key_id3, key_id4;
    key_id2 = ftok("keyfile", 66);
    key_id4 = ftok("keyfile", 69);
    shmid = shmget(key_id2, sizeof(struct processData), IPC_CREAT | 0666);
    shmaddr = (struct processData *)shmat(shmid, (void *)0, 0);
    if (shmaddr == -1)
    {
        perror("Error in attach in reader");
        exit(-1);
    }

    union Semun semun;
    key_id3 = ftok("keyfile", 67);
    key_id1 = ftok("keyfile", 68);

    sem1 = semget(key_id3, 1, 0666 | IPC_CREAT);
    sem2 = semget(key_id1, 1, 0666 | IPC_CREAT);
    sem3 = semget(key_id4, 1, 0666 | IPC_CREAT);
    if (sem1 == -1 || sem2 == -1 || sem3 == -1)
    {
        perror("Error in create sem");
        exit(-1);
    }
    /////////////////////////////////////
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
    //while(down(sem3) == -1){
        down(sem1);
        printf("%d\n", shmaddrr[0].id);
        up(sem2);
    //}
}

void sleep_a_quantum()
{
    int start = getClk();
    while (getClk() - start < quantum)
    {
        sleep(quantum - (getClk() - start));
    }
}

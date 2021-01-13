#include "headers.h"
// #include "circular.h"
#include "P_Q.h"
#include <time.h>
int sem1, sem2, sem3;

struct processData *shmaddrr;

void start_process(struct processData *process);
void check_for_new_processes();
void wait_next_clk();
void remove_process(struct processData *p);
void log_status(struct processData *p, char *status);

FILE *logptr;

struct processData* Head;
struct processData* Current_Process;
bool Finished_Processes=false;
int main(int argc, char *argv[])
    {
        key_t key_id1 = ftok("keyfile", 68);
        key_t key_id2 = ftok("keyfile", 66);
        key_t key_id3 = ftok("keyfile", 67);
        key_t key_id4 = ftok("keyfile", 69);
        key_t key_id5 = ftok("keyfile", 70);
        
        int shmid = shmget(key_id2, sizeof(struct processData), IPC_CREAT | 0666);
        shmaddrr = (struct processData *)shmat(shmid, (void *)0, 0);

        //Head=(struct processData*)malloc(sizeof(struct processData));
        //Current_Process=(struct processData*)malloc(sizeof(struct processData));
        Current_Process=New_Process(0,0,0,0);
        Head=New_Process(0,0,0,0);
        //Current_Process=NULL;
        if (shmaddrr == (void *)-1)
        {
            perror("Error in attach in reader");
            exit(-1);
        }

        sem1 = semget(key_id3, 1, 0666 | IPC_CREAT);
        sem2 = semget(key_id1, 1, 0666 | IPC_CREAT);
        sem3 = semget(key_id4, 1, 0666 | IPC_CREAT);
        if (sem1 == -1 || sem2 == -1 || sem3 == -1)
        {
            perror("Error in create sem");
            exit(-1);
        }
        initClk();
        while (1)
        {
                if(Finished_Processes==false)
                {
                check_for_new_processes();
                }
            //if (!Is_Empty(&Head))
            //{
                if (Current_Process->priority == 0 && Head->priority!=0)
                {
                    start_process(Head);
                    
                }
                if(Current_Process->status == STARTED&&Current_Process->remainingtime>0){
                    Current_Process->remainingtime--;
                    printf("remaningtime %d ",Current_Process->remainingtime);
                }
                
                else if(Current_Process->status == STARTED&&Current_Process->remainingtime==0)
                {
                    remove_process(Current_Process);
                }
                wait_next_clk();
                if(Finished_Processes==true &&Head->priority==0&&Current_Process->priority==0){
                    printf("5ls");
                    exit(0);
                }
           // }
        }

        return 0;
    }

void start_process(struct processData *pr)
{
    Current_Process->id=pr->id;
    Current_Process->arrivaltime=pr->arrivaltime;
    Current_Process->last_run=pr->last_run;
    Current_Process->priority=pr->priority;
    Current_Process->remainingtime=pr->remainingtime;
    Current_Process->responsetime=pr->responsetime;
    Current_Process->runningtime=pr->runningtime;
    printf("%d %d %d %d %d",pr->id,pr->priority,pr->remainingtime,pr->arrivaltime,pr->status);
    //Current_Process=Dequeue(&pr);
    Dequeue(&pr);
    if(Is_Empty(&Head)){
        Head=New_Process(0,0,0,0);
    }
    printf("start process %d %d %d %d %d \n",Current_Process->id,Current_Process->priority,Current_Process->remainingtime,Current_Process->arrivaltime,Current_Process->status);
    Current_Process->wait += getClk() - Current_Process->arrivaltime;
    int pid = fork();
    if (pid == 0)
    {
        char arg[10];
        sprintf(arg, "%d", Current_Process->runningtime);
        int ret = execl("process", "process", arg, NULL);
        perror("Error in execl: ");
        exit(-1);
    }
    else
    {
        Current_Process->pid = pid;
        Current_Process->status = STARTED;
        log_status(Current_Process, "started");
        //Current_Process->remainingtime--;
    }
    printf("start process status %d \n",Current_Process->status);
    printf("start process remaningtime %d \n",Current_Process->remainingtime);
}
void remove_process(struct processData *Pr)
    {   
        
        if (kill(Pr->pid, SIGSTOP) == -1)
        {
            perror("Error in kill: ");
            exit(-1);
        }
        Pr->status = FINISHED;
        Pr->last_run = getClk(); 
        log_status(Pr, "finished");
        //struct processData* temp=(struct processData*)malloc(sizeof(struct processData));
        printf("%d in delation\n",Pr->priority);
        //Dequeue(&Pr);
        //free(Current_Process);
        Current_Process=New_Process(0,0,0,0);
        //free(temp);
    }
void check_for_new_processes()
{
    while (1)
    {
        down(sem1);
        printf("HPF: downed sem1\n");
        fflush(stdout);
        struct processData *pr = (struct process *)malloc(sizeof(struct processData));
         if (shmaddrr->id == -2)
        {
            up(sem2);
            printf("HPF: upped sem2\n");
            fflush(stdout);
            Finished_Processes=true;
            return;
        }
        if (shmaddrr->id == -1)
        {
            up(sem2);
            printf("HPF: upped sem2\n");
            fflush(stdout);
            return;
        }
        if(Head->priority==0){
            Head=New_Process(shmaddrr->arrivaltime,shmaddrr->priority,shmaddrr->runningtime,shmaddrr->id);
            printf("after enqueue %d \n",Head->id);
        }
        else{
        pr=New_Process(shmaddrr->arrivaltime,shmaddrr->priority,shmaddrr->runningtime,shmaddrr->id);
        printf("before enque %d \n",pr->id);
        Enqueue(&Head,&pr);
        printf("%d in creation\n",Head->id);
        }
        up(sem2);
        printf("HPF: upped sem2\n");
        fflush(stdout);
    }
}

void wait_next_clk()
{
    int start = getClk();
    while (start == getClk())
        ;
}
void log_status(struct processData *p, char *status)
{
    logptr = fopen("scheduler.log", "a");
    int ret = fprintf(logptr, "At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), p->id, status, p->arrivaltime, p->runningtime, p->remainingtime, p->wait);
    fclose(logptr);
}
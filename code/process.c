#include "headers.h"

/* Modify this file as needed*/
int remainingtime;

int main(int agrc, char *argv[])
{
    initClk();
    //TODO it needs to get the remaining time from somewhere
    //remainingtime = ??;

    remainingtime = atoi(argv[1]);

    while (remainingtime > 0)
    {
        //printf("%d: process %d alive, remaining time = %d\n", getClk(), getpid(), remainingtime);
        int start = getClk();
        while(start == getClk());
        remainingtime--;
    }

    destroyClk(false);

    return 0;
}

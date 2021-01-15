#include "headers.h"
#include <time.h>
/* Modify this file as needed*/
int remainingtime, t;

int main(int agrc, char *argv[])
{
    initialize_ipc();
    initClk();

    remainingtime = atoi(argv[1]); // get running time argument

    while (remainingtime > 0)
    {
        t = getClk();
        while (t == getClk())
        {
        };
        remainingtime--;
        up(sem1_process);
        down(sem2_process);
    }

    destroyClk(false);

    return 0;
}

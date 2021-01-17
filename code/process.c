#include "headers.h"
#include <time.h>
/* Modify this file as needed*/
int remainingTime, t;

int main(int agrc, char *argv[])
{
    initialize_ipc();
    initClk();

    remainingTime = atoi(argv[1]); // get running time argument

    while (remainingTime > 0)
    {
        t = getClk();
        while (t == getClk())
        {
        };
        remainingTime--;
        up(sem1Process);
        down(sem2Process);
    }

    destroyClk(false);

    return 0;
}

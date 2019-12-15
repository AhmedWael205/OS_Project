#include "headers.h"

/* Modify this file as needed*/
int remainingtime;

int main(int agrc, char * argv[])
{
    initClk();
    
    //TODO it needs to get the remaining time from somewhere
    //remainingtime = ??;

    remainingtime = atoi (argv[0]);
    //printf("I am a process with remaining time = %d\n",remainingtime);
    int x = getClk();
    while (remainingtime > 0)
    {
        if(getClk()-x >= 1)
        {
            x = getClk();
            printf("Remaining Time = %d\n", remainingtime--);
        }
    }

    // printf("Process Terminating and sending SIGUSR1 to parent : %d\n",getppid());
    kill(getppid(),SIGUSR1);
    raise(SIGKILL);
    //destroyClk(false);
    
    return 0;
}

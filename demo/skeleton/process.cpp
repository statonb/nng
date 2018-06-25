#include "process.h"

Process::Process(void)
{

}

pid_t Process::Launch(void)
{
    pid_t childPid;

    switch (childPid = fork())
    {
        case -1:
            fprintf(stderr, "fork\n");

        case 0:
            printf("Launched pid=%ld\n",(long int)(getpid()));
            myPid = getpid();
            Run();
            printf("%ld terminated\n", (long int)(getpid()));
            break;

        default:
            break;
    }

    return childPid;
}


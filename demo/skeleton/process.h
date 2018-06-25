#ifndef __PROCESS_H__
#define __PROCESS_H_

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h> // defines NULL
#include <semaphore.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <vector>
#include <pthread.h>

class Process
{
public:
    Process(void);
    pid_t Launch(void);
    virtual void Run(void) {};
    pid_t myPid;
};

#endif

#include <iostream>
#include <string.h>
#include <getopt.h>
#include "process.h"
#include "urls.h"
#include <nng/nng.h>
#include <nng/protocol/pipeline0/pull.h>
#include <nng/protocol/pipeline0/push.h>

using namespace std;

unsigned int DelayTime = 0;

typedef struct
{
    char        textMsg[80];
    uint32_t    n;
}   ProcMsg_t;


void
fatal(const char *func, int rv)
{
    fprintf(stderr, "%s: %s\n", func, nng_strerror(rv));
    exit(1);
}

class ProcessA : public Process
{
public:
    ProcessA(void);
    void Run(void);
private:
    ProcMsg_t mMsg;
};

class ProcessB : public Process
{
public:
    ProcessB(void) {};
    void Run(void);
};

class ProcessC : public Process
{
public:
    ProcessC(void) {};
    void Run(void);
};

ProcessA::ProcessA(void)
    : Process()
{
    strcpy(mMsg.textMsg, "Msg from ProcA");
    mMsg.n = 0;
}

void ProcessA::Run(void)
{
    // int sz_msg = strlen(mMsg) + 1; // '\0' too
    nng_socket sock;
    int rv;
    // int bytes;

    if ((rv = nng_push0_open(&sock)) != 0)
    {
        fatal("nng_push0_open", rv);
    }
    if ((rv = nng_dial(sock, ProcB_URL, NULL, 0)) != 0)
    {
        fatal("nng_dial", rv);
    }
    for(;;)
    {
        mMsg.n++;
        cout << "ProcA(" << myPid << "): SENDING \"" << mMsg.textMsg << "\" : n=" << mMsg.n << endl;
        if ((rv = nng_send(sock, &mMsg, sizeof(mMsg), 0)) != 0)
        {
            fatal("nng_send", rv);
        }
        sleep(1);
    }
    nng_close(sock);
}

void ProcessB::Run(void)
{
    nng_socket rxSock, txSock;
    int rv;

    if ((rv = nng_pull0_open(&rxSock)) != 0)
    {
        fatal("nng_pull0_open", rv);
    }
    if ((rv = nng_listen(rxSock, ProcB_URL, NULL, 0)) != 0)
    {
        fatal("nng_listen", rv);
    }

    if ((rv = nng_push0_open(&txSock)) != 0)
    {
        fatal("nng_push0_open", rv);
    }
    if ((rv = nng_dial(txSock, ProcC_URL, NULL, 0)) != 0)
    {
        fatal("nng_dial", rv);
    }

    while (DelayTime)
    {
        cout << "ProcB(" << myPid << ") delaying " << DelayTime << " seconds before processing RX msgs" << endl;
        DelayTime--;
        sleep(1);
    }

    for (;;)
    {
        ProcMsg_t *buf = NULL;
        size_t sz;
        if ((rv = nng_recv(rxSock, &buf, &sz, NNG_FLAG_ALLOC)) != 0)
        {
            fatal("nng_recv", rv);
        }
        cout << "ProcB(" << myPid << "): RECEIVED \"" << buf->textMsg << "\" n=" << buf->n << " incrementing and sending to ProcC" << endl;
        buf->n = buf->n + 100;
        if ((rv = nng_send(txSock, buf, sz, 0)) != 0)
        {
            fatal("nng_send", rv);
        }
        nng_free(buf, sz);
    }
}

void ProcessC::Run(void)
{
    nng_socket sock;
    int rv;

    if ((rv = nng_pull0_open(&sock)) != 0)
    {
        fatal("nng_pull0_open", rv);
    }
    if ((rv = nng_listen(sock, ProcC_URL, NULL, 0)) != 0)
    {
        fatal("nng_listen", rv);
    }

    while (DelayTime)
    {
        cout << "ProcC(" << myPid << ") delaying " << DelayTime << " seconds before processing RX msgs" << endl;
        DelayTime--;
        sleep(1);
    }

    for (;;)
    {
        ProcMsg_t *buf = NULL;
        size_t sz;
        if ((rv = nng_recv(sock, &buf, &sz, NNG_FLAG_ALLOC)) != 0)
        {
            fatal("nng_recv", rv);
        }
        cout << "ProcC(" << myPid << "): RECEIVED \"" << buf->textMsg << "\" n=" << buf->n << endl;
        nng_free(buf, sz);
    }
}

int main(int argc, char *argv[])
{
    ProcessA *procA = new ProcessA;
    ProcessB *procB = new ProcessB;
    ProcessC *procC = new ProcessC;
    bool launchAFlag = false;
    bool launchBFlag = false;
    bool launchCFlag = false;
    bool launchAnyFlag = false;
    // bool usageError = false;
    int opt;

    while ((opt = getopt(argc, argv, "abcd:")) != -1)
    {
        switch (opt)
        {
        case 'a':
            launchAFlag = true;
            launchAnyFlag = true;
            break;
        case 'b':
            launchBFlag = true;
            launchAnyFlag = true;
            break;
        case 'c':
            launchCFlag = true;
            launchAnyFlag = true;
            break;
        case 'd':
            DelayTime = (unsigned int)(strtoul(optarg, NULL, 10));
            break;
        default:
            // usageError = true;
            break;
        }
    }

    if (!launchAnyFlag) return -1;
    cout << "Hello world!" << endl;
    if (launchCFlag)
    {
        procC->Launch();
        sleep(2);
    }
    if (launchBFlag)
    {
        procB->Launch();
        sleep(2);
    }
    if (launchAFlag)
    {
        procA->Launch();
    }

    return 0;
}

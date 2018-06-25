#ifndef __URLS_H__
#define __URLS_H__

#define __URL_IPC   (1)

#if (__URL_TCP)
const char *ProcB_URL = "tcp://192.168.106.150:9001";
const char *ProcC_URL = "tcp://192.168.106.149:9002";

#elif (__URL_IPC)
const char *ProcB_URL = "ipc:///tmp/skeketon_procB";
const char *ProcC_URL = "ipc:///tmp/skeketon_procC";

#else
#error "Must define __URL_TCP or __URL_IPC to be 1"
#endif

#endif

/* ipimbIocMain.cpp */
/* Author:  Marty Kraimer Date:    17MAR2000 */

#include <stddef.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <signal.h>

#include "epicsExit.h"
#include "epicsThread.h"
#include "iocsh.h"

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *start_main(void *arg)
{
    char *name = (char *) arg;
    sigset_t sigs;

    /* Wait for the main process to set itself up. */
    sigemptyset(&sigs);
    sigaddset(&sigs, SIGIO);
    pthread_sigmask(SIG_BLOCK, &sigs, NULL);
    pthread_mutex_lock(&mutex);

    if (name) {    
        iocsh(name);
        epicsThreadSleep(0.2);
    }
    iocsh(NULL);
    epicsExit(0);
    return(0);
}

int main(int argc,char *argv[])
{
    pthread_t tid;
    int status;
    struct sched_param param;

    pthread_mutex_lock(&mutex);
    status = pthread_create(&tid, NULL, start_main, argc >= 2 ? argv[1] : NULL);

    /* Raise main thread priority for interrupt handling! */
    param.sched_priority = 99;
    status = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);

    /* Release the child thread. */
    pthread_mutex_unlock(&mutex);

    /* Wait for thread to die. */
    pthread_join(tid, NULL);
}

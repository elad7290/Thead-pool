//elad baal-tzdaka 312531973
#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include "osqueue.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct thread_pool {
    OSQueue *tasks;
    pthread_t * threads;
    int numOfThreads;
    pthread_mutex_t look;
    pthread_cond_t notify;
    int flag;
    int destroy;

} ThreadPool;

typedef struct data {
    void (*computeFunc)(void *);
    void *param;
} Data;

ThreadPool *tpCreate(int numOfThreads);

void tpDestroy(ThreadPool *threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool *threadPool, void (*computeFunc)(void *), void *param);

void* execute(void*);

#endif

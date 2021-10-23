//elad baal-tzdaka 312531973
#include "threadPool.h"


ThreadPool *tpCreate(int numOfThreads) {
    ThreadPool *threadPool = (ThreadPool *) malloc(sizeof(ThreadPool));
    if (threadPool == NULL) {
        perror("Error in system call");
        exit(-1);
    }
    //init array of thread
    threadPool->threads = (pthread_t *) malloc(sizeof(pthread_t) * numOfThreads);
    if (threadPool->threads == NULL) {
        free(threadPool);
        perror("Error in system call");
        exit(-1);
    }
    //init mutex
    if (pthread_mutex_init(&threadPool->look, NULL) != 0) {
        free(threadPool->threads);
        free(threadPool);
        perror("Error in system call");
        exit(-1);
    }
    //init Queue
    if ((threadPool->tasks = osCreateQueue()) == NULL) {
        pthread_mutex_destroy(&threadPool->look);
        free(threadPool->threads);
        free(threadPool);
        perror("Error in system call");
        exit(-1);
    }
    //init notify
    if (pthread_cond_init(&threadPool->notify, NULL) != 0) {
        osDestroyQueue(threadPool->tasks);
        pthread_mutex_destroy(&threadPool->look);
        free(threadPool->threads);
        free(threadPool);
        perror("Error in system call");
        exit(-1);
    }
    threadPool->numOfThreads = numOfThreads;
    threadPool->flag = 0;
    threadPool->destroy=0;
    //
    int i,j;
    for ( i = 0; i < numOfThreads; i++) {
        if (pthread_create(&threadPool->threads[i], NULL, execute, threadPool) != 0) {
            for ( j = 0; j < i; j++) {
                pthread_cancel(threadPool->threads[j]);
            }
            osDestroyQueue(threadPool->tasks);
            pthread_mutex_destroy(&threadPool->look);
            free(threadPool->threads);
            free(threadPool);
            perror("Error in system call");
            exit(-1);
        }
    }
    return threadPool;
}


void *execute(void *param) {
    ThreadPool *threadPool = (ThreadPool *) param;
    while (!threadPool->flag) {
        pthread_mutex_lock(&threadPool->look);
        if(osIsQueueEmpty(threadPool->tasks)&&(threadPool->destroy ==1)){
            pthread_mutex_unlock(&threadPool->look);
            break;
        }
        if (osIsQueueEmpty(threadPool->tasks)){
            //check if wait success if not we free all allocated memory.
            if(pthread_cond_wait(&threadPool->notify,&threadPool->look)!=0){
                pthread_mutex_unlock(&threadPool->look);
                int j;
                for ( j = 0; j < threadPool->numOfThreads; j++) {

                    pthread_cancel(threadPool->threads[j]);
                }
                osDestroyQueue(threadPool->tasks);
                pthread_mutex_destroy(&threadPool->look);
                free(threadPool->threads);
                free(threadPool);
                perror("Error in system call");
                exit(-1);
            }
            pthread_mutex_unlock(&threadPool->look);
            continue;
        }
        //Activation of the action we received
        Data *data=osDequeue(threadPool->tasks);
        pthread_mutex_unlock(&threadPool->look);
        data->computeFunc(data->param);
    }

}

/**
 * if shouldWaitForTasks==0 We wait for all the tasks to be completed,
 * both those in queue and those currently running.
 * And  we do not add any more new tasks.
 * else we just waiting only for the tasks that currently run to be complete.
 * */
void tpDestroy(ThreadPool *threadPool, int shouldWaitForTasks) {
    //
    threadPool->destroy = 1;
    if (shouldWaitForTasks == 0) {
        threadPool->flag=1;
    }
    int i;
    for ( i = 0; i <threadPool->numOfThreads ; ++i) {
        if(pthread_cond_broadcast(&threadPool->notify))
        {
            int j;
            for ( j = 0; j < threadPool->numOfThreads; j++) {

                pthread_cancel(threadPool->threads[j]);
            }
            osDestroyQueue(threadPool->tasks);
            free(threadPool->threads);
            free(threadPool);
            perror("Error in system call");
            exit(-1);
        }
        //check if pthread_join successes
        if(pthread_join(threadPool->threads[i],NULL)){
            int j;
            for ( j = 0; j < threadPool->numOfThreads; j++) {
                pthread_cancel(threadPool->threads[j]);
            }
            osDestroyQueue(threadPool->tasks);
            pthread_mutex_destroy(&threadPool->look);
            free(threadPool->threads);
            free(threadPool);
            perror("Error in system call");
            exit(-1);
        }
    }

    osDestroyQueue(threadPool->tasks);
    pthread_mutex_destroy(&threadPool->look);
    free(threadPool->threads);
    free(threadPool);
}


int tpInsertTask(ThreadPool *threadPool, void (*computeFunc)(void *), void *param){
    pthread_mutex_lock(&threadPool->look);
    if (threadPool->destroy==1){
        pthread_mutex_unlock(&threadPool->look);
        return -1;
    }
    Data *data=(Data*) malloc(sizeof (Data));
    if(data==NULL){
        int j;
        for ( j = 0; j < threadPool->numOfThreads; j++) {
            pthread_cancel(threadPool->threads[j]);
        }
        osDestroyQueue(threadPool->tasks);
        pthread_mutex_destroy(&threadPool->look);
        free(threadPool->threads);
        free(threadPool);
        perror("Error in system call");
        exit(-1);
    }
    data->computeFunc=computeFunc;
    data->param=param;
    osEnqueue(threadPool->tasks,(void*)data);
    //check if signal successes
    if(pthread_cond_signal(&threadPool->notify))
    {
        pthread_mutex_unlock(&threadPool->look);
        int j;
        for ( j = 0; j < threadPool->numOfThreads; j++) {

            pthread_cancel(threadPool->threads[j]);
        }
        osDestroyQueue(threadPool->tasks);
        free(threadPool->threads);
        free(threadPool);
        perror("Error in system call");
        exit(-1);
    }
    pthread_mutex_unlock(&threadPool->look);
    return 0;
}





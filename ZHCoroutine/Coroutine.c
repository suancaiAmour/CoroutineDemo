
//  Coroutine.c
//  协程 Demo
//
//  Created by 张鸿 on 2017/11/28.
//  Copyright © 2017年 酸菜Amour. All rights reserved.
//

#include "Coroutine.h"
#include <stdlib.h>
#include <pthread.h>
#include <memory.h>
#include <setjmp.h>

typedef struct CoroutineUnit
{
    int *regEnv; // 存储寄存器内容的缓冲区 大小为 48
    void *stack; // 栈
    long stackSize; // 栈的大小
    int isSwitch:1, isRelease:1; // 标志位 切换标志 释放任务标志
    struct CoroutineUnit *next;
}*pCoroutineUnit, coroutineUnit;

typedef struct ThreadKey
{
    pthread_key_t coroutineKey;
    pthread_key_t spKey;
    pthread_key_t fpKey;
    pthread_key_t emptyUnitKey;
    pthread_key_t endBufKey;
}*pThreadKey, threadKey;

typedef struct ThreadInfo
{
    pThreadKey *keyArray;
    pthread_t  *threadArray;
    long length;
}*pThreadInfo, threadInfo;

extern void pushCoroutineEnv(int *regEnv);
extern void popCoroutineEnv(int *regEnv);
extern void *getSP(void);
extern void *getFP(void);
extern void blAddress(void *address);

static threadInfo threadMap;
pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

#define ENDFLAG 2

static pThreadKey getThreadKey(void)
{
    pthread_rwlock_rdlock(&rwlock);
    for (unsigned long threadId = 0; threadId < threadMap.length; threadId++) {
        if (threadMap.threadArray[threadId] == pthread_self()) {
            pthread_rwlock_unlock(&rwlock);
            return threadMap.keyArray[threadId];
        }
    }
    pthread_rwlock_unlock(&rwlock);
    return NULL;
}

void emptyTask(void)
{
    int array[5000];
    while (1) {
        printf("待机任务! %p\r\n", array);
        pThreadKey key = getThreadKey();
        pCoroutineUnit pTaskUnit = (pCoroutineUnit)pthread_getspecific(key->coroutineKey);
        if (pTaskUnit->next == pTaskUnit) {
            pCoroutineUnit __pEmptyUnit = (pCoroutineUnit)pthread_getspecific(key->emptyUnitKey);
            if ((*__pEmptyUnit).regEnv) {
                free((*__pEmptyUnit).regEnv);
                (*__pEmptyUnit).regEnv = NULL;
            }
            if ((*__pEmptyUnit).stack) {
                free((*__pEmptyUnit).stack);
                (*__pEmptyUnit).stack = NULL;
            }
            free(__pEmptyUnit);
            pthread_key_delete(key->coroutineKey);
            pthread_key_delete(key->spKey);
            pthread_key_delete(key->fpKey);
            pthread_key_delete(key->emptyUnitKey);
            jmp_buf *endBuf = pthread_getspecific(key->endBufKey);
            pthread_key_delete(key->endBufKey);
            longjmp(*endBuf, ENDFLAG);
        }
        coroutine_switch();
    }
}

// 创建协程任务
void coroutine_create(coroutineTask task)
{
    pThreadKey key = getThreadKey();
    
    void *__fp = pthread_getspecific(key->fpKey);
    void *__sp = pthread_getspecific(key->spKey);
    
    pCoroutineUnit pTaskUnit = (pCoroutineUnit)calloc(1, sizeof(coroutineUnit));
    // 初始化任务的 CPU 寄存器
    
    (*pTaskUnit).regEnv = (int *)calloc(48, sizeof(int));
    (*pTaskUnit).regEnv[24] = (int)(long)__fp;
    (*pTaskUnit).regEnv[25] = (int)(long)((long)__fp >> 32);
    (*pTaskUnit).regEnv[22] = (int)(long)task;
    (*pTaskUnit).regEnv[23] = (int)(long)((long)task >> 32);
    (*pTaskUnit).regEnv[26] = (int)(long)__sp;
    (*pTaskUnit).regEnv[27] = (int)(long)((long)__sp >> 32);
    
    pCoroutineUnit __pEmptyUnit = (pCoroutineUnit)pthread_getspecific(key->emptyUnitKey);
    (*pTaskUnit).next = __pEmptyUnit->next;
    __pEmptyUnit->next = pTaskUnit;
}

static pCoroutineUnit findPreCoroutineUnit(pCoroutineUnit currentUnit)
{
    pThreadKey key = getThreadKey();
    pCoroutineUnit preUnit = (pCoroutineUnit)pthread_getspecific(key->emptyUnitKey);
    while (1) {
        if (preUnit->next == currentUnit) {
            return preUnit;
        } else {
            preUnit = preUnit->next;
        }
    }
}

// 释放当前协程任务
void coroutine_release(void)
{
    pThreadKey key = getThreadKey();
    pCoroutineUnit pTaskUnit = (pCoroutineUnit)pthread_getspecific(key->coroutineKey);
    pTaskUnit->isRelease = 1;
    coroutine_switch();
}

static inline void memcpy2stack(void *dest, void *src, long count)
{
    static char *tmp = NULL;
    static char *s = NULL;
    static long repeatCount = 0;
    tmp = dest;
    s   = src;
    repeatCount = count;
    
    while (repeatCount--) {
        *tmp++ = *s++;
    }
}

// 从当前任务切换到下一任务
void coroutine_switch(void)
{
    // 当前任务栈的大小
    pThreadKey key = getThreadKey();
    void *__sp = pthread_getspecific(key->spKey);
    long stackSize = (long)__sp - (long)getSP();
    
    pCoroutineUnit pTaskUnit = (pCoroutineUnit)pthread_getspecific(key->coroutineKey);
    (*pTaskUnit).stackSize = stackSize;
    
    if (pTaskUnit->stack) {
        free(pTaskUnit->stack);
        pTaskUnit->stack = NULL;
    }
    
    (*pTaskUnit).stack = calloc(1, stackSize);
    memcpy((*pTaskUnit).stack, getSP(), (*pTaskUnit).stackSize);
    pushCoroutineEnv((*pTaskUnit).regEnv);
    
    pTaskUnit = (pCoroutineUnit)pthread_getspecific(key->coroutineKey);
    if ((*pTaskUnit).isSwitch) {
        (*pTaskUnit).isSwitch = 0; // 取消任务的切换状态
        return;
    }
    
    pCoroutineUnit pNextTaskUnit = (*pTaskUnit).next;
    while (pNextTaskUnit->isRelease) {
        pCoroutineUnit preUnit = findPreCoroutineUnit(pNextTaskUnit);
        preUnit->next = pNextTaskUnit->next;
        
        if ((*pNextTaskUnit).regEnv) {
            free((*pNextTaskUnit).regEnv);
            (*pNextTaskUnit).regEnv = NULL;
        }
        if ((*pNextTaskUnit).stack) {
            free((*pNextTaskUnit).stack);
            (*pNextTaskUnit).stack = NULL;
        }
        free(pNextTaskUnit);
        pNextTaskUnit = preUnit->next;
    }
    
    pthread_setspecific(key->coroutineKey, pNextTaskUnit);
    if ((*pNextTaskUnit).stack) {
        (*pNextTaskUnit).isSwitch = 1; //下一个是切换过来
        pthread_mutex_lock(&mutex);
        memcpy2stack((void *)((long)__sp - (*pNextTaskUnit).stackSize),(*pNextTaskUnit).stack, (*pNextTaskUnit).stackSize);
        pthread_mutex_unlock(&mutex);
        key = getThreadKey();
        pNextTaskUnit = (pCoroutineUnit)pthread_getspecific(key->coroutineKey);
    }
    popCoroutineEnv((*pNextTaskUnit).regEnv);
}

pthread_once_t once_control = PTHREAD_ONCE_INIT;

static void initLock(void)
{
    pthread_rwlock_init(&rwlock, NULL);
    pthread_mutex_init(&mutex,NULL);
}

void coroutine_start(coroutineTask entryTask)
{
    pthread_once(&once_control, initLock);
    
    pThreadKey key = getThreadKey();
    
    if (!key) {
        key = (pThreadKey)calloc(1, sizeof(threadKey));
        pthread_key_create(&key->spKey, NULL);
        pthread_key_create(&key->fpKey, NULL);
        pthread_key_create(&key->emptyUnitKey, NULL);
        
        void *__sp = getSP(); // 栈底寄存器
        void *__fp = getFP(); // 栈帧寄存器
        pthread_setspecific(key->spKey, __sp);
        pthread_setspecific(key->fpKey, __fp);
        
        pCoroutineUnit __pEmptyUnit = (pCoroutineUnit)calloc(1, sizeof(coroutineUnit));
        // 初始化任务的 CPU 寄存器
        (*__pEmptyUnit).regEnv = (int *)calloc(48, sizeof(int));
        (*__pEmptyUnit).regEnv[24] = (int)(long)__fp;
        (*__pEmptyUnit).regEnv[25] = (int)(long)((long)__fp >> 32);
        (*__pEmptyUnit).regEnv[22] = (int)(long)emptyTask;
        (*__pEmptyUnit).regEnv[23] = (int)(long)((long)emptyTask >> 32);
        (*__pEmptyUnit).regEnv[26] = (int)(long)__sp;
        (*__pEmptyUnit).regEnv[27] = (int)(long)((long)__sp >> 32);
        (*__pEmptyUnit).next = __pEmptyUnit;
        pthread_setspecific(key->emptyUnitKey, __pEmptyUnit);
        
        pthread_rwlock_wrlock(&rwlock);
        long length = threadMap.length++;
        pThreadKey *keyArray = (pThreadKey *)calloc(threadMap.length, sizeof(pThreadKey));
        pthread_t  *threadArray = (pthread_t *)calloc(threadMap.length, sizeof(pthread_t));
        memcpy(keyArray, threadMap.keyArray, length * sizeof(pThreadKey));
        memcpy(threadArray, threadMap.threadArray, length * sizeof(pthread_t));
        keyArray[length] = key;
        threadArray[length] = pthread_self();
        free(threadMap.keyArray);
        free(threadMap.threadArray);
        threadMap.keyArray = keyArray;
        threadMap.threadArray = threadArray;
        pthread_rwlock_unlock(&rwlock);
    }
    
    coroutine_create(entryTask);
    
    pthread_key_create(&key->coroutineKey, NULL);
    pthread_key_create(&key->endBufKey, NULL);
    pCoroutineUnit __pEmptyUnit = (pCoroutineUnit)pthread_getspecific(key->emptyUnitKey);
    pthread_setspecific(key->coroutineKey, __pEmptyUnit);
    jmp_buf *endBuf = calloc(1, sizeof(jmp_buf));
    pthread_setspecific(key->endBufKey, endBuf);
    int endFlag = setjmp(*endBuf);
    if (endFlag != ENDFLAG) {
        popCoroutineEnv(__pEmptyUnit->regEnv);
    }
    free(endBuf);
}

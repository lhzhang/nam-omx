/*
 *
 * Copyright 2010 Samsung Electronics S.LSI Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * @file        NAM_OSAL_Thread.c
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version     1.0
 * @history
 *   2010.7.15 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>

#include "NAM_OSAL_Memory.h"
#include "NAM_OSAL_Thread.h"

#undef NAM_LOG_TAG
#define NAM_LOG_TAG    "NAM_LOG_THREAD"
#define NAM_LOG_OFF
#include "NAM_OSAL_Log.h"


typedef struct _NAM_THREAD_HANDLE_TYPE
{
    pthread_t          pthread;
    pthread_attr_t     attr;
    struct sched_param schedparam;
    int                stack_size;
} NAM_THREAD_HANDLE_TYPE;


OMX_ERRORTYPE NAM_OSAL_ThreadCreate(OMX_HANDLETYPE *threadHandle, OMX_PTR function_name, OMX_PTR argument)
{
    FunctionIn();

    int result = 0;
    int detach_ret = 0;
    NAM_THREAD_HANDLE_TYPE *thread;
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    thread = NAM_OSAL_Malloc(sizeof(NAM_THREAD_HANDLE_TYPE));
    NAM_OSAL_Memset(thread, 0, sizeof(NAM_THREAD_HANDLE_TYPE));

    pthread_attr_init(&thread->attr);
    if (thread->stack_size != 0)
        pthread_attr_setstacksize(&thread->attr, thread->stack_size);

    /* set priority */
    if (thread->schedparam.sched_priority != 0)
        pthread_attr_setschedparam(&thread->attr, &thread->schedparam);

    detach_ret = pthread_attr_setdetachstate(&thread->attr, PTHREAD_CREATE_JOINABLE);
    if (detach_ret != 0) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

    result = pthread_create(&thread->pthread, &thread->attr, function_name, (void *)argument);
    /* pthread_setschedparam(thread->pthread, SCHED_RR, &thread->schedparam); */

    switch (result) {
    case 0:
        *threadHandle = (OMX_HANDLETYPE)thread;
        ret = OMX_ErrorNone;
        break;
    case EAGAIN:
        *threadHandle = NULL;
        ret = OMX_ErrorInsufficientResources;
        break;
    default:
        *threadHandle = NULL;
        ret = OMX_ErrorUndefined;
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_OSAL_ThreadTerminate(OMX_HANDLETYPE threadHandle)
{
    FunctionIn();

    OMX_ERRORTYPE ret = OMX_ErrorNone;
    NAM_THREAD_HANDLE_TYPE *thread = (NAM_THREAD_HANDLE_TYPE *)threadHandle;

    if (!thread) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pthread_join(thread->pthread, NULL) != 0) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

    NAM_OSAL_Free(thread);
    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;

}

OMX_ERRORTYPE NAM_OSAL_ThreadCancel(OMX_HANDLETYPE threadHandle)
{
    NAM_THREAD_HANDLE_TYPE *thread = (NAM_THREAD_HANDLE_TYPE *)threadHandle;

    if (!thread)
        return OMX_ErrorBadParameter;

    /* thread_cancel(thread->pthread); */
    pthread_exit(thread->pthread);
    pthread_join(thread->pthread, NULL);

    NAM_OSAL_Free(thread);
    return OMX_ErrorNone;
}

void NAM_OSAL_ThreadExit(void *value_ptr)
{
    pthread_exit(value_ptr);
    return;
}

void NAM_OSAL_SleepMillinam(OMX_U32 ms)
{
    usleep(ms * 1000);
    return;
}

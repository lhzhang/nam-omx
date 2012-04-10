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
 * @file        NAM_OSAL_Queue.c
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version     1.0
 * @history
 *   2010.7.15 : Create
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "NAM_OSAL_Memory.h"
#include "NAM_OSAL_Mutex.h"
#include "NAM_OSAL_Queue.h"


OMX_ERRORTYPE NAM_OSAL_QueueCreate(NAM_QUEUE *queueHandle)
{
    int i = 0;
    NAM_QElem *newqelem = NULL;
    NAM_QElem *currentqelem = NULL;
    NAM_QUEUE *queue = (NAM_QUEUE *)queueHandle;

    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if (!queue)
        return OMX_ErrorBadParameter;

    ret = NAM_OSAL_MutexCreate(&queue->qMutex);
    if (ret != OMX_ErrorNone)
        return ret;

    queue->first = (NAM_QElem *)NAM_OSAL_Malloc(sizeof(NAM_QElem));
    if (queue->first == NULL)
        return OMX_ErrorInsufficientResources;

    NAM_OSAL_Memset(queue->first, 0, sizeof(NAM_QElem));
    currentqelem = queue->last = queue->first;
    queue->numElem = 0;

    for (i = 0; i < (MAX_QUEUE_ELEMENTS - 2); i++) {
        newqelem = (NAM_QElem *)NAM_OSAL_Malloc(sizeof(NAM_QElem));
        if (newqelem == NULL) {
            while (queue->first != NULL) {
                currentqelem = queue->first->qNext;
                NAM_OSAL_Free((OMX_PTR)queue->first);
                queue->first = currentqelem;
            }
            return OMX_ErrorInsufficientResources;
        } else {
            NAM_OSAL_Memset(newqelem, 0, sizeof(NAM_QElem));
            currentqelem->qNext = newqelem;
            currentqelem = newqelem;
        }
    }

    currentqelem->qNext = queue->first;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE NAM_OSAL_QueueTerminate(NAM_QUEUE *queueHandle)
{
    int i = 0;
    NAM_QElem *currentqelem = NULL;
    NAM_QUEUE *queue = (NAM_QUEUE *)queueHandle;
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if (!queue)
        return OMX_ErrorBadParameter;

    for ( i = 0; i < (MAX_QUEUE_ELEMENTS - 2); i++) {
        currentqelem = queue->first->qNext;
        NAM_OSAL_Free(queue->first);
        queue->first = currentqelem;
    }

    if(queue->first) {
        NAM_OSAL_Free(queue->first);
        queue->first = NULL;
    }

    ret = NAM_OSAL_MutexTerminate(queue->qMutex);

    return ret;
}

int NAM_OSAL_Queue(NAM_QUEUE *queueHandle, void *data)
{
    NAM_QUEUE *queue = (NAM_QUEUE *)queueHandle;
    if (queue == NULL)
        return -1;

    NAM_OSAL_MutexLock(queue->qMutex);

    if ((queue->last->data != NULL) || (queue->numElem >= MAX_QUEUE_ELEMENTS)) {
        NAM_OSAL_MutexUnlock(queue->qMutex);
        return -1;
    }
    queue->last->data = data;
    queue->last = queue->last->qNext;
    queue->numElem++;

    NAM_OSAL_MutexUnlock(queue->qMutex);
    return 0;
}

void *NAM_OSAL_Dequeue(NAM_QUEUE *queueHandle)
{
    void *data = NULL;
    NAM_QUEUE *queue = (NAM_QUEUE *)queueHandle;
    if (queue == NULL)
        return NULL;

    NAM_OSAL_MutexLock(queue->qMutex);

    if ((queue->first->data == NULL) || (queue->numElem <= 0)) {
        NAM_OSAL_MutexUnlock(queue->qMutex);
        return NULL;
    }
    data = queue->first->data;
    queue->first->data = NULL;
    queue->first = queue->first->qNext;
    queue->numElem--;

    NAM_OSAL_MutexUnlock(queue->qMutex);
    return data;
}

int NAM_OSAL_GetElemNum(NAM_QUEUE *queueHandle)
{
    int ElemNum = 0;
    NAM_QUEUE *queue = (NAM_QUEUE *)queueHandle;
    if (queue == NULL)
        return -1;

    NAM_OSAL_MutexLock(queue->qMutex);
    ElemNum = queue->numElem;
    NAM_OSAL_MutexUnlock(queue->qMutex);
    return ElemNum;
}

int NAM_OSAL_SetElemNum(NAM_QUEUE *queueHandle, int ElemNum)
{
    NAM_QUEUE *queue = (NAM_QUEUE *)queueHandle;
    if (queue == NULL)
        return -1;

    NAM_OSAL_MutexLock(queue->qMutex);
    queue->numElem = ElemNum; 
    NAM_OSAL_MutexUnlock(queue->qMutex);
    return ElemNum;
}


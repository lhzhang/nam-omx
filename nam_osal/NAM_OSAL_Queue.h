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
 * @file    NAM_OSAL_Queue.h
 * @brief
 * @author    SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version    1.0
 * @history
 *   2010.7.15 : Create
 */

#ifndef NAM_OSAL_QUEUE
#define NAM_OSAL_QUEUE

#include "OMX_Types.h"
#include "OMX_Core.h"


#define MAX_QUEUE_ELEMENTS    10

typedef struct _NAM_QElem
{
    void              *data;
    struct _NAM_QElem *qNext;
} NAM_QElem;

typedef struct _NAM_QUEUE
{
    NAM_QElem     *first;
    NAM_QElem     *last;
    int            numElem;
    OMX_HANDLETYPE qMutex;
} NAM_QUEUE;


#ifdef __cplusplus
extern "C" {
#endif

OMX_ERRORTYPE NAM_OSAL_QueueCreate(NAM_QUEUE *queueHandle);
OMX_ERRORTYPE NAM_OSAL_QueueTerminate(NAM_QUEUE *queueHandle);
int           NAM_OSAL_Queue(NAM_QUEUE *queueHandle, void *data);
void         *NAM_OSAL_Dequeue(NAM_QUEUE *queueHandle);
int           NAM_OSAL_GetElemNum(NAM_QUEUE *queueHandle);
int           NAM_OSAL_SetElemNum(NAM_QUEUE *queueHandle, int ElemNum);

#ifdef __cplusplus
}
#endif

#endif

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
 * @file       NAM_OMX_Baseport.c
 * @brief
 * @author     SeungBeom Kim (sbcrux.kim@samsung.com)
 *             HyeYeon Chung (hyeon.chung@samsung.com)
 * @version    1.0
 * @history
 *    2010.7.15 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "NAM_OMX_Macros.h"
#include "NAM_OSAL_Event.h"
#include "NAM_OSAL_Semaphore.h"
#include "NAM_OSAL_Mutex.h"

#include "NAM_OMX_Baseport.h"
#include "NAM_OMX_Basecomponent.h"

#undef  NAM_LOG_TAG
#define NAM_LOG_TAG    "NAM_BASE_PORT"
//#define NAM_LOG_OFF
#define NAM_TRACE_ON
#include "NAM_OSAL_Log.h"


OMX_ERRORTYPE NAM_OMX_FlushPort(OMX_COMPONENTTYPE *pOMXComponent, OMX_S32 portIndex)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    NAM_OMX_BASECOMPONENT *pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    NAM_OMX_BASEPORT      *pNAMPort = NULL;
    OMX_BUFFERHEADERTYPE  *bufferHeader = NULL;
    NAM_OMX_MESSAGE       *message = NULL;
    OMX_U32                flushNum = 0;
    OMX_S32                semValue = 0;

    FunctionIn();

    pNAMPort = &pNAMComponent->pNAMPort[portIndex];
    while (NAM_OSAL_GetElemNum(&pNAMPort->bufferQ) > 0) {
        NAM_OSAL_Get_SemaphoreCount(pNAMComponent->pNAMPort[portIndex].bufferSemID, &semValue);
        if (semValue == 0)
            NAM_OSAL_SemaphorePost(pNAMComponent->pNAMPort[portIndex].bufferSemID);
        NAM_OSAL_SemaphoreWait(pNAMComponent->pNAMPort[portIndex].bufferSemID);

        message = (NAM_OMX_MESSAGE *)NAM_OSAL_Dequeue(&pNAMPort->bufferQ);
        if (message != NULL) {
            bufferHeader = (OMX_BUFFERHEADERTYPE *)message->pCmdData;
            bufferHeader->nFilledLen = 0;

            if (CHECK_PORT_TUNNELED(pNAMPort) && !CHECK_PORT_BUFFER_SUPPLIER(pNAMPort)) {
                if (portIndex) {
                    OMX_EmptyThisBuffer(pNAMPort->tunneledComponent, bufferHeader);
                } else {
                    OMX_FillThisBuffer(pNAMPort->tunneledComponent, bufferHeader);
                }
                NAM_OSAL_Free(message);
                message = NULL;
            } else if (CHECK_PORT_TUNNELED(pNAMPort) && CHECK_PORT_BUFFER_SUPPLIER(pNAMPort)) {
                NAM_OSAL_Log(NAM_LOG_ERROR, "Tunneled mode is not working, Line:%d", __LINE__);
                ret = OMX_ErrorNotImplemented;
                NAM_OSAL_Queue(&pNAMPort->bufferQ, pNAMPort);
                goto EXIT;
            } else {
                if (portIndex == OUTPUT_PORT_INDEX) {
                    pNAMComponent->pCallbacks->FillBufferDone(pOMXComponent, pNAMComponent->callbackData, bufferHeader);
                } else {
                    pNAMComponent->pCallbacks->EmptyBufferDone(pOMXComponent, pNAMComponent->callbackData, bufferHeader);
                }

                NAM_OSAL_Free(message);
                message = NULL;
            }
        }
    }

    if (pNAMComponent->namDataBuffer[portIndex].dataValid == OMX_TRUE) {
        if (CHECK_PORT_TUNNELED(pNAMPort) && CHECK_PORT_BUFFER_SUPPLIER(pNAMPort)) {
            message = NAM_OSAL_Malloc(sizeof(NAM_OMX_MESSAGE));
            message->pCmdData = pNAMComponent->namDataBuffer[portIndex].bufferHeader;
            message->messageType = 0;
            message->messageParam = -1;
            NAM_OSAL_Queue(&pNAMPort->bufferQ, message);
            pNAMComponent->nam_BufferReset(pOMXComponent, portIndex);
        } else {
            if (portIndex == INPUT_PORT_INDEX)
                pNAMComponent->nam_InputBufferReturn(pOMXComponent);
            else if (portIndex == OUTPUT_PORT_INDEX)
                pNAMComponent->nam_OutputBufferReturn(pOMXComponent);
        }
    }

    if (CHECK_PORT_TUNNELED(pNAMPort) && CHECK_PORT_BUFFER_SUPPLIER(pNAMPort)) {
        while (NAM_OSAL_GetElemNum(&pNAMPort->bufferQ) < pNAMPort->assignedBufferNum) {
            NAM_OSAL_SemaphoreWait(pNAMComponent->pNAMPort[portIndex].bufferSemID);
        }
        if (NAM_OSAL_GetElemNum(&pNAMPort->bufferQ) != pNAMPort->assignedBufferNum)
            NAM_OSAL_SetElemNum(&pNAMPort->bufferQ, pNAMPort->assignedBufferNum);
    } else {
        while(1) {
            int cnt;
            NAM_OSAL_Get_SemaphoreCount(pNAMComponent->pNAMPort[portIndex].bufferSemID, &cnt);
            if (cnt == 0)
                break;
            NAM_OSAL_SemaphoreWait(pNAMComponent->pNAMPort[portIndex].bufferSemID);
        }
        NAM_OSAL_SetElemNum(&pNAMPort->bufferQ, 0);
    }

    pNAMComponent->processData[portIndex].dataLen       = 0;
    pNAMComponent->processData[portIndex].nFlags        = 0;
    pNAMComponent->processData[portIndex].remainDataLen = 0;
    pNAMComponent->processData[portIndex].timeStamp     = 0;
    pNAMComponent->processData[portIndex].usedDataLen   = 0;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_OMX_BufferFlushProcess(OMX_COMPONENTTYPE *pOMXComponent, OMX_S32 nPortIndex)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    NAM_OMX_BASECOMPONENT *pNAMComponent = NULL;
    NAM_OMX_BASEPORT      *pNAMPort = NULL;
    OMX_S32                portIndex = 0;
    OMX_U32                i = 0, cnt = 0;
    NAM_OMX_DATABUFFER    *flushBuffer = NULL;

    FunctionIn();

    if (pOMXComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    ret = NAM_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    cnt = (nPortIndex == ALL_PORT_INDEX ) ? ALL_PORT_NUM : 1;

    for (i = 0; i < cnt; i++) {
        if (nPortIndex == ALL_PORT_INDEX)
            portIndex = i;
        else
            portIndex = nPortIndex;

        NAM_OSAL_SignalSet(pNAMComponent->pauseEvent);

        flushBuffer = &pNAMComponent->namDataBuffer[portIndex];

        NAM_OSAL_MutexLock(flushBuffer->bufferMutex);
        ret = NAM_OMX_FlushPort(pOMXComponent, portIndex);
        NAM_OSAL_MutexUnlock(flushBuffer->bufferMutex);

        pNAMComponent->pNAMPort[portIndex].bIsPortFlushed = OMX_FALSE;

        if (ret == OMX_ErrorNone) {
            NAM_OSAL_Log(NAM_LOG_TRACE,"OMX_CommandFlush EventCmdComplete");
            pNAMComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                            pNAMComponent->callbackData,
                            OMX_EventCmdComplete,
                            OMX_CommandFlush, portIndex, NULL);
        }

        if (portIndex == INPUT_PORT_INDEX) {
            pNAMComponent->checkTimeStamp.needSetStartTimeStamp = OMX_TRUE;
            pNAMComponent->checkTimeStamp.needCheckStartTimeStamp = OMX_FALSE;
            NAM_OSAL_Memset(pNAMComponent->timeStamp, -19771003, sizeof(OMX_TICKS) * MAX_TIMESTAMP);
            NAM_OSAL_Memset(pNAMComponent->nFlags, 0, sizeof(OMX_U32) * MAX_FLAGS);
            pNAMComponent->getAllDelayBuffer = OMX_FALSE;
            pNAMComponent->bSaveFlagEOS = OMX_FALSE;
            pNAMComponent->reInputData = OMX_FALSE;
        } else if (portIndex == OUTPUT_PORT_INDEX) {
            pNAMComponent->remainOutputData = OMX_FALSE;
        }
    }

EXIT:
    if (ret != OMX_ErrorNone) {
            pNAMComponent->pCallbacks->EventHandler(pOMXComponent,
                            pNAMComponent->callbackData,
                            OMX_EventError,
                            ret, 0, NULL);
    }

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_OMX_BufferFlushProcessNoEvent(OMX_COMPONENTTYPE *pOMXComponent, OMX_S32 nPortIndex)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    NAM_OMX_BASECOMPONENT *pNAMComponent = NULL;
    NAM_OMX_BASEPORT      *pNAMPort = NULL;
    OMX_S32                portIndex = 0;
    OMX_U32                i = 0, cnt = 0;
    NAM_OMX_DATABUFFER    *flushBuffer = NULL;

    FunctionIn();

    if (pOMXComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    ret = NAM_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    cnt = (nPortIndex == ALL_PORT_INDEX ) ? ALL_PORT_NUM : 1;

    for (i = 0; i < cnt; i++) {
        if (nPortIndex == ALL_PORT_INDEX)
            portIndex = i;
        else
            portIndex = nPortIndex;

        pNAMComponent->pNAMPort[portIndex].bIsPortFlushed = OMX_TRUE;

        NAM_OSAL_SignalSet(pNAMComponent->pauseEvent);

        flushBuffer = &pNAMComponent->namDataBuffer[portIndex];

        NAM_OSAL_MutexLock(flushBuffer->bufferMutex);
        ret = NAM_OMX_FlushPort(pOMXComponent, portIndex);
        NAM_OSAL_MutexUnlock(flushBuffer->bufferMutex);

        pNAMComponent->pNAMPort[portIndex].bIsPortFlushed = OMX_FALSE;

        if (portIndex == INPUT_PORT_INDEX) {
            pNAMComponent->checkTimeStamp.needSetStartTimeStamp = OMX_TRUE;
            pNAMComponent->checkTimeStamp.needCheckStartTimeStamp = OMX_FALSE;
            NAM_OSAL_Memset(pNAMComponent->timeStamp, -19771003, sizeof(OMX_TICKS) * MAX_TIMESTAMP);
            NAM_OSAL_Memset(pNAMComponent->nFlags, 0, sizeof(OMX_U32) * MAX_FLAGS);
            pNAMComponent->getAllDelayBuffer = OMX_FALSE;
            pNAMComponent->bSaveFlagEOS = OMX_FALSE;
            pNAMComponent->remainOutputData = OMX_FALSE;
            pNAMComponent->reInputData = OMX_FALSE;
        }
    }

EXIT:
    if (ret != OMX_ErrorNone) {
        pNAMComponent->pCallbacks->EventHandler(pOMXComponent,
                            pNAMComponent->callbackData,
                            OMX_EventError,
                            ret, 0, NULL);
    }

    FunctionOut();

    return ret;
}


OMX_ERRORTYPE NAM_OMX_EnablePort(OMX_COMPONENTTYPE *pOMXComponent, OMX_S32 portIndex)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    NAM_OMX_BASECOMPONENT *pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    NAM_OMX_BASEPORT      *pNAMPort = NULL;
    OMX_U32                i = 0, cnt = 0;

    FunctionIn();

    pNAMPort = &pNAMComponent->pNAMPort[portIndex];
    pNAMPort->portDefinition.bEnabled = OMX_TRUE;

    if (CHECK_PORT_TUNNELED(pNAMPort) && CHECK_PORT_BUFFER_SUPPLIER(pNAMPort)) {
        ret = pNAMComponent->nam_AllocateTunnelBuffer(pNAMPort, portIndex);
        if (OMX_ErrorNone != ret) {
            goto EXIT;
        }
        pNAMPort->portDefinition.bPopulated = OMX_TRUE;
        if (pNAMComponent->currentState == OMX_StateExecuting) {
            for (i=0; i<pNAMPort->tunnelBufferNum; i++) {
                NAM_OSAL_SemaphorePost(pNAMComponent->pNAMPort[portIndex].bufferSemID);
            }
        }
    } else if (CHECK_PORT_TUNNELED(pNAMPort) && !CHECK_PORT_BUFFER_SUPPLIER(pNAMPort)) {
        if ((pNAMComponent->currentState != OMX_StateLoaded) && (pNAMComponent->currentState != OMX_StateWaitForResources)) {
            NAM_OSAL_SemaphoreWait(pNAMPort->loadedResource);
            pNAMPort->portDefinition.bPopulated = OMX_TRUE;
        }
    } else {
        if ((pNAMComponent->currentState != OMX_StateLoaded) && (pNAMComponent->currentState != OMX_StateWaitForResources)) {
            NAM_OSAL_SemaphoreWait(pNAMPort->loadedResource);
            pNAMPort->portDefinition.bPopulated = OMX_TRUE;
        }
    }
    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_OMX_PortEnableProcess(OMX_COMPONENTTYPE *pOMXComponent, OMX_S32 nPortIndex)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    NAM_OMX_BASECOMPONENT *pNAMComponent = NULL;
    OMX_S32                portIndex = 0;
    OMX_U32                i = 0, cnt = 0;

    FunctionIn();

    if (pOMXComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    ret = NAM_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    cnt = (nPortIndex == ALL_PORT_INDEX) ? ALL_PORT_NUM : 1;

    for (i = 0; i < cnt; i++) {
        if (nPortIndex == ALL_PORT_INDEX)
            portIndex = i;
        else
            portIndex = nPortIndex;

        ret = NAM_OMX_EnablePort(pOMXComponent, portIndex);
        if (ret == OMX_ErrorNone) {
            pNAMComponent->pCallbacks->EventHandler(pOMXComponent,
                            pNAMComponent->callbackData,
                            OMX_EventCmdComplete,
                            OMX_CommandPortEnable, portIndex, NULL);
        }
    }

EXIT:
    if (ret != OMX_ErrorNone) {
            pNAMComponent->pCallbacks->EventHandler(pOMXComponent,
                            pNAMComponent->callbackData,
                            OMX_EventError,
                            ret, 0, NULL);
        }

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_OMX_DisablePort(OMX_COMPONENTTYPE *pOMXComponent, OMX_S32 portIndex)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    NAM_OMX_BASECOMPONENT *pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    NAM_OMX_BASEPORT      *pNAMPort = NULL;
    OMX_U32                i = 0, elemNum = 0;
    NAM_OMX_MESSAGE       *message;

    FunctionIn();

    pNAMPort = &pNAMComponent->pNAMPort[portIndex];

    if (!CHECK_PORT_ENABLED(pNAMPort)) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    if (pNAMComponent->currentState!=OMX_StateLoaded) {
        if (CHECK_PORT_TUNNELED(pNAMPort) && CHECK_PORT_BUFFER_SUPPLIER(pNAMPort)) {
            while (NAM_OSAL_GetElemNum(&pNAMPort->bufferQ) >0 ) {
                message = (NAM_OMX_MESSAGE*)NAM_OSAL_Dequeue(&pNAMPort->bufferQ);
                NAM_OSAL_Free(message);
            }
            ret = pNAMComponent->nam_FreeTunnelBuffer(pNAMPort, portIndex);
            if (OMX_ErrorNone != ret) {
                goto EXIT;
            }
            pNAMPort->portDefinition.bPopulated = OMX_FALSE;
        } else if (CHECK_PORT_TUNNELED(pNAMPort) && !CHECK_PORT_BUFFER_SUPPLIER(pNAMPort)) {
            pNAMPort->portDefinition.bPopulated = OMX_FALSE;
            NAM_OSAL_SemaphoreWait(pNAMPort->unloadedResource);
        } else {
            if (CHECK_PORT_BUFFER_SUPPLIER(pNAMPort)) {
                while (NAM_OSAL_GetElemNum(&pNAMPort->bufferQ) >0 ) {
                    message = (NAM_OMX_MESSAGE*)NAM_OSAL_Dequeue(&pNAMPort->bufferQ);
                    NAM_OSAL_Free(message);
                }
            }
            pNAMPort->portDefinition.bPopulated = OMX_FALSE;
            NAM_OSAL_SemaphoreWait(pNAMPort->unloadedResource);
        }
    }
    pNAMPort->portDefinition.bEnabled = OMX_FALSE;
    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_OMX_PortDisableProcess(OMX_COMPONENTTYPE *pOMXComponent, OMX_S32 nPortIndex)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    NAM_OMX_BASECOMPONENT *pNAMComponent = NULL;
    OMX_S32                portIndex = 0;
    OMX_U32                i = 0, cnt = 0;
    NAM_OMX_DATABUFFER      *flushBuffer = NULL;

    FunctionIn();

    if (pOMXComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    ret = NAM_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    cnt = (nPortIndex == ALL_PORT_INDEX ) ? ALL_PORT_NUM : 1;

    /* port flush*/
    for(i = 0; i < cnt; i++) {
        if (nPortIndex == ALL_PORT_INDEX)
            portIndex = i;
        else
            portIndex = nPortIndex;

        pNAMComponent->pNAMPort[portIndex].bIsPortFlushed = OMX_TRUE;

        flushBuffer = &pNAMComponent->namDataBuffer[portIndex];

        NAM_OSAL_MutexLock(flushBuffer->bufferMutex);
        ret = NAM_OMX_FlushPort(pOMXComponent, portIndex);
        NAM_OSAL_MutexUnlock(flushBuffer->bufferMutex);

        pNAMComponent->pNAMPort[portIndex].bIsPortFlushed = OMX_FALSE;

        if (portIndex == INPUT_PORT_INDEX) {
            pNAMComponent->checkTimeStamp.needSetStartTimeStamp = OMX_TRUE;
            pNAMComponent->checkTimeStamp.needCheckStartTimeStamp = OMX_FALSE;
            NAM_OSAL_Memset(pNAMComponent->timeStamp, -19771003, sizeof(OMX_TICKS) * MAX_TIMESTAMP);
            NAM_OSAL_Memset(pNAMComponent->nFlags, 0, sizeof(OMX_U32) * MAX_FLAGS);
            pNAMComponent->getAllDelayBuffer = OMX_FALSE;
            pNAMComponent->bSaveFlagEOS = OMX_FALSE;
            pNAMComponent->reInputData = OMX_FALSE;
        } else if (portIndex == OUTPUT_PORT_INDEX) {
            pNAMComponent->remainOutputData = OMX_FALSE;
        }
    }

    for(i = 0; i < cnt; i++) {
        if (nPortIndex == ALL_PORT_INDEX)
            portIndex = i;
        else
            portIndex = nPortIndex;

        ret = NAM_OMX_DisablePort(pOMXComponent, portIndex);
        pNAMComponent->pNAMPort[portIndex].bIsPortDisabled = OMX_FALSE;
        if (ret == OMX_ErrorNone) {
            pNAMComponent->pCallbacks->EventHandler(pOMXComponent,
                            pNAMComponent->callbackData,
                            OMX_EventCmdComplete,
                            OMX_CommandPortDisable, portIndex, NULL);
        }
    }

EXIT:
    if (ret != OMX_ErrorNone) {
        pNAMComponent->pCallbacks->EventHandler(pOMXComponent,
                        pNAMComponent->callbackData,
                        OMX_EventError,
                        ret, 0, NULL);
    }

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_OMX_EmptyThisBuffer(
    OMX_IN OMX_HANDLETYPE        hComponent,
    OMX_IN OMX_BUFFERHEADERTYPE *pBuffer)
{
    OMX_ERRORTYPE           ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    NAM_OMX_BASECOMPONENT *pNAMComponent = NULL;
    NAM_OMX_BASEPORT      *pNAMPort = NULL;
    OMX_BOOL               findBuffer = OMX_FALSE;
    NAM_OMX_MESSAGE       *message;
    OMX_U32                i = 0;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = NAM_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pNAMComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    if (pBuffer == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pBuffer->nInputPortIndex != INPUT_PORT_INDEX) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    ret = NAM_OMX_Check_SizeVersion(pBuffer, sizeof(OMX_BUFFERHEADERTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if ((pNAMComponent->currentState != OMX_StateIdle) &&
        (pNAMComponent->currentState != OMX_StateExecuting) &&
        (pNAMComponent->currentState != OMX_StatePause)) {
        ret = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    pNAMPort = &pNAMComponent->pNAMPort[INPUT_PORT_INDEX];
    if ((!CHECK_PORT_ENABLED(pNAMPort)) ||
        ((CHECK_PORT_BEING_FLUSHED(pNAMPort) || CHECK_PORT_BEING_DISABLED(pNAMPort)) &&
        (!CHECK_PORT_TUNNELED(pNAMPort) || !CHECK_PORT_BUFFER_SUPPLIER(pNAMPort))) ||
        ((pNAMComponent->transientState == NAM_OMX_TransStateExecutingToIdle) &&
        (CHECK_PORT_TUNNELED(pNAMPort) && !CHECK_PORT_BUFFER_SUPPLIER(pNAMPort)))) {
        ret = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    for (i = 0; i < pNAMPort->portDefinition.nBufferCountActual; i++) {
        if (pBuffer == pNAMPort->bufferHeader[i]) {
            findBuffer = OMX_TRUE;
            break;
        }
    }

    if (findBuffer == OMX_FALSE) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    } else {
        ret = OMX_ErrorNone;
    }

    message = NAM_OSAL_Malloc(sizeof(NAM_OMX_MESSAGE));
    if (message == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    message->messageType = NAM_OMX_CommandEmptyBuffer;
    message->messageParam = (OMX_U32) i;
    message->pCmdData = (OMX_PTR)pBuffer;

    NAM_OSAL_Queue(&pNAMPort->bufferQ, (void *)message);
    NAM_OSAL_SemaphorePost(pNAMPort->bufferSemID);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_OMX_FillThisBuffer(
    OMX_IN OMX_HANDLETYPE        hComponent,
    OMX_IN OMX_BUFFERHEADERTYPE *pBuffer)
{
    OMX_ERRORTYPE           ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    NAM_OMX_BASECOMPONENT *pNAMComponent = NULL;
    NAM_OMX_BASEPORT      *pNAMPort = NULL;
    OMX_BOOL               findBuffer = OMX_FALSE;
    NAM_OMX_MESSAGE       *message;
    OMX_U32                i = 0;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = NAM_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pNAMComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    if (pBuffer == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pBuffer->nOutputPortIndex != OUTPUT_PORT_INDEX) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    ret = NAM_OMX_Check_SizeVersion(pBuffer, sizeof(OMX_BUFFERHEADERTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if ((pNAMComponent->currentState != OMX_StateIdle) &&
        (pNAMComponent->currentState != OMX_StateExecuting) &&
        (pNAMComponent->currentState != OMX_StatePause)) {
        ret = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    pNAMPort = &pNAMComponent->pNAMPort[OUTPUT_PORT_INDEX];
    if ((!CHECK_PORT_ENABLED(pNAMPort)) ||
        ((CHECK_PORT_BEING_FLUSHED(pNAMPort) || CHECK_PORT_BEING_DISABLED(pNAMPort)) &&
        (!CHECK_PORT_TUNNELED(pNAMPort) || !CHECK_PORT_BUFFER_SUPPLIER(pNAMPort))) ||
        ((pNAMComponent->transientState == NAM_OMX_TransStateExecutingToIdle) &&
        (CHECK_PORT_TUNNELED(pNAMPort) && !CHECK_PORT_BUFFER_SUPPLIER(pNAMPort)))) {
        ret = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    for (i = 0; i < pNAMPort->portDefinition.nBufferCountActual; i++) {
        if (pBuffer == pNAMPort->bufferHeader[i]) {
            findBuffer = OMX_TRUE;
            break;
        }
    }

    if (findBuffer == OMX_FALSE) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    } else {
        ret = OMX_ErrorNone;
    }

    /* Release dsp buffer: set UseMark */
    ret = pNAMComponent->nam_dmai_release_buffer(pOMXComponent, pBuffer);
    if (ret != OMX_ErrorNone) {
	goto EXIT;
    }

    message = NAM_OSAL_Malloc(sizeof(NAM_OMX_MESSAGE));
    if (message == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    message->messageType = NAM_OMX_CommandFillBuffer;
    message->messageParam = (OMX_U32) i;
    message->pCmdData = (OMX_PTR)pBuffer;

    NAM_OSAL_Queue(&pNAMPort->bufferQ, (void *)message);
    NAM_OSAL_SemaphorePost(pNAMPort->bufferSemID);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_OMX_Port_Constructor(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    NAM_OMX_BASECOMPONENT *pNAMComponent = NULL;
    NAM_OMX_BASEPORT      *pNAMPort = NULL;
    NAM_OMX_BASEPORT      *pNAMInputPort = NULL;
    NAM_OMX_BASEPORT      *pNAMOutputPort = NULL;
    int i = 0;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        NAM_OSAL_Log(NAM_LOG_ERROR, "OMX_ErrorBadParameter, Line:%d", __LINE__);
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = NAM_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        NAM_OSAL_Log(NAM_LOG_ERROR, "OMX_ErrorBadParameter, Line:%d", __LINE__);
        goto EXIT;
    }
    pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    INIT_SET_SIZE_VERSION(&pNAMComponent->portParam, OMX_PORT_PARAM_TYPE);
    pNAMComponent->portParam.nPorts = ALL_PORT_NUM;
    pNAMComponent->portParam.nStartPortNumber = INPUT_PORT_INDEX;

    pNAMPort = NAM_OSAL_Malloc(sizeof(NAM_OMX_BASEPORT) * ALL_PORT_NUM);
    if (pNAMPort == NULL) {
        ret = OMX_ErrorInsufficientResources;
        NAM_OSAL_Log(NAM_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    NAM_OSAL_Memset(pNAMPort, 0, sizeof(NAM_OMX_BASEPORT) * ALL_PORT_NUM);
    pNAMComponent->pNAMPort = pNAMPort;

    /* Input Port */
    pNAMInputPort = &pNAMPort[INPUT_PORT_INDEX];

    NAM_OSAL_QueueCreate(&pNAMInputPort->bufferQ);

    pNAMInputPort->bufferHeader = NAM_OSAL_Malloc(sizeof(OMX_BUFFERHEADERTYPE*) * MAX_BUFFER_NUM);
    if (pNAMInputPort->bufferHeader == NULL) {
        NAM_OSAL_Free(pNAMPort);
        pNAMPort = NULL;
        ret = OMX_ErrorInsufficientResources;
        NAM_OSAL_Log(NAM_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    NAM_OSAL_Memset(pNAMInputPort->bufferHeader, 0, sizeof(OMX_BUFFERHEADERTYPE*) * MAX_BUFFER_NUM);

    pNAMInputPort->bufferStateAllocate = NAM_OSAL_Malloc(sizeof(OMX_U32) * MAX_BUFFER_NUM);
    if (pNAMInputPort->bufferStateAllocate == NULL) {
        NAM_OSAL_Free(pNAMInputPort->bufferHeader);
        pNAMInputPort->bufferHeader = NULL;
        NAM_OSAL_Free(pNAMPort);
        pNAMPort = NULL;
        ret = OMX_ErrorInsufficientResources;
        NAM_OSAL_Log(NAM_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    NAM_OSAL_Memset(pNAMInputPort->bufferStateAllocate, 0, sizeof(OMX_U32) * MAX_BUFFER_NUM);

    pNAMInputPort->bufferSemID = NULL;
    pNAMInputPort->assignedBufferNum = 0;
    pNAMInputPort->portState = OMX_StateMax;
    pNAMInputPort->bIsPortFlushed = OMX_FALSE;
    pNAMInputPort->bIsPortDisabled = OMX_FALSE;
    pNAMInputPort->tunneledComponent = NULL;
    pNAMInputPort->tunneledPort = 0;
    pNAMInputPort->tunnelBufferNum = 0;
    pNAMInputPort->bufferSupplier = OMX_BufferSupplyUnspecified;
    pNAMInputPort->tunnelFlags = 0;
    pNAMInputPort->eControlRate = OMX_Video_ControlRateDisable;
    ret = NAM_OSAL_SemaphoreCreate(&pNAMInputPort->loadedResource);
    if (ret != OMX_ErrorNone) {
        NAM_OSAL_Free(pNAMInputPort->bufferStateAllocate);
        pNAMInputPort->bufferStateAllocate = NULL;
        NAM_OSAL_Free(pNAMInputPort->bufferHeader);
        pNAMInputPort->bufferHeader = NULL;
        NAM_OSAL_Free(pNAMPort);
        pNAMPort = NULL;
        goto EXIT;
    }
    ret = NAM_OSAL_SemaphoreCreate(&pNAMInputPort->unloadedResource);
    if (ret != OMX_ErrorNone) {
        NAM_OSAL_SemaphoreTerminate(pNAMInputPort->loadedResource);
        pNAMInputPort->loadedResource = NULL;
        NAM_OSAL_Free(pNAMInputPort->bufferStateAllocate);
        pNAMInputPort->bufferStateAllocate = NULL;
        NAM_OSAL_Free(pNAMInputPort->bufferHeader);
        pNAMInputPort->bufferHeader = NULL;
        NAM_OSAL_Free(pNAMPort);
        pNAMPort = NULL;
        goto EXIT;
    }

    INIT_SET_SIZE_VERSION(&pNAMInputPort->portDefinition, OMX_PARAM_PORTDEFINITIONTYPE);
    pNAMInputPort->portDefinition.nPortIndex = INPUT_PORT_INDEX;
    pNAMInputPort->portDefinition.eDir = OMX_DirInput;
    pNAMInputPort->portDefinition.nBufferCountActual = 0;
    pNAMInputPort->portDefinition.nBufferCountMin = 0;
    pNAMInputPort->portDefinition.nBufferSize = 0;
    pNAMInputPort->portDefinition.bEnabled = OMX_FALSE;
    pNAMInputPort->portDefinition.bPopulated = OMX_FALSE;
    pNAMInputPort->portDefinition.eDomain = OMX_PortDomainMax;
    pNAMInputPort->portDefinition.bBuffersContiguous = OMX_FALSE;
    pNAMInputPort->portDefinition.nBufferAlignment = 0;
    pNAMInputPort->markType.hMarkTargetComponent = NULL;
    pNAMInputPort->markType.pMarkData = NULL;
    pNAMInputPort->bUseAndroidNativeBuffer = OMX_FALSE;
    pNAMInputPort->bStoreMetaDataInBuffer = OMX_FALSE;

    /* Output Port */
    pNAMOutputPort = &pNAMPort[OUTPUT_PORT_INDEX];

    NAM_OSAL_QueueCreate(&pNAMOutputPort->bufferQ);

    pNAMOutputPort->bufferHeader = NAM_OSAL_Malloc(sizeof(OMX_BUFFERHEADERTYPE*) * MAX_BUFFER_NUM);
    if (pNAMOutputPort->bufferHeader == NULL) {
        NAM_OSAL_SemaphoreTerminate(pNAMInputPort->unloadedResource);
        pNAMInputPort->unloadedResource = NULL;
        NAM_OSAL_SemaphoreTerminate(pNAMInputPort->loadedResource);
        pNAMInputPort->loadedResource = NULL;
        NAM_OSAL_Free(pNAMInputPort->bufferStateAllocate);
        pNAMInputPort->bufferStateAllocate = NULL;
        NAM_OSAL_Free(pNAMInputPort->bufferHeader);
        pNAMInputPort->bufferHeader = NULL;
        NAM_OSAL_Free(pNAMPort);
        pNAMPort = NULL;
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    NAM_OSAL_Memset(pNAMOutputPort->bufferHeader, 0, sizeof(OMX_BUFFERHEADERTYPE*) * MAX_BUFFER_NUM);

    pNAMOutputPort->bufferStateAllocate = NAM_OSAL_Malloc(sizeof(OMX_U32) * MAX_BUFFER_NUM);
    if (pNAMOutputPort->bufferStateAllocate == NULL) {
        NAM_OSAL_Free(pNAMOutputPort->bufferHeader);
        pNAMOutputPort->bufferHeader = NULL;

        NAM_OSAL_SemaphoreTerminate(pNAMInputPort->unloadedResource);
        pNAMInputPort->unloadedResource = NULL;
        NAM_OSAL_SemaphoreTerminate(pNAMInputPort->loadedResource);
        pNAMInputPort->loadedResource = NULL;
        NAM_OSAL_Free(pNAMInputPort->bufferStateAllocate);
        pNAMInputPort->bufferStateAllocate = NULL;
        NAM_OSAL_Free(pNAMInputPort->bufferHeader);
        pNAMInputPort->bufferHeader = NULL;
        NAM_OSAL_Free(pNAMPort);
        pNAMPort = NULL;
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    NAM_OSAL_Memset(pNAMOutputPort->bufferStateAllocate, 0, sizeof(OMX_U32) * MAX_BUFFER_NUM);

    pNAMOutputPort->bufferSemID = NULL;
    pNAMOutputPort->assignedBufferNum = 0;
    pNAMOutputPort->portState = OMX_StateMax;
    pNAMOutputPort->bIsPortFlushed = OMX_FALSE;
    pNAMOutputPort->bIsPortDisabled = OMX_FALSE;
    pNAMOutputPort->tunneledComponent = NULL;
    pNAMOutputPort->tunneledPort = 0;
    pNAMOutputPort->tunnelBufferNum = 0;
    pNAMOutputPort->bufferSupplier = OMX_BufferSupplyUnspecified;
    pNAMOutputPort->tunnelFlags = 0;
    pNAMOutputPort->eControlRate = OMX_Video_ControlRateDisable;
    ret = NAM_OSAL_SemaphoreCreate(&pNAMOutputPort->loadedResource);
    if (ret != OMX_ErrorNone) {
        NAM_OSAL_Free(pNAMOutputPort->bufferStateAllocate);
        pNAMOutputPort->bufferStateAllocate = NULL;
        NAM_OSAL_Free(pNAMOutputPort->bufferHeader);
        pNAMOutputPort->bufferHeader = NULL;

        NAM_OSAL_SemaphoreTerminate(pNAMInputPort->unloadedResource);
        pNAMInputPort->unloadedResource = NULL;
        NAM_OSAL_SemaphoreTerminate(pNAMInputPort->loadedResource);
        pNAMInputPort->loadedResource = NULL;
        NAM_OSAL_Free(pNAMInputPort->bufferStateAllocate);
        pNAMInputPort->bufferStateAllocate = NULL;
        NAM_OSAL_Free(pNAMInputPort->bufferHeader);
        pNAMInputPort->bufferHeader = NULL;
        NAM_OSAL_Free(pNAMPort);
        pNAMPort = NULL;
        goto EXIT;
    }
    ret = NAM_OSAL_SemaphoreCreate(&pNAMOutputPort->unloadedResource);
    if (ret != OMX_ErrorNone) {
        NAM_OSAL_SemaphoreTerminate(pNAMOutputPort->loadedResource);
        pNAMOutputPort->loadedResource = NULL;
        NAM_OSAL_Free(pNAMOutputPort->bufferStateAllocate);
        pNAMOutputPort->bufferStateAllocate = NULL;
        NAM_OSAL_Free(pNAMOutputPort->bufferHeader);
        pNAMOutputPort->bufferHeader = NULL;

        NAM_OSAL_SemaphoreTerminate(pNAMInputPort->unloadedResource);
        pNAMInputPort->unloadedResource = NULL;
        NAM_OSAL_SemaphoreTerminate(pNAMInputPort->loadedResource);
        pNAMInputPort->loadedResource = NULL;
        NAM_OSAL_Free(pNAMInputPort->bufferStateAllocate);
        pNAMInputPort->bufferStateAllocate = NULL;
        NAM_OSAL_Free(pNAMInputPort->bufferHeader);
        pNAMInputPort->bufferHeader = NULL;
        NAM_OSAL_Free(pNAMPort);
        pNAMPort = NULL;
        goto EXIT;
    }

    INIT_SET_SIZE_VERSION(&pNAMOutputPort->portDefinition, OMX_PARAM_PORTDEFINITIONTYPE);
    pNAMOutputPort->portDefinition.nPortIndex = OUTPUT_PORT_INDEX;
    pNAMOutputPort->portDefinition.eDir = OMX_DirOutput;
    pNAMOutputPort->portDefinition.nBufferCountActual = 0;
    pNAMOutputPort->portDefinition.nBufferCountMin = 0;
    pNAMOutputPort->portDefinition.nBufferSize = 0;
    pNAMOutputPort->portDefinition.bEnabled = OMX_FALSE;
    pNAMOutputPort->portDefinition.bPopulated = OMX_FALSE;
    pNAMOutputPort->portDefinition.eDomain = OMX_PortDomainMax;
    pNAMOutputPort->portDefinition.bBuffersContiguous = OMX_FALSE;
    pNAMOutputPort->portDefinition.nBufferAlignment = 0;
    pNAMOutputPort->markType.hMarkTargetComponent = NULL;
    pNAMOutputPort->markType.pMarkData = NULL;
    pNAMOutputPort->bUseAndroidNativeBuffer = OMX_FALSE;
    pNAMOutputPort->bStoreMetaDataInBuffer = OMX_FALSE;

    pNAMComponent->checkTimeStamp.needSetStartTimeStamp = OMX_FALSE;
    pNAMComponent->checkTimeStamp.needCheckStartTimeStamp = OMX_FALSE;
    pNAMComponent->checkTimeStamp.startTimeStamp = 0;
    pNAMComponent->checkTimeStamp.nStartFlags = 0x0;

    pOMXComponent->EmptyThisBuffer = &NAM_OMX_EmptyThisBuffer;
    pOMXComponent->FillThisBuffer  = &NAM_OMX_FillThisBuffer;

    ret = OMX_ErrorNone;
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_OMX_Port_Destructor(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    NAM_OMX_BASECOMPONENT *pNAMComponent = NULL;
    NAM_OMX_BASEPORT      *pNAMPort = NULL;

    FunctionIn();

    int i = 0;

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = NAM_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    for (i = 0; i < ALL_PORT_NUM; i++) {
        pNAMPort = &pNAMComponent->pNAMPort[i];

        NAM_OSAL_SemaphoreTerminate(pNAMPort->loadedResource);
        pNAMPort->loadedResource = NULL;
        NAM_OSAL_SemaphoreTerminate(pNAMPort->unloadedResource);
        pNAMPort->unloadedResource = NULL;
        NAM_OSAL_Free(pNAMPort->bufferStateAllocate);
        pNAMPort->bufferStateAllocate = NULL;
        NAM_OSAL_Free(pNAMPort->bufferHeader);
        pNAMPort->bufferHeader = NULL;

        NAM_OSAL_QueueTerminate(&pNAMPort->bufferQ);
    }
    NAM_OSAL_Free(pNAMComponent->pNAMPort);
    pNAMComponent->pNAMPort = NULL;
    ret = OMX_ErrorNone;
EXIT:
    FunctionOut();

    return ret;
}

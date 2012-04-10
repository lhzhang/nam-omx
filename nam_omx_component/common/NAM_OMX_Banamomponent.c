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
 * @file       NAM_OMX_Banamomponent.c
 * @brief
 * @author     SeungBeom Kim (sbcrux.kim@samsung.com)
 *             Yunji Kim (yunji.kim@samsung.com)
 * @version    1.0
 * @history
 *    2010.7.15 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "NAM_OSAL_Event.h"
#include "NAM_OSAL_Thread.h"
#include "NAM_OMX_Baseport.h"
#include "NAM_OMX_Banamomponent.h"
#include "NAM_OMX_Macros.h"

#undef  NAM_LOG_TAG
#define NAM_LOG_TAG    "NAM_BASE_COMP"
#define NAM_LOG_OFF
#include "NAM_OSAL_Log.h"


/* Change CHECK_SIZE_VERSION Macro */
OMX_ERRORTYPE NAM_OMX_Check_SizeVersion(OMX_PTR header, OMX_U32 size)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    OMX_VERSIONTYPE* version = NULL;
    if (header == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    version = (OMX_VERSIONTYPE*)((char*)header + sizeof(OMX_U32));
    if (*((OMX_U32*)header) != size) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (version->s.nVersionMajor != VERSIONMAJOR_NUMBER ||
        version->s.nVersionMinor != VERSIONMINOR_NUMBER) {
        ret = OMX_ErrorVersionMismatch;
        goto EXIT;
    }
    ret = OMX_ErrorNone;
EXIT:
    return ret;
}

OMX_ERRORTYPE NAM_OMX_GetComponentVersion(
    OMX_IN  OMX_HANDLETYPE   hComponent,
    OMX_OUT OMX_STRING       pComponentName,
    OMX_OUT OMX_VERSIONTYPE *pComponentVersion,
    OMX_OUT OMX_VERSIONTYPE *pSpecVersion,
    OMX_OUT OMX_UUIDTYPE    *pComponentUUID)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    NAM_OMX_BANAMOMPONENT *pNAMComponent = NULL;
    OMX_U32                compUUID[3];

    FunctionIn();

    /* check parameters */
    if (hComponent     == NULL ||
        pComponentName == NULL || pComponentVersion == NULL ||
        pSpecVersion   == NULL || pComponentUUID    == NULL) {
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
    pNAMComponent = (NAM_OMX_BANAMOMPONENT *)pOMXComponent->pComponentPrivate;

    if (pNAMComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    NAM_OSAL_Strcpy(pComponentName, pNAMComponent->componentName);
    NAM_OSAL_Memcpy(pComponentVersion, &(pNAMComponent->componentVersion), sizeof(OMX_VERSIONTYPE));
    NAM_OSAL_Memcpy(pSpecVersion, &(pNAMComponent->specVersion), sizeof(OMX_VERSIONTYPE));

    /* Fill UUID with handle address, PID and UID.
     * This should guarantee uiniqness */
    compUUID[0] = (OMX_U32)pOMXComponent;
    compUUID[1] = getpid();
    compUUID[2] = getuid();
    NAM_OSAL_Memcpy(*pComponentUUID, compUUID, 3 * sizeof(*compUUID));

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_OMX_GetState (
    OMX_IN OMX_HANDLETYPE  hComponent,
    OMX_OUT OMX_STATETYPE *pState)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    NAM_OMX_BANAMOMPONENT *pNAMComponent = NULL;

    FunctionIn();

    if (hComponent == NULL || pState == NULL) {
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
    pNAMComponent = (NAM_OMX_BANAMOMPONENT *)pOMXComponent->pComponentPrivate;

    *pState = pNAMComponent->currentState;
    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE NAM_OMX_BufferProcessThread(OMX_PTR threadData)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    NAM_OMX_BANAMOMPONENT *pNAMComponent = NULL;
    NAM_OMX_MESSAGE       *message = NULL;

    FunctionIn();

    if (threadData == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)threadData;
    ret = NAM_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    pNAMComponent = (NAM_OMX_BANAMOMPONENT *)pOMXComponent->pComponentPrivate;
    pNAMComponent->nam_BufferProcess(pOMXComponent);

    NAM_OSAL_ThreadExit(NULL);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_OMX_ComponentStateSet(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 messageParam)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    NAM_OMX_BANAMOMPONENT *pNAMComponent = (NAM_OMX_BANAMOMPONENT *)pOMXComponent->pComponentPrivate;
    NAM_OMX_MESSAGE       *message;
    OMX_STATETYPE          destState = messageParam;
    OMX_STATETYPE          currentState = pNAMComponent->currentState;
    NAM_OMX_BASEPORT      *pNAMPort = NULL;
    OMX_S32                countValue = 0;
    int                   i = 0, j = 0;

    FunctionIn();

    /* check parameters */
    if (currentState == destState) {
         ret = OMX_ErrorSameState;
            goto EXIT;
    }
    if (currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    if ((currentState == OMX_StateLoaded) && (destState == OMX_StateIdle)) {
        ret = NAM_OMX_Get_Resource(pOMXComponent);
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }
    }
    if (((currentState == OMX_StateIdle) && (destState == OMX_StateLoaded))       ||
        ((currentState == OMX_StateIdle) && (destState == OMX_StateInvalid))      ||
        ((currentState == OMX_StateExecuting) && (destState == OMX_StateInvalid)) ||
        ((currentState == OMX_StatePause) && (destState == OMX_StateInvalid))) {
        NAM_OMX_Release_Resource(pOMXComponent);
    }

    NAM_OSAL_Log(NAM_LOG_TRACE, "destState: %d", destState);

    switch (destState) {
    case OMX_StateInvalid:
        switch (currentState) {
        case OMX_StateIdle:
        case OMX_StateExecuting:
        case OMX_StatePause:
        case OMX_StateLoaded:
        case OMX_StateWaitForResources:
            pNAMComponent->currentState = OMX_StateInvalid;
            if (pNAMComponent->hBufferProcess) {
                pNAMComponent->bExitBufferProcessThread = OMX_TRUE;

                for (i = 0; i < ALL_PORT_NUM; i++) {
                    NAM_OSAL_Get_SemaphoreCount(pNAMComponent->pNAMPort[i].bufferSemID, &countValue);
                    if (countValue == 0)
                        NAM_OSAL_SemaphorePost(pNAMComponent->pNAMPort[i].bufferSemID);
                }

                NAM_OSAL_SignalSet(pNAMComponent->pauseEvent);
                NAM_OSAL_ThreadTerminate(pNAMComponent->hBufferProcess);
                pNAMComponent->hBufferProcess = NULL;

                for (i = 0; i < ALL_PORT_NUM; i++) {
                    NAM_OSAL_MutexTerminate(pNAMComponent->namDataBuffer[i].bufferMutex);
                    pNAMComponent->namDataBuffer[i].bufferMutex = NULL;
                }

                NAM_OSAL_SignalTerminate(pNAMComponent->pauseEvent);
                for (i = 0; i < ALL_PORT_NUM; i++) {
                    NAM_OSAL_SemaphoreTerminate(pNAMComponent->pNAMPort[i].bufferSemID);
                    pNAMComponent->pNAMPort[i].bufferSemID = NULL;
                }
            }
            if (pNAMComponent->nam_dmai_componentTerminate != NULL)
                pNAMComponent->nam_dmai_componentTerminate(pOMXComponent);
            break;
        }
        ret = OMX_ErrorInvalidState;
        break;
    case OMX_StateLoaded:
        switch (currentState) {
        case OMX_StateIdle:
            pNAMComponent->bExitBufferProcessThread = OMX_TRUE;

            for (i = 0; i < ALL_PORT_NUM; i++) {
                NAM_OSAL_Get_SemaphoreCount(pNAMComponent->pNAMPort[i].bufferSemID, &countValue);
                if (countValue == 0)
                    NAM_OSAL_SemaphorePost(pNAMComponent->pNAMPort[i].bufferSemID);
            }

            NAM_OSAL_SignalSet(pNAMComponent->pauseEvent);
            NAM_OSAL_ThreadTerminate(pNAMComponent->hBufferProcess);
            pNAMComponent->hBufferProcess = NULL;

            for (i = 0; i < ALL_PORT_NUM; i++) {
                NAM_OSAL_MutexTerminate(pNAMComponent->namDataBuffer[i].bufferMutex);
                pNAMComponent->namDataBuffer[i].bufferMutex = NULL;
            }

            NAM_OSAL_SignalTerminate(pNAMComponent->pauseEvent);
            for (i = 0; i < ALL_PORT_NUM; i++) {
                NAM_OSAL_SemaphoreTerminate(pNAMComponent->pNAMPort[i].bufferSemID);
                pNAMComponent->pNAMPort[i].bufferSemID = NULL;
            }

            pNAMComponent->nam_dmai_componentTerminate(pOMXComponent);

            for (i = 0; i < (pNAMComponent->portParam.nPorts); i++) {
                pNAMPort = (pNAMComponent->pNAMPort + i);
                if (CHECK_PORT_TUNNELED(pNAMPort) && CHECK_PORT_BUFFER_SUPPLIER(pNAMPort)) {
                    while (NAM_OSAL_GetElemNum(&pNAMPort->bufferQ) > 0) {
                        message = (NAM_OMX_MESSAGE*)NAM_OSAL_Dequeue(&pNAMPort->bufferQ);
                        if (message != NULL)
                            NAM_OSAL_Free(message);
                    }
                    ret = pNAMComponent->nam_FreeTunnelBuffer(pNAMComponent, i);
                    if (OMX_ErrorNone != ret) {
                        goto EXIT;
                    }
                } else {
                    if (CHECK_PORT_ENABLED(pNAMPort)) {
                        NAM_OSAL_SemaphoreWait(pNAMPort->unloadedResource);
                        pNAMPort->portDefinition.bPopulated = OMX_FALSE;
                    }
                }
            }
            pNAMComponent->currentState = OMX_StateLoaded;
            break;
        case OMX_StateWaitForResources:
            ret = NAM_OMX_Out_WaitForResource(pOMXComponent);
            pNAMComponent->currentState = OMX_StateLoaded;
            break;
        case OMX_StateExecuting:
        case OMX_StatePause:
        default:
            ret = OMX_ErrorIncorrectStateTransition;
            break;
        }
        break;
    case OMX_StateIdle:
        switch (currentState) {
        case OMX_StateLoaded:
            for (i = 0; i < pNAMComponent->portParam.nPorts; i++) {
                pNAMPort = (pNAMComponent->pNAMPort + i);
                if (pNAMPort == NULL) {
                    ret = OMX_ErrorBadParameter;
                    goto EXIT;
                }
                if (CHECK_PORT_TUNNELED(pNAMPort) && CHECK_PORT_BUFFER_SUPPLIER(pNAMPort)) {
                    if (CHECK_PORT_ENABLED(pNAMPort)) {
                        ret = pNAMComponent->nam_AllocateTunnelBuffer(pNAMPort, i);
                        if (ret!=OMX_ErrorNone)
                            goto EXIT;
                    }
                } else {
                    if (CHECK_PORT_ENABLED(pNAMPort)) {
                        NAM_OSAL_SemaphoreWait(pNAMComponent->pNAMPort[i].loadedResource);
                        pNAMPort->portDefinition.bPopulated = OMX_TRUE;
                    }
                }
            }
            ret = pNAMComponent->nam_dmai_componentInit(pOMXComponent);
            if (ret != OMX_ErrorNone) {
                /*
                 * if (CHECK_PORT_TUNNELED == OMX_TRUE) thenTunnel Buffer Free
                 */
                goto EXIT;
            }
            pNAMComponent->bExitBufferProcessThread = OMX_FALSE;
            NAM_OSAL_SignalCreate(&pNAMComponent->pauseEvent);
            for (i = 0; i < ALL_PORT_NUM; i++) {
                NAM_OSAL_SemaphoreCreate(&pNAMComponent->pNAMPort[i].bufferSemID);
            }
            for (i = 0; i < ALL_PORT_NUM; i++) {
                NAM_OSAL_MutexCreate(&pNAMComponent->namDataBuffer[i].bufferMutex);
            }
            ret = NAM_OSAL_ThreadCreate(&pNAMComponent->hBufferProcess,
                             NAM_OMX_BufferProcessThread,
                             pOMXComponent);
            if (ret != OMX_ErrorNone) {
                /*
                 * if (CHECK_PORT_TUNNELED == OMX_TRUE) thenTunnel Buffer Free
                 */

                NAM_OSAL_SignalTerminate(pNAMComponent->pauseEvent);
                for (i = 0; i < ALL_PORT_NUM; i++) {
                    NAM_OSAL_MutexTerminate(pNAMComponent->namDataBuffer[i].bufferMutex);
                    pNAMComponent->namDataBuffer[i].bufferMutex = NULL;
                }
                for (i = 0; i < ALL_PORT_NUM; i++) {
                    NAM_OSAL_SemaphoreTerminate(pNAMComponent->pNAMPort[i].bufferSemID);
                    pNAMComponent->pNAMPort[i].bufferSemID = NULL;
                }

                ret = OMX_ErrorInsufficientResources;
                goto EXIT;
            }
            pNAMComponent->currentState = OMX_StateIdle;
            break;
        case OMX_StateExecuting:
        case OMX_StatePause:
            NAM_OMX_BufferFlushProcessNoEvent(pOMXComponent, ALL_PORT_INDEX);
            pNAMComponent->currentState = OMX_StateIdle;
            break;
        case OMX_StateWaitForResources:
            pNAMComponent->currentState = OMX_StateIdle;
            break;
        }
        break;
    case OMX_StateExecuting:
        switch (currentState) {
        case OMX_StateLoaded:
            ret = OMX_ErrorIncorrectStateTransition;
            break;
        case OMX_StateIdle:
            for (i = 0; i < pNAMComponent->portParam.nPorts; i++) {
                pNAMPort = &pNAMComponent->pNAMPort[i];
                if (CHECK_PORT_TUNNELED(pNAMPort) && CHECK_PORT_BUFFER_SUPPLIER(pNAMPort) && CHECK_PORT_ENABLED(pNAMPort)) {
                    for (j = 0; j < pNAMPort->tunnelBufferNum; j++) {
                        NAM_OSAL_SemaphorePost(pNAMComponent->pNAMPort[i].bufferSemID);
                    }
                }
            }

            pNAMComponent->transientState = NAM_OMX_TransStateMax;
            pNAMComponent->currentState = OMX_StateExecuting;
            NAM_OSAL_SignalSet(pNAMComponent->pauseEvent);
            break;
        case OMX_StatePause:
            for (i = 0; i < pNAMComponent->portParam.nPorts; i++) {
                pNAMPort = &pNAMComponent->pNAMPort[i];
                if (CHECK_PORT_TUNNELED(pNAMPort) && CHECK_PORT_BUFFER_SUPPLIER(pNAMPort) && CHECK_PORT_ENABLED(pNAMPort)) {
                    OMX_U32 semaValue = 0, cnt = 0;
                    NAM_OSAL_Get_SemaphoreCount(pNAMComponent->pNAMPort[i].bufferSemID, &semaValue);
                    if (NAM_OSAL_GetElemNum(&pNAMPort->bufferQ) > semaValue) {
                        cnt = NAM_OSAL_GetElemNum(&pNAMPort->bufferQ) - semaValue;
                        for (j = 0; j < cnt; j++) {
                            NAM_OSAL_SemaphorePost(pNAMComponent->pNAMPort[i].bufferSemID);
                        }
                    }
                }
            }

            pNAMComponent->currentState = OMX_StateExecuting;
            NAM_OSAL_SignalSet(pNAMComponent->pauseEvent);
            break;
        case OMX_StateWaitForResources:
            ret = OMX_ErrorIncorrectStateTransition;
            break;
        }
        break;
    case OMX_StatePause:
        switch (currentState) {
        case OMX_StateLoaded:
            ret = OMX_ErrorIncorrectStateTransition;
            break;
        case OMX_StateIdle:
            pNAMComponent->currentState = OMX_StatePause;
            break;
        case OMX_StateExecuting:
            pNAMComponent->currentState = OMX_StatePause;
            break;
        case OMX_StateWaitForResources:
            ret = OMX_ErrorIncorrectStateTransition;
            break;
        }
        break;
    case OMX_StateWaitForResources:
        switch (currentState) {
        case OMX_StateLoaded:
            ret = NAM_OMX_In_WaitForResource(pOMXComponent);
            pNAMComponent->currentState = OMX_StateWaitForResources;
            break;
        case OMX_StateIdle:
        case OMX_StateExecuting:
        case OMX_StatePause:
            ret = OMX_ErrorIncorrectStateTransition;
            break;
        }
        break;
    }

EXIT:
    if (ret == OMX_ErrorNone) {
        if (pNAMComponent->pCallbacks != NULL) {
            pNAMComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
            pNAMComponent->callbackData,
            OMX_EventCmdComplete, OMX_CommandStateSet,
            destState, NULL);
        }
    } else {
        if (pNAMComponent->pCallbacks != NULL) {
            pNAMComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
            pNAMComponent->callbackData,
            OMX_EventError, ret, 0, NULL);
        }
    }
    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE NAM_OMX_MessageHandlerThread(OMX_PTR threadData)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    NAM_OMX_BANAMOMPONENT *pNAMComponent = NULL;
    NAM_OMX_MESSAGE       *message = NULL;
    OMX_U32                messageType = 0, portIndex = 0;

    FunctionIn();

    if (threadData == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pOMXComponent = (OMX_COMPONENTTYPE *)threadData;
    ret = NAM_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    pNAMComponent = (NAM_OMX_BANAMOMPONENT *)pOMXComponent->pComponentPrivate;

    while (pNAMComponent->bExitMessageHandlerThread != OMX_TRUE) {
        NAM_OSAL_SemaphoreWait(pNAMComponent->msgSemaphoreHandle);
        message = (NAM_OMX_MESSAGE *)NAM_OSAL_Dequeue(&pNAMComponent->messageQ);
        if (message != NULL) {
            messageType = message->messageType;
            switch (messageType) {
            case OMX_CommandStateSet:
                ret = NAM_OMX_ComponentStateSet(pOMXComponent, message->messageParam);
                break;
            case OMX_CommandFlush:
                ret = NAM_OMX_BufferFlushProcess(pOMXComponent, message->messageParam);
                break;
            case OMX_CommandPortDisable:
                ret = NAM_OMX_PortDisableProcess(pOMXComponent, message->messageParam);
                break;
            case OMX_CommandPortEnable:
                ret = NAM_OMX_PortEnableProcess(pOMXComponent, message->messageParam);
                break;
            case OMX_CommandMarkBuffer:
                portIndex = message->messageParam;
                pNAMComponent->pNAMPort[portIndex].markType.hMarkTargetComponent = ((OMX_MARKTYPE *)message->pCmdData)->hMarkTargetComponent;
                pNAMComponent->pNAMPort[portIndex].markType.pMarkData            = ((OMX_MARKTYPE *)message->pCmdData)->pMarkData;
                break;
            case (OMX_COMMANDTYPE)NAM_OMX_CommandComponentDeInit:
                pNAMComponent->bExitMessageHandlerThread = OMX_TRUE;
                break;
            default:
                break;
            }
            NAM_OSAL_Free(message);
            message = NULL;
        }
    }

    NAM_OSAL_ThreadExit(NULL);

EXIT:
    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE NAM_StateSet(NAM_OMX_BANAMOMPONENT *pNAMComponent, OMX_U32 nParam)
{
    OMX_U32 destState = nParam;
    OMX_U32 i = 0;

    if ((destState == OMX_StateIdle) && (pNAMComponent->currentState == OMX_StateLoaded)) {
        pNAMComponent->transientState = NAM_OMX_TransStateLoadedToIdle;
        for(i = 0; i < pNAMComponent->portParam.nPorts; i++) {
            pNAMComponent->pNAMPort[i].portState = OMX_StateIdle;
        }
        NAM_OSAL_Log(NAM_LOG_TRACE, "to OMX_StateIdle");
    } else if ((destState == OMX_StateLoaded) && (pNAMComponent->currentState == OMX_StateIdle)) {
        pNAMComponent->transientState = NAM_OMX_TransStateIdleToLoaded;
        for (i = 0; i < pNAMComponent->portParam.nPorts; i++) {
            pNAMComponent->pNAMPort[i].portState = OMX_StateLoaded;
        }
        NAM_OSAL_Log(NAM_LOG_TRACE, "to OMX_StateLoaded");
    } else if ((destState == OMX_StateIdle) && (pNAMComponent->currentState == OMX_StateExecuting)) {
        pNAMComponent->transientState = NAM_OMX_TransStateExecutingToIdle;
        NAM_OSAL_Log(NAM_LOG_TRACE, "to OMX_StateIdle");
    } else if ((destState == OMX_StateExecuting) && (pNAMComponent->currentState == OMX_StateIdle)) {
        pNAMComponent->transientState = NAM_OMX_TransStateIdleToExecuting;
        NAM_OSAL_Log(NAM_LOG_TRACE, "to OMX_StateExecuting");
    } else if (destState == OMX_StateInvalid) {
        for (i = 0; i < pNAMComponent->portParam.nPorts; i++) {
            pNAMComponent->pNAMPort[i].portState = OMX_StateInvalid;
        }
    }

    return OMX_ErrorNone;
}

static OMX_ERRORTYPE NAM_SetPortFlush(NAM_OMX_BANAMOMPONENT *pNAMComponent, OMX_U32 nParam)
{
    OMX_ERRORTYPE     ret = OMX_ErrorNone;
    NAM_OMX_BASEPORT *pNAMPort = NULL;
    OMX_U32           portIndex = nParam;
    OMX_U16           i = 0, cnt = 0, index = 0;


    if ((pNAMComponent->currentState == OMX_StateExecuting) ||
        (pNAMComponent->currentState == OMX_StatePause)) {
        if ((portIndex != ALL_PORT_INDEX) &&
           ((OMX_S32)portIndex >= (OMX_S32)pNAMComponent->portParam.nPorts)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        /*********************
        *    need flush event set ?????
        **********************/
        cnt = (portIndex == ALL_PORT_INDEX ) ? ALL_PORT_NUM : 1;
        for (i = 0; i < cnt; i++) {
            if (portIndex == ALL_PORT_INDEX)
                index = i;
            else
                index = portIndex;
            pNAMComponent->pNAMPort[index].bIsPortFlushed = OMX_TRUE;
        }
    } else {
        ret = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }
    ret = OMX_ErrorNone;

EXIT:
    return ret;
}

static OMX_ERRORTYPE NAM_SetPortEnable(NAM_OMX_BANAMOMPONENT *pNAMComponent, OMX_U32 nParam)
{
    OMX_ERRORTYPE     ret = OMX_ErrorNone;
    NAM_OMX_BASEPORT *pNAMPort = NULL;
    OMX_U32           portIndex = nParam;
    OMX_U16           i = 0, cnt = 0;

    FunctionIn();

    if ((portIndex != ALL_PORT_INDEX) &&
        ((OMX_S32)portIndex >= (OMX_S32)pNAMComponent->portParam.nPorts)) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    if (portIndex == ALL_PORT_INDEX) {
        for (i = 0; i < pNAMComponent->portParam.nPorts; i++) {
            pNAMPort = &pNAMComponent->pNAMPort[i];
            if (CHECK_PORT_ENABLED(pNAMPort)) {
                ret = OMX_ErrorIncorrectStateOperation;
                goto EXIT;
            } else {
                pNAMPort->portState = OMX_StateIdle;
            }
        }
    } else {
        pNAMPort = &pNAMComponent->pNAMPort[portIndex];
        if (CHECK_PORT_ENABLED(pNAMPort)) {
            ret = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
        } else {
            pNAMPort->portState = OMX_StateIdle;
        }
    }
    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;

}

static OMX_ERRORTYPE NAM_SetPortDisable(NAM_OMX_BANAMOMPONENT *pNAMComponent, OMX_U32 nParam)
{
    OMX_ERRORTYPE     ret = OMX_ErrorNone;
    NAM_OMX_BASEPORT *pNAMPort = NULL;
    OMX_U32           portIndex = nParam;
    OMX_U16           i = 0, cnt = 0;

    FunctionIn();

    if ((portIndex != ALL_PORT_INDEX) &&
        ((OMX_S32)portIndex >= (OMX_S32)pNAMComponent->portParam.nPorts)) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    if (portIndex == ALL_PORT_INDEX) {
        for (i = 0; i < pNAMComponent->portParam.nPorts; i++) {
            pNAMPort = &pNAMComponent->pNAMPort[i];
            if (!CHECK_PORT_ENABLED(pNAMPort)) {
                ret = OMX_ErrorIncorrectStateOperation;
                goto EXIT;
            }
            pNAMPort->portState = OMX_StateLoaded;
            pNAMPort->bIsPortDisabled = OMX_TRUE;
        }
    } else {
        pNAMPort = &pNAMComponent->pNAMPort[portIndex];
        pNAMPort->portState = OMX_StateLoaded;
        pNAMPort->bIsPortDisabled = OMX_TRUE;
    }
    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE NAM_SetMarkBuffer(NAM_OMX_BANAMOMPONENT *pNAMComponent, OMX_U32 nParam)
{
    OMX_ERRORTYPE     ret = OMX_ErrorNone;
    NAM_OMX_BASEPORT *pNAMPort = NULL;
    OMX_U32           portIndex = nParam;
    OMX_U16           i = 0, cnt = 0;


    if (nParam >= pNAMComponent->portParam.nPorts) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    if ((pNAMComponent->currentState == OMX_StateExecuting) ||
        (pNAMComponent->currentState == OMX_StatePause)) {
        ret = OMX_ErrorNone;
    } else {
        ret = OMX_ErrorIncorrectStateOperation;
    }

EXIT:
    return ret;
}

static OMX_ERRORTYPE NAM_OMX_CommandQueue(
    NAM_OMX_BANAMOMPONENT *pNAMComponent,
    OMX_COMMANDTYPE        Cmd,
    OMX_U32                nParam,
    OMX_PTR                pCmdData)
{
    OMX_ERRORTYPE    ret = OMX_ErrorNone;
    NAM_OMX_MESSAGE *command = (NAM_OMX_MESSAGE *)NAM_OSAL_Malloc(sizeof(NAM_OMX_MESSAGE));

    if (command == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    command->messageType  = (OMX_U32)Cmd;
    command->messageParam = nParam;
    command->pCmdData     = pCmdData;

    ret = NAM_OSAL_Queue(&pNAMComponent->messageQ, (void *)command);
    if (ret != 0) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }
    ret = NAM_OSAL_SemaphorePost(pNAMComponent->msgSemaphoreHandle);

EXIT:
    return ret;
}

OMX_ERRORTYPE NAM_OMX_SendCommand(
    OMX_IN OMX_HANDLETYPE  hComponent,
    OMX_IN OMX_COMMANDTYPE Cmd,
    OMX_IN OMX_U32         nParam,
    OMX_IN OMX_PTR         pCmdData)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    NAM_OMX_BANAMOMPONENT *pNAMComponent = NULL;
    NAM_OMX_MESSAGE       *message = NULL;

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
    pNAMComponent = (NAM_OMX_BANAMOMPONENT *)pOMXComponent->pComponentPrivate;

    if (pNAMComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (Cmd) {
    case OMX_CommandStateSet :
        NAM_OSAL_Log(NAM_LOG_TRACE, "Command: OMX_CommandStateSet");
        NAM_StateSet(pNAMComponent, nParam);
        break;
    case OMX_CommandFlush :
        NAM_OSAL_Log(NAM_LOG_TRACE, "Command: OMX_CommandFlush");
        ret = NAM_SetPortFlush(pNAMComponent, nParam);
        if (ret != OMX_ErrorNone)
            goto EXIT;
        break;
    case OMX_CommandPortDisable :
        NAM_OSAL_Log(NAM_LOG_TRACE, "Command: OMX_CommandPortDisable");
        ret = NAM_SetPortDisable(pNAMComponent, nParam);
        if (ret != OMX_ErrorNone)
            goto EXIT;
        break;
    case OMX_CommandPortEnable :
        NAM_OSAL_Log(NAM_LOG_TRACE, "Command: OMX_CommandPortEnable");
        ret = NAM_SetPortEnable(pNAMComponent, nParam);
        if (ret != OMX_ErrorNone)
            goto EXIT;
        break;
    case OMX_CommandMarkBuffer :
        NAM_OSAL_Log(NAM_LOG_TRACE, "Command: OMX_CommandMarkBuffer");
        ret = NAM_SetMarkBuffer(pNAMComponent, nParam);
        if (ret != OMX_ErrorNone)
            goto EXIT;
        break;
/*
    case NAM_CommandFillBuffer :
    case NAM_CommandEmptyBuffer :
    case NAM_CommandDeInit :
*/
    default:
        break;
    }

    ret = NAM_OMX_CommandQueue(pNAMComponent, Cmd, nParam, pCmdData);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_OMX_GetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nParamIndex,
    OMX_INOUT OMX_PTR     ComponentParameterStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    NAM_OMX_BANAMOMPONENT *pNAMComponent = NULL;

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
    pNAMComponent = (NAM_OMX_BANAMOMPONENT *)pOMXComponent->pComponentPrivate;

    if (ComponentParameterStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pNAMComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nParamIndex) {
    case (OMX_INDEXTYPE)OMX_COMPONENT_CAPABILITY_TYPE_INDEX:
    {
        /* For Android PV OpenCORE */
        OMXComponentCapabilityFlagsType *capabilityFlags = (OMXComponentCapabilityFlagsType *)ComponentParameterStructure;
        NAM_OSAL_Memcpy(capabilityFlags, &pNAMComponent->capabilityFlags, sizeof(OMXComponentCapabilityFlagsType));
    }
        break;
    case OMX_IndexParamAudioInit:
    case OMX_IndexParamVideoInit:
    case OMX_IndexParamImageInit:
    case OMX_IndexParamOtherInit:
    {
        OMX_PORT_PARAM_TYPE *portParam = (OMX_PORT_PARAM_TYPE *)ComponentParameterStructure;
        ret = NAM_OMX_Check_SizeVersion(portParam, sizeof(OMX_PORT_PARAM_TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }
        portParam->nPorts         = 0;
        portParam->nStartPortNumber     = 0;
    }
        break;
    case OMX_IndexParamPortDefinition:
    {
        OMX_PARAM_PORTDEFINITIONTYPE *portDefinition = (OMX_PARAM_PORTDEFINITIONTYPE *)ComponentParameterStructure;
        OMX_U32                       portIndex = portDefinition->nPortIndex;
        NAM_OMX_BASEPORT             *pNAMPort;

        if (portIndex >= pNAMComponent->portParam.nPorts) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }
        ret = NAM_OMX_Check_SizeVersion(portDefinition, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        pNAMPort = &pNAMComponent->pNAMPort[portIndex];
        NAM_OSAL_Memcpy(portDefinition, &pNAMPort->portDefinition, portDefinition->nSize);
    }
        break;
    case OMX_IndexParamPriorityMgmt:
    {
        OMX_PRIORITYMGMTTYPE *compPriority = (OMX_PRIORITYMGMTTYPE *)ComponentParameterStructure;

        ret = NAM_OMX_Check_SizeVersion(compPriority, sizeof(OMX_PRIORITYMGMTTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        compPriority->nGroupID       = pNAMComponent->compPriority.nGroupID;
        compPriority->nGroupPriority = pNAMComponent->compPriority.nGroupPriority;
    }
        break;

    case OMX_IndexParamCompBufferSupplier:
    {
        OMX_PARAM_BUFFERSUPPLIERTYPE *bufferSupplier = (OMX_PARAM_BUFFERSUPPLIERTYPE *)ComponentParameterStructure;
        OMX_U32                       portIndex = bufferSupplier->nPortIndex;
        NAM_OMX_BASEPORT             *pNAMPort;

        if ((pNAMComponent->currentState == OMX_StateLoaded) ||
            (pNAMComponent->currentState == OMX_StateWaitForResources)) {
            if (portIndex >= pNAMComponent->portParam.nPorts) {
                ret = OMX_ErrorBadPortIndex;
                goto EXIT;
            }
            ret = NAM_OMX_Check_SizeVersion(bufferSupplier, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));
            if (ret != OMX_ErrorNone) {
                goto EXIT;
            }

            pNAMPort = &pNAMComponent->pNAMPort[portIndex];


            if (pNAMPort->portDefinition.eDir == OMX_DirInput) {
                if (CHECK_PORT_BUFFER_SUPPLIER(pNAMPort)) {
                    bufferSupplier->eBufferSupplier = OMX_BufferSupplyInput;
                } else if (CHECK_PORT_TUNNELED(pNAMPort)) {
                    bufferSupplier->eBufferSupplier = OMX_BufferSupplyOutput;
                } else {
                    bufferSupplier->eBufferSupplier = OMX_BufferSupplyUnspecified;
                }
            } else {
                if (CHECK_PORT_BUFFER_SUPPLIER(pNAMPort)) {
                    bufferSupplier->eBufferSupplier = OMX_BufferSupplyOutput;
                } else if (CHECK_PORT_TUNNELED(pNAMPort)) {
                    bufferSupplier->eBufferSupplier = OMX_BufferSupplyInput;
                } else {
                    bufferSupplier->eBufferSupplier = OMX_BufferSupplyUnspecified;
                }
            }
        }
        else
        {
            ret = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
        }
    }
        break;
    default:
    {
        ret = OMX_ErrorUnsupportedIndex;
        goto EXIT;
    }
        break;
    }

    ret = OMX_ErrorNone;

EXIT:

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_OMX_SetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        ComponentParameterStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    NAM_OMX_BANAMOMPONENT *pNAMComponent = NULL;

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
    pNAMComponent = (NAM_OMX_BANAMOMPONENT *)pOMXComponent->pComponentPrivate;

    if (ComponentParameterStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pNAMComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nIndex) {
    case OMX_IndexParamAudioInit:
    case OMX_IndexParamVideoInit:
    case OMX_IndexParamImageInit:
    case OMX_IndexParamOtherInit:
    {
        OMX_PORT_PARAM_TYPE *portParam = (OMX_PORT_PARAM_TYPE *)ComponentParameterStructure;
        ret = NAM_OMX_Check_SizeVersion(portParam, sizeof(OMX_PORT_PARAM_TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if ((pNAMComponent->currentState != OMX_StateLoaded) &&
            (pNAMComponent->currentState != OMX_StateWaitForResources)) {
            ret = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
        }
        ret = OMX_ErrorUndefined;
        /* NAM_OSAL_Memcpy(&pNAMComponent->portParam, portParam, sizeof(OMX_PORT_PARAM_TYPE)); */
    }
        break;
    case OMX_IndexParamPortDefinition:
    {
        OMX_PARAM_PORTDEFINITIONTYPE *portDefinition = (OMX_PARAM_PORTDEFINITIONTYPE *)ComponentParameterStructure;
        OMX_U32               portIndex = portDefinition->nPortIndex;
        NAM_OMX_BASEPORT         *pNAMPort;

        if (portIndex >= pNAMComponent->portParam.nPorts) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }
        ret = NAM_OMX_Check_SizeVersion(portDefinition, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        pNAMPort = &pNAMComponent->pNAMPort[portIndex];

        if ((pNAMComponent->currentState != OMX_StateLoaded) && (pNAMComponent->currentState != OMX_StateWaitForResources)) {
            if (pNAMPort->portDefinition.bEnabled == OMX_TRUE) {
                ret = OMX_ErrorIncorrectStateOperation;
                goto EXIT;
            }
        }
        if (portDefinition->nBufferCountActual < pNAMPort->portDefinition.nBufferCountMin) {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }

        NAM_OSAL_Memcpy(&pNAMPort->portDefinition, portDefinition, portDefinition->nSize);
    }
        break;
    case OMX_IndexParamPriorityMgmt:
    {
        OMX_PRIORITYMGMTTYPE *compPriority = (OMX_PRIORITYMGMTTYPE *)ComponentParameterStructure;

        if ((pNAMComponent->currentState != OMX_StateLoaded) &&
            (pNAMComponent->currentState != OMX_StateWaitForResources)) {
            ret = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
        }

        ret = NAM_OMX_Check_SizeVersion(compPriority, sizeof(OMX_PRIORITYMGMTTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        pNAMComponent->compPriority.nGroupID = compPriority->nGroupID;
        pNAMComponent->compPriority.nGroupPriority = compPriority->nGroupPriority;
    }
        break;
    case OMX_IndexParamCompBufferSupplier:
    {
        OMX_PARAM_BUFFERSUPPLIERTYPE *bufferSupplier = (OMX_PARAM_BUFFERSUPPLIERTYPE *)ComponentParameterStructure;
        OMX_U32               portIndex = bufferSupplier->nPortIndex;
        NAM_OMX_BASEPORT      *pNAMPort = &pNAMComponent->pNAMPort[portIndex];

        if ((pNAMComponent->currentState != OMX_StateLoaded) && (pNAMComponent->currentState != OMX_StateWaitForResources)) {
            if (pNAMPort->portDefinition.bEnabled == OMX_TRUE) {
                ret = OMX_ErrorIncorrectStateOperation;
                goto EXIT;
            }
        }

        if (portIndex >= pNAMComponent->portParam.nPorts) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }
        ret = NAM_OMX_Check_SizeVersion(bufferSupplier, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (bufferSupplier->eBufferSupplier == OMX_BufferSupplyUnspecified) {
            ret = OMX_ErrorNone;
            goto EXIT;
        }
        if (CHECK_PORT_TUNNELED(pNAMPort) == 0) {
            ret = OMX_ErrorNone; /*OMX_ErrorNone ?????*/
            goto EXIT;
        }

        if (pNAMPort->portDefinition.eDir == OMX_DirInput) {
            if (bufferSupplier->eBufferSupplier == OMX_BufferSupplyInput) {
                /*
                if (CHECK_PORT_BUFFER_SUPPLIER(pNAMPort)) {
                    ret = OMX_ErrorNone;
                }
                */
                pNAMPort->tunnelFlags |= NAM_TUNNEL_IS_SUPPLIER;
                bufferSupplier->nPortIndex = pNAMPort->tunneledPort;
                ret = OMX_SetParameter(pNAMPort->tunneledComponent, OMX_IndexParamCompBufferSupplier, bufferSupplier);
                goto EXIT;
            } else if (bufferSupplier->eBufferSupplier == OMX_BufferSupplyOutput) {
                ret = OMX_ErrorNone;
                if (CHECK_PORT_BUFFER_SUPPLIER(pNAMPort)) {
                    pNAMPort->tunnelFlags &= ~NAM_TUNNEL_IS_SUPPLIER;
                    bufferSupplier->nPortIndex = pNAMPort->tunneledPort;
                    ret = OMX_SetParameter(pNAMPort->tunneledComponent, OMX_IndexParamCompBufferSupplier, bufferSupplier);
                }
                goto EXIT;
            }
        } else if (pNAMPort->portDefinition.eDir == OMX_DirOutput) {
            if (bufferSupplier->eBufferSupplier == OMX_BufferSupplyInput) {
                ret = OMX_ErrorNone;
                if (CHECK_PORT_BUFFER_SUPPLIER(pNAMPort)) {
                    pNAMPort->tunnelFlags &= ~NAM_TUNNEL_IS_SUPPLIER;
                    ret = OMX_ErrorNone;
                }
                goto EXIT;
            } else if (bufferSupplier->eBufferSupplier == OMX_BufferSupplyOutput) {
                /*
                if (CHECK_PORT_BUFFER_SUPPLIER(pNAMPort)) {
                    ret = OMX_ErrorNone;
                }
                */
                pNAMPort->tunnelFlags |= NAM_TUNNEL_IS_SUPPLIER;
                ret = OMX_ErrorNone;
                goto EXIT;
            }
        }
    }
        break;
    default:
    {
        ret = OMX_ErrorUnsupportedIndex;
        goto EXIT;
    }
        break;
    }

    ret = OMX_ErrorNone;

EXIT:

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_OMX_GetConfig(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_INOUT OMX_PTR     pComponentConfigStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    NAM_OMX_BANAMOMPONENT *pNAMComponent = NULL;

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
    pNAMComponent = (NAM_OMX_BANAMOMPONENT *)pOMXComponent->pComponentPrivate;

    if (pComponentConfigStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pNAMComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }
    ret = OMX_ErrorUnsupportedIndex;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_OMX_SetConfig(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        pComponentConfigStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    NAM_OMX_BANAMOMPONENT *pNAMComponent = NULL;

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
    pNAMComponent = (NAM_OMX_BANAMOMPONENT *)pOMXComponent->pComponentPrivate;

    if (pComponentConfigStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pNAMComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }
    ret = OMX_ErrorUnsupportedIndex;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_OMX_GetExtensionIndex(
    OMX_IN OMX_HANDLETYPE  hComponent,
    OMX_IN OMX_STRING      cParameterName,
    OMX_OUT OMX_INDEXTYPE *pIndexType)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    NAM_OMX_BANAMOMPONENT *pNAMComponent = NULL;

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
    pNAMComponent = (NAM_OMX_BANAMOMPONENT *)pOMXComponent->pComponentPrivate;

    if ((cParameterName == NULL) || (pIndexType == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pNAMComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    ret = OMX_ErrorBadParameter;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_OMX_SetCallbacks (
    OMX_IN OMX_HANDLETYPE    hComponent,
    OMX_IN OMX_CALLBACKTYPE* pCallbacks,
    OMX_IN OMX_PTR           pAppData)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    NAM_OMX_BANAMOMPONENT *pNAMComponent = NULL;

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
    pNAMComponent = (NAM_OMX_BANAMOMPONENT *)pOMXComponent->pComponentPrivate;

    if (pCallbacks == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pNAMComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }
    if (pNAMComponent->currentState != OMX_StateLoaded) {
        ret = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    pNAMComponent->pCallbacks = pCallbacks;
    pNAMComponent->callbackData = pAppData;

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_OMX_UseEGLImage(
    OMX_IN OMX_HANDLETYPE            hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE **ppBufferHdr,
    OMX_IN OMX_U32                   nPortIndex,
    OMX_IN OMX_PTR                   pAppPrivate,
    OMX_IN void                     *eglImage)
{
    return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE NAM_OMX_BaseComponent_Constructor(
    OMX_IN OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent;
    NAM_OMX_BANAMOMPONENT *pNAMComponent = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        NAM_OSAL_Log(NAM_LOG_ERROR, "OMX_ErrorBadParameter, Line:%d", __LINE__);
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    pNAMComponent = NAM_OSAL_Malloc(sizeof(NAM_OMX_BANAMOMPONENT));
    if (pNAMComponent == NULL) {
        ret = OMX_ErrorInsufficientResources;
        NAM_OSAL_Log(NAM_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    NAM_OSAL_Memset(pNAMComponent, 0, sizeof(NAM_OMX_BANAMOMPONENT));
    pOMXComponent->pComponentPrivate = (OMX_PTR)pNAMComponent;

    ret = NAM_OSAL_SemaphoreCreate(&pNAMComponent->msgSemaphoreHandle);
    if (ret != OMX_ErrorNone) {
        ret = OMX_ErrorInsufficientResources;
        NAM_OSAL_Log(NAM_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    ret = NAM_OSAL_MutexCreate(&pNAMComponent->compMutex);
    if (ret != OMX_ErrorNone) {
        ret = OMX_ErrorInsufficientResources;
        NAM_OSAL_Log(NAM_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }

    pNAMComponent->bExitMessageHandlerThread = OMX_FALSE;
    NAM_OSAL_QueueCreate(&pNAMComponent->messageQ);
    ret = NAM_OSAL_ThreadCreate(&pNAMComponent->hMessageHandler, NAM_OMX_MessageHandlerThread, pOMXComponent);
    if (ret != OMX_ErrorNone) {
        ret = OMX_ErrorInsufficientResources;
        NAM_OSAL_Log(NAM_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }

    pOMXComponent->GetComponentVersion = &NAM_OMX_GetComponentVersion;
    pOMXComponent->SendCommand         = &NAM_OMX_SendCommand;
    pOMXComponent->GetExtensionIndex   = &NAM_OMX_GetExtensionIndex;
    pOMXComponent->GetState            = &NAM_OMX_GetState;
    pOMXComponent->SetCallbacks        = &NAM_OMX_SetCallbacks;
    pOMXComponent->UseEGLImage         = &NAM_OMX_UseEGLImage;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_OMX_BaseComponent_Destructor(
    OMX_IN OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    NAM_OMX_BANAMOMPONENT *pNAMComponent = NULL;
    OMX_U32                semaValue = 0;

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
    pNAMComponent = (NAM_OMX_BANAMOMPONENT *)pOMXComponent->pComponentPrivate;

    NAM_OMX_CommandQueue(pNAMComponent, NAM_OMX_CommandComponentDeInit, 0, NULL);
    NAM_OSAL_SleepMillinam(0);
    NAM_OSAL_Get_SemaphoreCount(pNAMComponent->msgSemaphoreHandle, &semaValue);
    if (semaValue == 0)
        NAM_OSAL_SemaphorePost(pNAMComponent->msgSemaphoreHandle);
    NAM_OSAL_SemaphorePost(pNAMComponent->msgSemaphoreHandle);

    NAM_OSAL_ThreadTerminate(pNAMComponent->hMessageHandler);
    pNAMComponent->hMessageHandler = NULL;

    NAM_OSAL_MutexTerminate(pNAMComponent->compMutex);
    pNAMComponent->compMutex = NULL;
    NAM_OSAL_SemaphoreTerminate(pNAMComponent->msgSemaphoreHandle);
    pNAMComponent->msgSemaphoreHandle = NULL;
    NAM_OSAL_QueueTerminate(&pNAMComponent->messageQ);

    NAM_OSAL_Free(pNAMComponent);
    pNAMComponent = NULL;

    ret = OMX_ErrorNone;
EXIT:
    FunctionOut();

    return ret;
}

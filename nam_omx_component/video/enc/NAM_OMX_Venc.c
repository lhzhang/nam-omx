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
 * @file        NAM_OMX_Venc.c
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 *              Yunji Kim (yunji.kim@samsung.com)
 * @version     1.0
 * @history
 *   2010.7.15 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "NAM_OMX_Macros.h"
#include "NAM_OSAL_Event.h"
#include "NAM_OMX_Venc.h"
#include "NAM_OMX_Basecomponent.h"
#include "NAM_OSAL_Thread.h"
#include "color_space_convertor.h"

#undef  NAM_LOG_TAG
#define NAM_LOG_TAG    "NAM_VIDEO_ENC"
#define NAM_LOG_OFF
#include "NAM_OSAL_Log.h"


inline void NAM_UpdateFrameSize(OMX_COMPONENTTYPE *pOMXComponent)
{
    NAM_OMX_BASECOMPONENT *pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    NAM_OMX_BASEPORT      *namInputPort = &pNAMComponent->pNAMPort[INPUT_PORT_INDEX];
    NAM_OMX_BASEPORT      *namOutputPort = &pNAMComponent->pNAMPort[OUTPUT_PORT_INDEX];

    if ((namOutputPort->portDefinition.format.video.nFrameWidth !=
            namInputPort->portDefinition.format.video.nFrameWidth) ||
        (namOutputPort->portDefinition.format.video.nFrameHeight !=
            namInputPort->portDefinition.format.video.nFrameHeight)) {
        OMX_U32 width = 0, height = 0;

        namOutputPort->portDefinition.format.video.nFrameWidth =
            namInputPort->portDefinition.format.video.nFrameWidth;
        namOutputPort->portDefinition.format.video.nFrameHeight =
            namInputPort->portDefinition.format.video.nFrameHeight;
        width = namOutputPort->portDefinition.format.video.nStride =
            namInputPort->portDefinition.format.video.nStride;
        height = namOutputPort->portDefinition.format.video.nSliceHeight =
            namInputPort->portDefinition.format.video.nSliceHeight;

        switch(namOutputPort->portDefinition.format.video.eColorFormat) {
        case OMX_COLOR_FormatYUV420Planar:
        case OMX_COLOR_FormatYUV420SemiPlanar:
            if (width && height)
                namOutputPort->portDefinition.nBufferSize = (width * height * 3) / 2;
            break;
        default:
            if (width && height)
                namOutputPort->portDefinition.nBufferSize = width * height * 2;
            break;
        }
    }

  return ;
}

OMX_ERRORTYPE NAM_OMX_UseBuffer(
    OMX_IN OMX_HANDLETYPE            hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE **ppBufferHdr,
    OMX_IN OMX_U32                   nPortIndex,
    OMX_IN OMX_PTR                   pAppPrivate,
    OMX_IN OMX_U32                   nSizeBytes,
    OMX_IN OMX_U8                   *pBuffer)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    NAM_OMX_BASECOMPONENT *pNAMComponent = NULL;
    NAM_OMX_BASEPORT      *pNAMPort = NULL;
    OMX_BUFFERHEADERTYPE  *temp_bufferHeader = NULL;
    int                    i = 0;

    FunctionIn();

    if (hComponent == NULL)    {
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

    pNAMPort = &pNAMComponent->pNAMPort[nPortIndex];
    if (nPortIndex >= pNAMComponent->portParam.nPorts) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }
    if (pNAMPort->portState != OMX_StateIdle) {
        ret = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    if (CHECK_PORT_TUNNELED(pNAMPort) && CHECK_PORT_BUFFER_SUPPLIER(pNAMPort)) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    temp_bufferHeader = (OMX_BUFFERHEADERTYPE *)NAM_OSAL_Malloc(sizeof(OMX_BUFFERHEADERTYPE));
    if (temp_bufferHeader == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    NAM_OSAL_Memset(temp_bufferHeader, 0, sizeof(OMX_BUFFERHEADERTYPE));

    for (i = 0; i < pNAMPort->portDefinition.nBufferCountActual; i++) {
        if (pNAMPort->bufferStateAllocate[i] == BUFFER_STATE_FREE) {
            pNAMPort->bufferHeader[i] = temp_bufferHeader;
            pNAMPort->bufferStateAllocate[i] = (BUFFER_STATE_ASSIGNED | HEADER_STATE_ALLOCATED);
            INIT_SET_SIZE_VERSION(temp_bufferHeader, OMX_BUFFERHEADERTYPE);
            temp_bufferHeader->pBuffer        = pBuffer;
            temp_bufferHeader->nAllocLen        = nSizeBytes;
            temp_bufferHeader->pAppPrivate        = pAppPrivate;
            if (nPortIndex == INPUT_PORT_INDEX)
                temp_bufferHeader->nInputPortIndex = INPUT_PORT_INDEX;
            else
                temp_bufferHeader->nOutputPortIndex = OUTPUT_PORT_INDEX;

            pNAMPort->assignedBufferNum++;
            if (pNAMPort->assignedBufferNum == pNAMPort->portDefinition.nBufferCountActual) {
                pNAMPort->portDefinition.bPopulated = OMX_TRUE;
                /* NAM_OSAL_MutexLock(pNAMComponent->compMutex); */
                NAM_OSAL_SemaphorePost(pNAMPort->loadedResource);
                /* NAM_OSAL_MutexUnlock(pNAMComponent->compMutex); */
            }
            *ppBufferHdr = temp_bufferHeader;
            ret = OMX_ErrorNone;
            goto EXIT;
        }
    }

    NAM_OSAL_Free(temp_bufferHeader);
    ret = OMX_ErrorInsufficientResources;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_OMX_AllocateBuffer(
    OMX_IN OMX_HANDLETYPE            hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE **ppBuffer,
    OMX_IN OMX_U32                   nPortIndex,
    OMX_IN OMX_PTR                   pAppPrivate,
    OMX_IN OMX_U32                   nSizeBytes)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    NAM_OMX_BASECOMPONENT *pNAMComponent = NULL;
    NAM_OMX_BASEPORT      *pNAMPort = NULL;
    OMX_BUFFERHEADERTYPE  *temp_bufferHeader = NULL;
    OMX_U8                *temp_buffer = NULL;
    int                    i = 0;

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

    pNAMPort = &pNAMComponent->pNAMPort[nPortIndex];
    if (nPortIndex >= pNAMComponent->portParam.nPorts) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }
/*
    if (pNAMPort->portState != OMX_StateIdle ) {
        ret = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }
*/
    if (CHECK_PORT_TUNNELED(pNAMPort) && CHECK_PORT_BUFFER_SUPPLIER(pNAMPort)) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    temp_buffer = NAM_OSAL_Malloc(sizeof(OMX_U8) * nSizeBytes);
    if (temp_buffer == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    temp_bufferHeader = (OMX_BUFFERHEADERTYPE *)NAM_OSAL_Malloc(sizeof(OMX_BUFFERHEADERTYPE));
    if (temp_bufferHeader == NULL) {
        NAM_OSAL_Free(temp_buffer);
        temp_buffer = NULL;
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    NAM_OSAL_Memset(temp_bufferHeader, 0, sizeof(OMX_BUFFERHEADERTYPE));

    for (i = 0; i < pNAMPort->portDefinition.nBufferCountActual; i++) {
        if (pNAMPort->bufferStateAllocate[i] == BUFFER_STATE_FREE) {
            pNAMPort->bufferHeader[i] = temp_bufferHeader;
            pNAMPort->bufferStateAllocate[i] = (BUFFER_STATE_ALLOCATED | HEADER_STATE_ALLOCATED);
            INIT_SET_SIZE_VERSION(temp_bufferHeader, OMX_BUFFERHEADERTYPE);
            temp_bufferHeader->pBuffer        = temp_buffer;
            temp_bufferHeader->nAllocLen        = nSizeBytes;
            temp_bufferHeader->pAppPrivate        = pAppPrivate;
            if (nPortIndex == INPUT_PORT_INDEX)
                temp_bufferHeader->nInputPortIndex = INPUT_PORT_INDEX;
            else
                temp_bufferHeader->nOutputPortIndex = OUTPUT_PORT_INDEX;
            pNAMPort->assignedBufferNum++;
            if (pNAMPort->assignedBufferNum == pNAMPort->portDefinition.nBufferCountActual) {
                pNAMPort->portDefinition.bPopulated = OMX_TRUE;
                /* NAM_OSAL_MutexLock(pNAMComponent->compMutex); */
                NAM_OSAL_SemaphorePost(pNAMPort->loadedResource);
                /* NAM_OSAL_MutexUnlock(pNAMComponent->compMutex); */
            }
            *ppBuffer = temp_bufferHeader;
            ret = OMX_ErrorNone;
            goto EXIT;
        }
    }

    NAM_OSAL_Free(temp_bufferHeader);
    NAM_OSAL_Free(temp_buffer);
    ret = OMX_ErrorInsufficientResources;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_OMX_FreeBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_U32        nPortIndex,
    OMX_IN OMX_BUFFERHEADERTYPE *pBufferHdr)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    NAM_OMX_BASECOMPONENT *pNAMComponent = NULL;
    NAM_OMX_BASEPORT      *pNAMPort = NULL;
    OMX_BUFFERHEADERTYPE  *temp_bufferHeader = NULL;
    OMX_U8                *temp_buffer = NULL;
    int                    i = 0;

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
    pNAMPort = &pNAMComponent->pNAMPort[nPortIndex];

    if (CHECK_PORT_TUNNELED(pNAMPort) && CHECK_PORT_BUFFER_SUPPLIER(pNAMPort)) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    if ((pNAMPort->portState != OMX_StateLoaded) && (pNAMPort->portState != OMX_StateInvalid)) {
        (*(pNAMComponent->pCallbacks->EventHandler)) (pOMXComponent,
                        pNAMComponent->callbackData,
                        (OMX_U32)OMX_EventError,
                        (OMX_U32)OMX_ErrorPortUnpopulated,
                        nPortIndex, NULL);
    }

    for (i = 0; i < pNAMPort->portDefinition.nBufferCountActual; i++) {
        if (((pNAMPort->bufferStateAllocate[i] | BUFFER_STATE_FREE) != 0) && (pNAMPort->bufferHeader[i] != NULL)) {
            if (pNAMPort->bufferHeader[i]->pBuffer == pBufferHdr->pBuffer) {
                if (pNAMPort->bufferStateAllocate[i] & BUFFER_STATE_ALLOCATED) {
                    NAM_OSAL_Free(pNAMPort->bufferHeader[i]->pBuffer);
                    pNAMPort->bufferHeader[i]->pBuffer = NULL;
                    pBufferHdr->pBuffer = NULL;
                } else if (pNAMPort->bufferStateAllocate[i] & BUFFER_STATE_ASSIGNED) {
                    ; /* None*/
                }
                pNAMPort->assignedBufferNum--;
                if (pNAMPort->bufferStateAllocate[i] & HEADER_STATE_ALLOCATED) {
                    NAM_OSAL_Free(pNAMPort->bufferHeader[i]);
                    pNAMPort->bufferHeader[i] = NULL;
                    pBufferHdr = NULL;
                }
                pNAMPort->bufferStateAllocate[i] = BUFFER_STATE_FREE;
                ret = OMX_ErrorNone;
                goto EXIT;
            }
        }
    }

EXIT:
    if (ret == OMX_ErrorNone) {
        if (pNAMPort->assignedBufferNum == 0 ) {
            NAM_OSAL_Log(NAM_LOG_TRACE, "pNAMPort->unloadedResource signal set");
            /* NAM_OSAL_MutexLock(pNAMComponent->compMutex); */
            NAM_OSAL_SemaphorePost(pNAMPort->unloadedResource);
            /* NAM_OSAL_MutexUnlock(pNAMComponent->compMutex); */
            pNAMPort->portDefinition.bPopulated = OMX_FALSE;
        }
    }

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_OMX_AllocateTunnelBuffer(NAM_OMX_BASEPORT *pOMXBasePort, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE                 ret = OMX_ErrorNone;
    NAM_OMX_BASEPORT             *pNAMPort = NULL;
    OMX_BUFFERHEADERTYPE         *temp_bufferHeader = NULL;
    OMX_U8                       *temp_buffer = NULL;
    OMX_U32                       bufferSize = 0;
    OMX_PARAM_PORTDEFINITIONTYPE  portDefinition;

    ret = OMX_ErrorTunnelingUnsupported;
EXIT:
    return ret;
}

OMX_ERRORTYPE NAM_OMX_FreeTunnelBuffer(NAM_OMX_BASEPORT *pOMXBasePort, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    NAM_OMX_BASEPORT* pNAMPort = NULL;
    OMX_BUFFERHEADERTYPE* temp_bufferHeader = NULL;
    OMX_U8 *temp_buffer = NULL;
    OMX_U32 bufferSize = 0;
    OMX_PARAM_PORTDEFINITIONTYPE portDefinition;

    ret = OMX_ErrorTunnelingUnsupported;
EXIT:
    return ret;
}

OMX_ERRORTYPE NAM_OMX_ComponentTunnelRequest(
    OMX_IN OMX_HANDLETYPE hComp,
    OMX_IN OMX_U32        nPort,
    OMX_IN OMX_HANDLETYPE hTunneledComp,
    OMX_IN OMX_U32        nTunneledPort,
    OMX_INOUT OMX_TUNNELSETUPTYPE *pTunnelSetup)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    ret = OMX_ErrorTunnelingUnsupported;
EXIT:
    return ret;
}

OMX_BOOL NAM_Check_BufferProcess_State(NAM_OMX_BASECOMPONENT *pNAMComponent)
{
    if ((pNAMComponent->currentState == OMX_StateExecuting) &&
        (pNAMComponent->pNAMPort[INPUT_PORT_INDEX].portState == OMX_StateIdle) &&
        (pNAMComponent->pNAMPort[OUTPUT_PORT_INDEX].portState == OMX_StateIdle) &&
        (pNAMComponent->transientState != NAM_OMX_TransStateExecutingToIdle) &&
        (pNAMComponent->transientState != NAM_OMX_TransStateIdleToExecuting))
        return OMX_TRUE;
    else
        return OMX_FALSE;
}

static OMX_ERRORTYPE NAM_InputBufferReturn(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    NAM_OMX_BASECOMPONENT *pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    NAM_OMX_BASEPORT      *namOMXInputPort = &pNAMComponent->pNAMPort[INPUT_PORT_INDEX];
    NAM_OMX_BASEPORT      *namOMXOutputPort = &pNAMComponent->pNAMPort[OUTPUT_PORT_INDEX];
    NAM_OMX_DATABUFFER    *dataBuffer = &pNAMComponent->namDataBuffer[INPUT_PORT_INDEX];
    OMX_BUFFERHEADERTYPE  *bufferHeader = dataBuffer->bufferHeader;

    FunctionIn();

    if (bufferHeader != NULL) {
        if (namOMXInputPort->markType.hMarkTargetComponent != NULL ) {
            bufferHeader->hMarkTargetComponent      = namOMXInputPort->markType.hMarkTargetComponent;
            bufferHeader->pMarkData                 = namOMXInputPort->markType.pMarkData;
            namOMXInputPort->markType.hMarkTargetComponent = NULL;
            namOMXInputPort->markType.pMarkData = NULL;
        }

        if (bufferHeader->hMarkTargetComponent != NULL) {
            if (bufferHeader->hMarkTargetComponent == pOMXComponent) {
                pNAMComponent->pCallbacks->EventHandler(pOMXComponent,
                                pNAMComponent->callbackData,
                                OMX_EventMark,
                                0, 0, bufferHeader->pMarkData);
            } else {
                pNAMComponent->propagateMarkType.hMarkTargetComponent = bufferHeader->hMarkTargetComponent;
                pNAMComponent->propagateMarkType.pMarkData = bufferHeader->pMarkData;
            }
        }

        if (CHECK_PORT_TUNNELED(namOMXInputPort)) {
            OMX_FillThisBuffer(namOMXInputPort->tunneledComponent, bufferHeader);
        } else {
            bufferHeader->nFilledLen = 0;
            pNAMComponent->pCallbacks->EmptyBufferDone(pOMXComponent, pNAMComponent->callbackData, bufferHeader);
        }
    }

    if ((pNAMComponent->currentState == OMX_StatePause) &&
        ((!CHECK_PORT_BEING_FLUSHED(namOMXInputPort) && !CHECK_PORT_BEING_FLUSHED(namOMXOutputPort)))) {
        NAM_OSAL_SignalReset(pNAMComponent->pauseEvent);
        NAM_OSAL_SignalWait(pNAMComponent->pauseEvent, DEF_MAX_WAIT_TIME);
    }

    dataBuffer->dataValid     = OMX_FALSE;
    dataBuffer->dataLen       = 0;
    dataBuffer->remainDataLen = 0;
    dataBuffer->usedDataLen   = 0;
    dataBuffer->bufferHeader  = NULL;
    dataBuffer->nFlags        = 0;
    dataBuffer->timeStamp     = 0;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_InputBufferGetQueue(NAM_OMX_BASECOMPONENT *pNAMComponent)
{
    OMX_ERRORTYPE       ret = OMX_ErrorNone;
    NAM_OMX_BASEPORT   *pNAMPort = NULL;
    NAM_OMX_DATABUFFER *dataBuffer = NULL;
    NAM_OMX_MESSAGE*    message = NULL;
    NAM_OMX_DATABUFFER *inputUseBuffer = &pNAMComponent->namDataBuffer[INPUT_PORT_INDEX];

    FunctionIn();

    pNAMPort= &pNAMComponent->pNAMPort[INPUT_PORT_INDEX];
    dataBuffer = &pNAMComponent->namDataBuffer[INPUT_PORT_INDEX];

    if (pNAMComponent->currentState != OMX_StateExecuting) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    } else {
        NAM_OSAL_SemaphoreWait(pNAMPort->bufferSemID);
        NAM_OSAL_MutexLock(inputUseBuffer->bufferMutex);
        if (dataBuffer->dataValid != OMX_TRUE) {
            message = (NAM_OMX_MESSAGE *)NAM_OSAL_Dequeue(&pNAMPort->bufferQ);
            if (message == NULL) {
                ret = OMX_ErrorUndefined;
                NAM_OSAL_MutexUnlock(inputUseBuffer->bufferMutex);
                goto EXIT;
            }

            dataBuffer->bufferHeader = (OMX_BUFFERHEADERTYPE *)(message->pCmdData);
            dataBuffer->allocSize = dataBuffer->bufferHeader->nAllocLen;
            dataBuffer->dataLen = dataBuffer->bufferHeader->nFilledLen;
            dataBuffer->remainDataLen = dataBuffer->dataLen;
            dataBuffer->usedDataLen = 0; //dataBuffer->bufferHeader->nOffset;
            dataBuffer->dataValid = OMX_TRUE;
            dataBuffer->nFlags = dataBuffer->bufferHeader->nFlags;
            dataBuffer->timeStamp = dataBuffer->bufferHeader->nTimeStamp;
            pNAMComponent->processData[INPUT_PORT_INDEX].dataBuffer = dataBuffer->bufferHeader->pBuffer;
            pNAMComponent->processData[INPUT_PORT_INDEX].allocSize = dataBuffer->bufferHeader->nAllocLen;

            NAM_OSAL_Free(message);
        }
        NAM_OSAL_MutexUnlock(inputUseBuffer->bufferMutex);
        ret = OMX_ErrorNone;
    }
EXIT:
    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE NAM_OutputBufferReturn(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    NAM_OMX_BASECOMPONENT *pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    NAM_OMX_BASEPORT      *namOMXInputPort = &pNAMComponent->pNAMPort[INPUT_PORT_INDEX];
    NAM_OMX_BASEPORT      *namOMXOutputPort = &pNAMComponent->pNAMPort[OUTPUT_PORT_INDEX];
    NAM_OMX_DATABUFFER    *dataBuffer = &pNAMComponent->namDataBuffer[OUTPUT_PORT_INDEX];
    OMX_BUFFERHEADERTYPE  *bufferHeader = dataBuffer->bufferHeader;

    FunctionIn();

    if (bufferHeader != NULL) {
        bufferHeader->nFilledLen = dataBuffer->remainDataLen;
        bufferHeader->nOffset    = 0;
        bufferHeader->nFlags     = dataBuffer->nFlags;
        bufferHeader->nTimeStamp = dataBuffer->timeStamp;

        if (pNAMComponent->propagateMarkType.hMarkTargetComponent != NULL) {
            bufferHeader->hMarkTargetComponent = pNAMComponent->propagateMarkType.hMarkTargetComponent;
            bufferHeader->pMarkData = pNAMComponent->propagateMarkType.pMarkData;
            pNAMComponent->propagateMarkType.hMarkTargetComponent = NULL;
            pNAMComponent->propagateMarkType.pMarkData = NULL;
        }

        if (bufferHeader->nFlags & OMX_BUFFERFLAG_EOS) {
            pNAMComponent->pCallbacks->EventHandler(pOMXComponent,
                            pNAMComponent->callbackData,
                            OMX_EventBufferFlag,
                            OUTPUT_PORT_INDEX,
                            bufferHeader->nFlags, NULL);
        }

        if (CHECK_PORT_TUNNELED(namOMXOutputPort)) {
            OMX_EmptyThisBuffer(namOMXOutputPort->tunneledComponent, bufferHeader);
        } else {
            pNAMComponent->pCallbacks->FillBufferDone(pOMXComponent, pNAMComponent->callbackData, bufferHeader);
        }
    }

    if ((pNAMComponent->currentState == OMX_StatePause) &&
        ((!CHECK_PORT_BEING_FLUSHED(namOMXInputPort) && !CHECK_PORT_BEING_FLUSHED(namOMXOutputPort)))) {
        NAM_OSAL_SignalReset(pNAMComponent->pauseEvent);
        NAM_OSAL_SignalWait(pNAMComponent->pauseEvent, DEF_MAX_WAIT_TIME);
    }
    
    /* reset dataBuffer */
    dataBuffer->dataValid     = OMX_FALSE;
    dataBuffer->dataLen       = 0;
    dataBuffer->remainDataLen = 0;
    dataBuffer->usedDataLen   = 0;
    dataBuffer->bufferHeader  = NULL;
    dataBuffer->nFlags        = 0;
    dataBuffer->timeStamp     = 0;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_OutputBufferGetQueue(NAM_OMX_BASECOMPONENT *pNAMComponent)
{
    OMX_ERRORTYPE       ret = OMX_ErrorNone;
    NAM_OMX_BASEPORT   *pNAMPort = NULL;
    NAM_OMX_DATABUFFER *dataBuffer = NULL;
    NAM_OMX_MESSAGE    *message = NULL;
    NAM_OMX_DATABUFFER *outputUseBuffer = &pNAMComponent->namDataBuffer[OUTPUT_PORT_INDEX];

    FunctionIn();

    pNAMPort= &pNAMComponent->pNAMPort[OUTPUT_PORT_INDEX];
    dataBuffer = &pNAMComponent->namDataBuffer[OUTPUT_PORT_INDEX];

    if (pNAMComponent->currentState != OMX_StateExecuting) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    } else {
        NAM_OSAL_SemaphoreWait(pNAMPort->bufferSemID);
        NAM_OSAL_MutexLock(outputUseBuffer->bufferMutex);
        if (dataBuffer->dataValid != OMX_TRUE) {
            message = (NAM_OMX_MESSAGE *)NAM_OSAL_Dequeue(&pNAMPort->bufferQ);
            if (message == NULL) {
                ret = OMX_ErrorUndefined;
                NAM_OSAL_MutexUnlock(outputUseBuffer->bufferMutex);
                goto EXIT;
            }

            dataBuffer->bufferHeader = (OMX_BUFFERHEADERTYPE *)(message->pCmdData);
            dataBuffer->allocSize = dataBuffer->bufferHeader->nAllocLen;
            dataBuffer->dataLen = 0; //dataBuffer->bufferHeader->nFilledLen;
            dataBuffer->remainDataLen = dataBuffer->dataLen;
            dataBuffer->usedDataLen = 0; //dataBuffer->bufferHeader->nOffset;
            dataBuffer->dataValid =OMX_TRUE;
            /* dataBuffer->nFlags = dataBuffer->bufferHeader->nFlags; */
            /* dataBuffer->nTimeStamp = dataBuffer->bufferHeader->nTimeStamp; */
            NAM_OSAL_Free(message);
        }
        NAM_OSAL_MutexUnlock(outputUseBuffer->bufferMutex);
        ret = OMX_ErrorNone;
    }
EXIT:
    FunctionOut();

    return ret;

}

static OMX_ERRORTYPE NAM_BufferReset(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 portIndex)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    NAM_OMX_BASECOMPONENT *pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    /* NAM_OMX_BASEPORT      *pNAMPort = &pNAMComponent->pNAMPort[portIndex]; */
    NAM_OMX_DATABUFFER    *dataBuffer = &pNAMComponent->namDataBuffer[portIndex];
    /* OMX_BUFFERHEADERTYPE  *bufferHeader = dataBuffer->bufferHeader; */

    dataBuffer->dataValid     = OMX_FALSE;
    dataBuffer->dataLen       = 0;
    dataBuffer->remainDataLen = 0;
    dataBuffer->usedDataLen   = 0;
    dataBuffer->bufferHeader  = NULL;
    dataBuffer->nFlags        = 0;
    dataBuffer->timeStamp     = 0;

    return ret;
}

static OMX_ERRORTYPE NAM_DataReset(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 portIndex)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    NAM_OMX_BASECOMPONENT *pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    /* NAM_OMX_BASEPORT      *pNAMPort = &pNAMComponent->pNAMPort[portIndex]; */
    /* NAM_OMX_DATABUFFER    *dataBuffer = &pNAMComponent->namDataBuffer[portIndex]; */
    /* OMX_BUFFERHEADERTYPE  *bufferHeader = dataBuffer->bufferHeader; */
    NAM_OMX_DATA          *processData = &pNAMComponent->processData[portIndex];

    processData->dataLen       = 0;
    processData->remainDataLen = 0;
    processData->usedDataLen   = 0;
    processData->nFlags        = 0;
    processData->timeStamp     = 0;

    return ret;
}

OMX_BOOL NAM_Preprocessor_InputData(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_BOOL               ret = OMX_FALSE;
    NAM_OMX_BASECOMPONENT *pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    NAM_OMX_DATABUFFER    *inputUseBuffer = &pNAMComponent->namDataBuffer[INPUT_PORT_INDEX];
    NAM_OMX_DATA          *inputData = &pNAMComponent->processData[INPUT_PORT_INDEX];
    OMX_U32                copySize = 0;
    OMX_BYTE               checkInputStream = NULL;
    OMX_U32                checkInputStreamLen = 0;
    OMX_U32                checkedSize = 0;
    OMX_BOOL               flagEOS = OMX_FALSE;
    OMX_BOOL               flagEOF = OMX_FALSE;
    OMX_BOOL               previousFrameEOF = OMX_FALSE;

    if (inputUseBuffer->dataValid == OMX_TRUE) {
        checkInputStream = inputUseBuffer->bufferHeader->pBuffer + inputUseBuffer->usedDataLen;
        checkInputStreamLen = inputUseBuffer->remainDataLen;

        if ((inputUseBuffer->nFlags & OMX_BUFFERFLAG_ENDOFFRAME) &&
            (pNAMComponent->bUseFlagEOF == OMX_FALSE)) {
            pNAMComponent->bUseFlagEOF = OMX_TRUE;
        }

        if (inputData->dataLen == 0) {
            previousFrameEOF = OMX_TRUE;
        } else {
            previousFrameEOF = OMX_FALSE;
        }
        if (pNAMComponent->bUseFlagEOF == OMX_TRUE) {
            flagEOF = OMX_TRUE;
            checkedSize = checkInputStreamLen;
            if (checkedSize == 0) {
                NAM_OMX_BASEPORT *pNAMPort = &pNAMComponent->pNAMPort[INPUT_PORT_INDEX];
                int width = pNAMPort->portDefinition.format.video.nFrameWidth;
                int height = pNAMPort->portDefinition.format.video.nFrameHeight;
                inputUseBuffer->remainDataLen = inputUseBuffer->dataLen = (width * height * 3) / 2;
                checkedSize = checkInputStreamLen = inputUseBuffer->remainDataLen;
                inputUseBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
            }
            if (inputUseBuffer->nFlags & OMX_BUFFERFLAG_EOS) {
                flagEOS = OMX_TRUE;
            }
        } else {
            NAM_OMX_BASEPORT *pNAMPort = &pNAMComponent->pNAMPort[INPUT_PORT_INDEX];
            int width = pNAMPort->portDefinition.format.video.nFrameWidth;
            int height = pNAMPort->portDefinition.format.video.nFrameHeight;
            int oneFrameSize = 0;

            if (pNAMPort->portDefinition.format.video.eColorFormat == OMX_COLOR_FormatYUV420SemiPlanar)
                oneFrameSize = (width * height * 3) / 2;
            else if (pNAMPort->portDefinition.format.video.eColorFormat == OMX_COLOR_FormatYUV420Planar)
                oneFrameSize = (width * height * 3) / 2;
            else if (pNAMPort->portDefinition.format.video.eColorFormat == OMX_COLOR_FormatYUV422Planar)
                oneFrameSize = width * height * 2;

            if (previousFrameEOF == OMX_TRUE) {
                if (checkInputStreamLen >= oneFrameSize) {
                    checkedSize = oneFrameSize;
                    flagEOF = OMX_TRUE;
                } else {
                    flagEOF = OMX_FALSE;
                }
            } else {
                if (checkInputStreamLen >= (oneFrameSize - inputData->dataLen)) {
                    checkedSize = oneFrameSize - inputData->dataLen;
                    flagEOF = OMX_TRUE;
                } else {
                    flagEOF = OMX_FALSE;
                }
            }

            if ((flagEOF == OMX_FALSE) && (inputUseBuffer->nFlags & OMX_BUFFERFLAG_EOS)) {
                flagEOF = OMX_TRUE;
                flagEOS = OMX_TRUE;
            }
        }

        if (flagEOF == OMX_TRUE) {
            copySize = checkedSize;
            NAM_OSAL_Log(NAM_LOG_TRACE, "nam_checkInputFrame : OMX_TRUE");
        } else {
            copySize = checkInputStreamLen;
            NAM_OSAL_Log(NAM_LOG_TRACE, "nam_checkInputFrame : OMX_FALSE");
        }

        if (inputUseBuffer->nFlags & OMX_BUFFERFLAG_EOS)
            pNAMComponent->bSaveFlagEOS = OMX_TRUE;

        if (((inputData->allocSize) - (inputData->dataLen)) >= copySize) {
            NAM_OMX_BASEPORT *pNAMPort = &pNAMComponent->pNAMPort[INPUT_PORT_INDEX];

            if ((pNAMPort->portDefinition.format.video.eColorFormat != OMX_SEC_COLOR_FormatNV12TPhysicalAddress) &&
                (pNAMPort->bStoreMetaDataInBuffer == OMX_FALSE)) {
                if (flagEOF == OMX_TRUE) {
                    OMX_U32 width, height;

                    width = pNAMPort->portDefinition.format.video.nFrameWidth;
                    height = pNAMPort->portDefinition.format.video.nFrameHeight;

                    NAM_OSAL_Log(NAM_LOG_TRACE, "inputData->specificBufferHeader.YVirAddr : 0x%x", inputData->specificBufferHeader.YVirAddr);
                    NAM_OSAL_Log(NAM_LOG_TRACE, "inputData->specificBufferHeader.CVirAddr : 0x%x", inputData->specificBufferHeader.CVirAddr);

                    NAM_OSAL_Log(NAM_LOG_TRACE, "width:%d, height:%d, Ysize:%d", width, height, ALIGN_TO_8KB(ALIGN_TO_128B(width) * ALIGN_TO_32B(height)));
                    NAM_OSAL_Log(NAM_LOG_TRACE, "width:%d, height:%d, Csize:%d", width, height, ALIGN_TO_8KB(ALIGN_TO_128B(width) * ALIGN_TO_32B(height / 2)));

                    switch (pNAMPort->portDefinition.format.video.eColorFormat) {
                    case OMX_COLOR_FormatYUV420Planar:
                        /* Real YUV420P Data */
                        csc_linear_to_tiled(inputData->specificBufferHeader.YVirAddr,
                                                checkInputStream, width, height);
                        csc_linear_to_tiled_interleave(inputData->specificBufferHeader.CVirAddr,
                                                 checkInputStream + (width * height),
                                                 checkInputStream + (((width * height) * 5) / 4),
                                                 width, height >> 1);
                        break;
                    case OMX_COLOR_FormatYUV420SemiPlanar:
                    default:
                        NAM_OSAL_Memcpy(inputData->specificBufferHeader.YVirAddr, checkInputStream, (width * height));
                        NAM_OSAL_Memcpy(inputData->specificBufferHeader.CVirAddr, checkInputStream + (width * height), (width * height / 2));
                        break;
                    }
                }
            }

            inputUseBuffer->dataLen -= copySize;
            inputUseBuffer->remainDataLen -= copySize;
            inputUseBuffer->usedDataLen += copySize;

            inputData->dataLen += copySize;
            inputData->remainDataLen += copySize;

            if (previousFrameEOF == OMX_TRUE) {
                inputData->timeStamp = inputUseBuffer->timeStamp;
                inputData->nFlags = inputUseBuffer->nFlags;
            }

            if (pNAMComponent->bUseFlagEOF == OMX_TRUE) {
                if (pNAMComponent->bSaveFlagEOS == OMX_TRUE) {
                    inputData->nFlags |= OMX_BUFFERFLAG_EOS;
                    flagEOF = OMX_TRUE;
                    pNAMComponent->bSaveFlagEOS = OMX_FALSE;
                }
            } else {
                if ((checkedSize == checkInputStreamLen) && (pNAMComponent->bSaveFlagEOS == OMX_TRUE)) {
                    inputData->nFlags |= OMX_BUFFERFLAG_EOS;
                    flagEOF = OMX_TRUE;
                    pNAMComponent->bSaveFlagEOS = OMX_FALSE;
                } else {
                    inputData->nFlags = (inputUseBuffer->nFlags & (~OMX_BUFFERFLAG_EOS));
                }
            }
        } else {
            /*????????????????????????????????? Error ?????????????????????????????????*/
            NAM_DataReset(pOMXComponent, INPUT_PORT_INDEX);
            flagEOF = OMX_FALSE;
        }

        if (inputUseBuffer->remainDataLen == 0) {
            if(flagEOF == OMX_FALSE)
                NAM_InputBufferReturn(pOMXComponent);
        } else {
            inputUseBuffer->dataValid = OMX_TRUE;
        }
    }

    if (flagEOF == OMX_TRUE) {
        if (pNAMComponent->checkTimeStamp.needSetStartTimeStamp == OMX_TRUE) {
            pNAMComponent->checkTimeStamp.needCheckStartTimeStamp = OMX_TRUE;
            pNAMComponent->checkTimeStamp.startTimeStamp = inputData->timeStamp;
            pNAMComponent->checkTimeStamp.nStartFlags = inputData->nFlags;
            pNAMComponent->checkTimeStamp.needSetStartTimeStamp = OMX_FALSE;
        }

        ret = OMX_TRUE;
    } else {
        ret = OMX_FALSE;
    }
    return ret;
}

OMX_BOOL NAM_Postprocess_OutputData(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_BOOL               ret = OMX_FALSE;
    NAM_OMX_BASECOMPONENT *pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    NAM_OMX_DATABUFFER    *outputUseBuffer = &pNAMComponent->namDataBuffer[OUTPUT_PORT_INDEX];
    NAM_OMX_DATA          *outputData = &pNAMComponent->processData[OUTPUT_PORT_INDEX];
    OMX_U32                copySize = 0;

    if (outputUseBuffer->dataValid == OMX_TRUE) {
        if (pNAMComponent->checkTimeStamp.needCheckStartTimeStamp == OMX_TRUE) {
            if (pNAMComponent->checkTimeStamp.startTimeStamp == outputData->timeStamp){
                pNAMComponent->checkTimeStamp.startTimeStamp = -19761123;
                pNAMComponent->checkTimeStamp.nStartFlags = 0x0;
                pNAMComponent->checkTimeStamp.needSetStartTimeStamp = OMX_FALSE;
                pNAMComponent->checkTimeStamp.needCheckStartTimeStamp = OMX_FALSE;
            } else {
                NAM_DataReset(pOMXComponent, OUTPUT_PORT_INDEX);

                ret = OMX_TRUE;
                goto EXIT;
            }
        } else if (pNAMComponent->checkTimeStamp.needSetStartTimeStamp == OMX_TRUE) {
            NAM_DataReset(pOMXComponent, OUTPUT_PORT_INDEX);

            ret = OMX_TRUE;
            goto EXIT;
        }

        if (outputData->remainDataLen <= (outputUseBuffer->allocSize - outputUseBuffer->dataLen)) {
            copySize = outputData->remainDataLen;
            if (copySize > 0)
                NAM_OSAL_Memcpy((outputUseBuffer->bufferHeader->pBuffer + outputUseBuffer->dataLen),
                    (outputData->dataBuffer + outputData->usedDataLen),
                     copySize);

            outputUseBuffer->dataLen += copySize;
            outputUseBuffer->remainDataLen += copySize;
            outputUseBuffer->nFlags = outputData->nFlags;
            outputUseBuffer->timeStamp = outputData->timeStamp;

            ret = OMX_TRUE;

            /* reset outputData */
            NAM_DataReset(pOMXComponent, OUTPUT_PORT_INDEX);

            if ((outputUseBuffer->remainDataLen > 0) ||
                (outputUseBuffer->nFlags & OMX_BUFFERFLAG_EOS))
                NAM_OutputBufferReturn(pOMXComponent);
        } else {
            NAM_OSAL_Log(NAM_LOG_ERROR, "output buffer is smaller than encoded data size Out Length");

            copySize = outputUseBuffer->allocSize - outputUseBuffer->dataLen;

            NAM_OSAL_Memcpy((outputUseBuffer->bufferHeader->pBuffer + outputUseBuffer->dataLen),
                    (outputData->dataBuffer + outputData->usedDataLen),
                     copySize);

            outputUseBuffer->dataLen += copySize;
            outputUseBuffer->remainDataLen += copySize;
            outputUseBuffer->nFlags = 0;
            outputUseBuffer->timeStamp = outputData->timeStamp;

            ret = OMX_FALSE;

            outputData->remainDataLen -= copySize;
            outputData->usedDataLen += copySize;

            NAM_OutputBufferReturn(pOMXComponent);
        }
    } else {
        ret = OMX_FALSE;
    }

EXIT:
    return ret;
}

OMX_ERRORTYPE NAM_OMX_BufferProcess(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    NAM_OMX_BASECOMPONENT *pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    NAM_OMX_BASEPORT      *namInputPort = &pNAMComponent->pNAMPort[INPUT_PORT_INDEX];
    NAM_OMX_BASEPORT      *namOutputPort = &pNAMComponent->pNAMPort[OUTPUT_PORT_INDEX];
    NAM_OMX_DATABUFFER    *inputUseBuffer = &pNAMComponent->namDataBuffer[INPUT_PORT_INDEX];
    NAM_OMX_DATABUFFER    *outputUseBuffer = &pNAMComponent->namDataBuffer[OUTPUT_PORT_INDEX];
    NAM_OMX_DATA          *inputData = &pNAMComponent->processData[INPUT_PORT_INDEX];
    NAM_OMX_DATA          *outputData = &pNAMComponent->processData[OUTPUT_PORT_INDEX];
    OMX_U32                copySize = 0;

    pNAMComponent->remainOutputData = OMX_FALSE;
    pNAMComponent->reInputData = OMX_FALSE;

    FunctionIn();

    while (!pNAMComponent->bExitBufferProcessThread) {
        NAM_OSAL_SleepMillinam(0);

        if (((pNAMComponent->currentState == OMX_StatePause) ||
            (pNAMComponent->currentState == OMX_StateIdle) ||
            (pNAMComponent->transientState == NAM_OMX_TransStateLoadedToIdle) ||
            (pNAMComponent->transientState == NAM_OMX_TransStateExecutingToIdle)) &&
            (pNAMComponent->transientState != NAM_OMX_TransStateIdleToLoaded)&&
            ((!CHECK_PORT_BEING_FLUSHED(namInputPort) && !CHECK_PORT_BEING_FLUSHED(namOutputPort)))) {
            NAM_OSAL_SignalReset(pNAMComponent->pauseEvent);
            NAM_OSAL_SignalWait(pNAMComponent->pauseEvent, DEF_MAX_WAIT_TIME);
        }

        while (NAM_Check_BufferProcess_State(pNAMComponent) && !pNAMComponent->bExitBufferProcessThread) {
            NAM_OSAL_SleepMillinam(0);

            NAM_OSAL_MutexLock(outputUseBuffer->bufferMutex);
            if ((outputUseBuffer->dataValid != OMX_TRUE) &&
                (!CHECK_PORT_BEING_FLUSHED(namOutputPort))) {
                NAM_OSAL_MutexUnlock(outputUseBuffer->bufferMutex);
                ret = NAM_OutputBufferGetQueue(pNAMComponent);
                if ((ret == OMX_ErrorUndefined) ||
                    (namInputPort->portState != OMX_StateIdle) ||
                    (namOutputPort->portState != OMX_StateIdle)) {
                    break;
                }
            } else {
                NAM_OSAL_MutexUnlock(outputUseBuffer->bufferMutex);
            }

            if (pNAMComponent->remainOutputData == OMX_FALSE) {
                if (pNAMComponent->reInputData == OMX_FALSE) {
                    NAM_OSAL_MutexLock(inputUseBuffer->bufferMutex);
                    if ((NAM_Preprocessor_InputData(pOMXComponent) == OMX_FALSE) &&
                        (!CHECK_PORT_BEING_FLUSHED(namInputPort))) {
                            NAM_OSAL_MutexUnlock(inputUseBuffer->bufferMutex);
                            ret = NAM_InputBufferGetQueue(pNAMComponent);
                            break;
                    }

                    NAM_OSAL_MutexUnlock(inputUseBuffer->bufferMutex);
                }

                NAM_OSAL_MutexLock(inputUseBuffer->bufferMutex);
                NAM_OSAL_MutexLock(outputUseBuffer->bufferMutex);
                ret = pNAMComponent->nam_dmai_bufferProcess(pOMXComponent, inputData, outputData);

                if (inputUseBuffer->remainDataLen == 0)
                    NAM_InputBufferReturn(pOMXComponent);
                else
                    inputUseBuffer->dataValid = OMX_TRUE;

                NAM_OSAL_MutexUnlock(outputUseBuffer->bufferMutex);
                NAM_OSAL_MutexUnlock(inputUseBuffer->bufferMutex);

                if (ret == OMX_ErrorInputDataEncodeYet)
                    pNAMComponent->reInputData = OMX_TRUE;
                else
                    pNAMComponent->reInputData = OMX_FALSE;
            }

            NAM_OSAL_MutexLock(outputUseBuffer->bufferMutex);

            if (NAM_Postprocess_OutputData(pOMXComponent) == OMX_FALSE)
                pNAMComponent->remainOutputData = OMX_TRUE;
            else
                pNAMComponent->remainOutputData = OMX_FALSE;

            NAM_OSAL_MutexUnlock(outputUseBuffer->bufferMutex);
        }
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_OMX_VideoEncodeGetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nParamIndex,
    OMX_INOUT OMX_PTR     ComponentParameterStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    NAM_OMX_BASECOMPONENT *pNAMComponent = NULL;
    NAM_OMX_BASEPORT      *pNAMPort = NULL;

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

    if (pNAMComponent->currentState == OMX_StateInvalid ) {
        ret = OMX_StateInvalid;
        goto EXIT;
    }

    if (ComponentParameterStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    switch (nParamIndex) {
    case OMX_IndexParamVideoInit:
    {
        OMX_PORT_PARAM_TYPE *portParam = (OMX_PORT_PARAM_TYPE *)ComponentParameterStructure;
        ret = NAM_OMX_Check_SizeVersion(portParam, sizeof(OMX_PORT_PARAM_TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        portParam->nPorts           = pNAMComponent->portParam.nPorts;
        portParam->nStartPortNumber = pNAMComponent->portParam.nStartPortNumber;
        ret = OMX_ErrorNone;
    }
        break;
    case OMX_IndexParamVideoPortFormat:
    {
        OMX_VIDEO_PARAM_PORTFORMATTYPE *portFormat = (OMX_VIDEO_PARAM_PORTFORMATTYPE *)ComponentParameterStructure;
        OMX_U32                         portIndex = portFormat->nPortIndex;
        OMX_U32                         index    = portFormat->nIndex;
        NAM_OMX_BASEPORT               *pNAMPort = NULL;
        OMX_PARAM_PORTDEFINITIONTYPE   *portDefinition = NULL;
        OMX_U32                         supportFormatNum = 0;

        ret = NAM_OMX_Check_SizeVersion(portFormat, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if ((portIndex >= pNAMComponent->portParam.nPorts)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }


        if (portIndex == INPUT_PORT_INDEX) {
            supportFormatNum = INPUT_PORT_SUPPORTFORMAT_NUM_MAX - 1;
            if (index > supportFormatNum) {
                ret = OMX_ErrorNoMore;
                goto EXIT;
            }

            pNAMPort = &pNAMComponent->pNAMPort[INPUT_PORT_INDEX];
            portDefinition = &pNAMPort->portDefinition;

            switch (index) {
            case supportFormat_0:
                portFormat->eCompressionFormat = OMX_VIDEO_CodingUnused;
                portFormat->eColorFormat       = OMX_COLOR_FormatYUV420SemiPlanar;
                portFormat->xFramerate         = portDefinition->format.video.xFramerate;
                break;
            case supportFormat_1:
                portFormat->eCompressionFormat = OMX_VIDEO_CodingUnused;
                portFormat->eColorFormat       = OMX_COLOR_FormatYUV420Planar;
                portFormat->xFramerate         = portDefinition->format.video.xFramerate;
                break;
            case supportFormat_2:
                portFormat->eCompressionFormat = OMX_VIDEO_CodingUnused;
                portFormat->eColorFormat       = OMX_SEC_COLOR_FormatNV12TPhysicalAddress;
                portFormat->xFramerate         = portDefinition->format.video.xFramerate;
                break;
#ifdef USE_ANDROID_EXTENSION
            case supportFormat_3:
                portFormat->eCompressionFormat = OMX_VIDEO_CodingUnused;
                portFormat->eColorFormat       = OMX_COLOR_FormatAndroidOpaque;
                portFormat->xFramerate         = portDefinition->format.video.xFramerate;
                break;
#endif
            }
        } else if (portIndex == OUTPUT_PORT_INDEX) {
            supportFormatNum = OUTPUT_PORT_SUPPORTFORMAT_NUM_MAX - 1;
            if (index > supportFormatNum) {
                ret = OMX_ErrorNoMore;
                goto EXIT;
            }

            pNAMPort = &pNAMComponent->pNAMPort[OUTPUT_PORT_INDEX];
            portDefinition = &pNAMPort->portDefinition;

            portFormat->eCompressionFormat = portDefinition->format.video.eCompressionFormat;
            portFormat->eColorFormat       = portDefinition->format.video.eColorFormat;
            portFormat->xFramerate         = portDefinition->format.video.xFramerate;
        }
        ret = OMX_ErrorNone;
    }
        break;
    case OMX_IndexParamVideoBitrate:
    {
        OMX_VIDEO_PARAM_BITRATETYPE  *videoRateControl = (OMX_VIDEO_PARAM_BITRATETYPE *)ComponentParameterStructure;
        OMX_U32                       portIndex = videoRateControl->nPortIndex;
        NAM_OMX_BASEPORT         *pNAMPort = NULL;
        OMX_PARAM_PORTDEFINITIONTYPE *portDefinition = NULL;

        if ((portIndex != OUTPUT_PORT_INDEX)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        } else {
            pNAMPort = &pNAMComponent->pNAMPort[portIndex];
            portDefinition = &pNAMPort->portDefinition;

            videoRateControl->eControlRate = pNAMPort->eControlRate;
            videoRateControl->nTargetBitrate = portDefinition->format.video.nBitrate;
        }
        ret = OMX_ErrorNone;
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

#ifdef USE_ANDROID_EXTENSION
        if (portIndex == 0 && pNAMPort->bStoreMetaDataInBuffer == OMX_TRUE) {
            portDefinition->nBufferSize = MAX_INPUT_METADATA_BUFFER_SIZE;
        }
#endif
    }
        break;
    default:
    {
        ret = NAM_OMX_GetParameter(hComponent, nParamIndex, ComponentParameterStructure);
    }
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}
OMX_ERRORTYPE NAM_OMX_VideoEncodeSetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        ComponentParameterStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    NAM_OMX_BASECOMPONENT *pNAMComponent = NULL;
    NAM_OMX_BASEPORT      *pNAMPort = NULL;

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

    if (pNAMComponent->currentState == OMX_StateInvalid ) {
        ret = OMX_StateInvalid;
        goto EXIT;
    }

    if (ComponentParameterStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    switch (nIndex) {
    case OMX_IndexParamVideoPortFormat:
    {
        OMX_VIDEO_PARAM_PORTFORMATTYPE *portFormat = (OMX_VIDEO_PARAM_PORTFORMATTYPE *)ComponentParameterStructure;
        OMX_U32                         portIndex = portFormat->nPortIndex;
        OMX_U32                         index    = portFormat->nIndex;
        NAM_OMX_BASEPORT               *pNAMPort = NULL;
        OMX_PARAM_PORTDEFINITIONTYPE   *portDefinition = NULL;
        OMX_U32                         supportFormatNum = 0;

        ret = NAM_OMX_Check_SizeVersion(portFormat, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if ((portIndex >= pNAMComponent->portParam.nPorts)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        } else {
            pNAMPort = &pNAMComponent->pNAMPort[portIndex];
            portDefinition = &pNAMPort->portDefinition;

            portDefinition->format.video.eColorFormat       = portFormat->eColorFormat;
            portDefinition->format.video.eCompressionFormat = portFormat->eCompressionFormat;
            portDefinition->format.video.xFramerate         = portFormat->xFramerate;
        }
    }
        break;
    case OMX_IndexParamVideoBitrate:
    {
        OMX_VIDEO_PARAM_BITRATETYPE  *videoRateControl = (OMX_VIDEO_PARAM_BITRATETYPE *)ComponentParameterStructure;
        OMX_U32                       portIndex = videoRateControl->nPortIndex;
        NAM_OMX_BASEPORT             *pNAMPort = NULL;
        OMX_PARAM_PORTDEFINITIONTYPE *portDefinition = NULL;

        if ((portIndex != OUTPUT_PORT_INDEX)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        } else {
            pNAMPort = &pNAMComponent->pNAMPort[portIndex];
            portDefinition = &pNAMPort->portDefinition;

            pNAMPort->eControlRate = videoRateControl->eControlRate;
            portDefinition->format.video.nBitrate = videoRateControl->nTargetBitrate;
        }
        ret = OMX_ErrorNone;
    }
        break;
    case OMX_IndexParamPortDefinition:
    {
        OMX_PARAM_PORTDEFINITIONTYPE *pPortDefinition =
                (OMX_PARAM_PORTDEFINITIONTYPE *)ComponentParameterStructure;
        ret = NAM_OMX_SetParameter(hComponent, nIndex, ComponentParameterStructure);
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pPortDefinition->nPortIndex == INPUT_PORT_INDEX) {
            NAM_OMX_BASEPORT *pNAMInputPort = &pNAMComponent->pNAMPort[INPUT_PORT_INDEX];
            OMX_U32 width = pNAMInputPort->portDefinition.format.video.nFrameWidth;
            OMX_U32 height = pNAMInputPort->portDefinition.format.video.nFrameHeight;
            NAM_OMX_BASEPORT *pNAMOutputPort = &pNAMComponent->pNAMPort[OUTPUT_PORT_INDEX];
            pNAMOutputPort->portDefinition.format.video.nFrameWidth = width;
            pNAMOutputPort->portDefinition.format.video.nFrameHeight = height;
            pNAMOutputPort->portDefinition.nBufferSize = (width * height * 3) / 2;
            NAM_OSAL_Log(NAM_LOG_TRACE, "pNAMOutputPort->portDefinition.nBufferSize: %d",
                            pNAMOutputPort->portDefinition.nBufferSize);
        }
    }
        break;
#ifdef USE_ANDROID_EXTENSION
    case OMX_IndexParamStoreMetaDataBuffer:
    {
        if (OMX_ErrorNone != checkVersionANB(ComponentParameterStructure))
            goto EXIT;

        if (INPUT_PORT_INDEX != checkPortIndexANB(ComponentParameterStructure)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        ret = enableStoreMetaDataInBuffers(hComponent, ComponentParameterStructure);
    }
        break;
#endif
    default:
    {
        ret = NAM_OMX_SetParameter(hComponent, nIndex, ComponentParameterStructure);
    }
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_OMX_VideoEncodeComponentInit(OMX_IN OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    NAM_OMX_BASECOMPONENT *pNAMComponent = NULL;
    NAM_OMX_BASEPORT      *pNAMPort = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = NAM_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        NAM_OSAL_Log(NAM_LOG_ERROR, "OMX_Error, Line:%d", __LINE__);
        goto EXIT;
    }

    ret = NAM_OMX_BaseComponent_Constructor(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        NAM_OSAL_Log(NAM_LOG_ERROR, "OMX_Error, Line:%d", __LINE__);
        goto EXIT;
    }

    ret = NAM_OMX_Port_Constructor(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        NAM_OMX_BaseComponent_Destructor(pOMXComponent);
        NAM_OSAL_Log(NAM_LOG_ERROR, "OMX_Error, Line:%d", __LINE__);
        goto EXIT;
    }

    pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pNAMComponent->bSaveFlagEOS = OMX_FALSE;

    /* Input port */
    pNAMPort = &pNAMComponent->pNAMPort[INPUT_PORT_INDEX];
    pNAMPort->portDefinition.nBufferCountActual = MAX_VIDEO_INPUTBUFFER_NUM;
    pNAMPort->portDefinition.nBufferCountMin = MAX_VIDEO_INPUTBUFFER_NUM;
    pNAMPort->portDefinition.nBufferSize = 0;
    pNAMPort->portDefinition.eDomain = OMX_PortDomainVideo;

    pNAMPort->portDefinition.format.video.cMIMEType = NAM_OSAL_Malloc(MAX_OMX_MIMETYPE_SIZE);
    NAM_OSAL_Strcpy(pNAMPort->portDefinition.format.video.cMIMEType, "raw/video");
    pNAMPort->portDefinition.format.video.pNativeRender = 0;
    pNAMPort->portDefinition.format.video.bFlagErrorConcealment = OMX_FALSE;
    pNAMPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;

    pNAMPort->portDefinition.format.video.nFrameWidth = 0;
    pNAMPort->portDefinition.format.video.nFrameHeight= 0;
    pNAMPort->portDefinition.format.video.nStride = 0;
    pNAMPort->portDefinition.format.video.nSliceHeight = 0;
    pNAMPort->portDefinition.format.video.nBitrate = 64000;
    pNAMPort->portDefinition.format.video.xFramerate = (15 << 16);
    pNAMPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    pNAMPort->portDefinition.format.video.pNativeWindow = NULL;

    /* Output port */
    pNAMPort = &pNAMComponent->pNAMPort[OUTPUT_PORT_INDEX];
    pNAMPort->portDefinition.nBufferCountActual = MAX_VIDEO_OUTPUTBUFFER_NUM;
    pNAMPort->portDefinition.nBufferCountMin = MAX_VIDEO_OUTPUTBUFFER_NUM;
    pNAMPort->portDefinition.nBufferSize = DEFAULT_VIDEO_OUTPUT_BUFFER_SIZE;
    pNAMPort->portDefinition.eDomain = OMX_PortDomainVideo;

    pNAMPort->portDefinition.format.video.cMIMEType = NAM_OSAL_Malloc(MAX_OMX_MIMETYPE_SIZE);
    NAM_OSAL_Strcpy(pNAMPort->portDefinition.format.video.cMIMEType, "raw/video");
    pNAMPort->portDefinition.format.video.pNativeRender = 0;
    pNAMPort->portDefinition.format.video.bFlagErrorConcealment = OMX_FALSE;
    pNAMPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;

    pNAMPort->portDefinition.format.video.nFrameWidth = 0;
    pNAMPort->portDefinition.format.video.nFrameHeight= 0;
    pNAMPort->portDefinition.format.video.nStride = 0;
    pNAMPort->portDefinition.format.video.nSliceHeight = 0;
    pNAMPort->portDefinition.format.video.nBitrate = 64000;
    pNAMPort->portDefinition.format.video.xFramerate = (15 << 16);
    pNAMPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    pNAMPort->portDefinition.format.video.pNativeWindow = NULL;

    pOMXComponent->UseBuffer              = &NAM_OMX_UseBuffer;
    pOMXComponent->AllocateBuffer         = &NAM_OMX_AllocateBuffer;
    pOMXComponent->FreeBuffer             = &NAM_OMX_FreeBuffer;
    pOMXComponent->ComponentTunnelRequest = &NAM_OMX_ComponentTunnelRequest;

    pNAMComponent->nam_AllocateTunnelBuffer = &NAM_OMX_AllocateTunnelBuffer;
    pNAMComponent->nam_FreeTunnelBuffer     = &NAM_OMX_FreeTunnelBuffer;
    pNAMComponent->nam_BufferProcess        = &NAM_OMX_BufferProcess;
    pNAMComponent->nam_BufferReset          = &NAM_BufferReset;
    pNAMComponent->nam_InputBufferReturn    = &NAM_InputBufferReturn;
    pNAMComponent->nam_OutputBufferReturn   = &NAM_OutputBufferReturn;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_OMX_VideoEncodeComponentDeinit(OMX_IN OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    NAM_OMX_BASECOMPONENT *pNAMComponent = NULL;
    NAM_OMX_BASEPORT      *pNAMPort = NULL;
    int                    i = 0;

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

    for(i = 0; i < ALL_PORT_NUM; i++) {
        pNAMPort = &pNAMComponent->pNAMPort[i];
        NAM_OSAL_Free(pNAMPort->portDefinition.format.video.cMIMEType);
        pNAMPort->portDefinition.format.video.cMIMEType = NULL;
    }

    ret = NAM_OMX_Port_Destructor(pOMXComponent);

    ret = NAM_OMX_BaseComponent_Destructor(hComponent);

EXIT:
    FunctionOut();

    return ret;
}

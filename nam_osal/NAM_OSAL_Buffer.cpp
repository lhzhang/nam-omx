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
 * @file        NAM_OSAL_Buffer.c
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 *              Jinsung Yang (jsgood.yang@samsung.com)
 * @version     1.0.2
 * @history
 *   2011.5.15 : Create
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "NAM_OMX_Def.h"
#include "NAM_OMX_Macros.h"
#include "NAM_OSAL_Memory.h"
#include "NAM_OSAL_Semaphore.h"
#include "NAM_OSAL_Buffer.h"
#include "NAM_OMX_Basecomponent.h"

#define NAM_LOG_OFF
#include "NAM_OSAL_Log.h"

#ifdef __cplusplus
}
#endif

#include <ui/android_native_buffer.h>
#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferMapper.h>
#include <ui/Rect.h>
#include <media/stagefright/HardwareAPI.h>
#include <hardware/hardware.h>
#include <media/stagefright/MetadataBufferType.h>
#include "hal_public.h"

#define HAL_PIXEL_FORMAT_C110_NV12          0x100

using namespace android;


struct AndroidNativeBuffersParams {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
};

#ifdef USE_ANDROID_EXTENSION
OMX_ERRORTYPE checkVersionANB(OMX_PTR ComponentParameterStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_VERSIONTYPE* version = NULL;


    AndroidNativeBuffersParams *pANBP;
    pANBP = (AndroidNativeBuffersParams *)ComponentParameterStructure;

    version = (OMX_VERSIONTYPE*)((char*)pANBP + sizeof(OMX_U32));
    if (*((OMX_U32*)pANBP) <= sizeof(AndroidNativeBuffersParams)) {
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

OMX_U32 checkPortIndexANB(OMX_PTR ComponentParameterStructure)
{
    AndroidNativeBuffersParams *pANBP;
    pANBP = (AndroidNativeBuffersParams *)ComponentParameterStructure;

    return pANBP->nPortIndex;
}

OMX_U32 getMetadataBufferType(const uint8_t *ptr)
{
    OMX_U32 type = *(OMX_U32 *) ptr;
    NAM_OSAL_Log(NAM_LOG_TRACE, "getMetadataBufferType: %ld", type);
    return type;
}

OMX_U32 getVADDRfromANB(OMX_PTR pUnreadableBuffer, OMX_U32 Width, OMX_U32 Height, void *pVirAddrs[])
{
    OMX_U32 ret = 0;
    android_native_buffer_t *buf;
    void *readableBuffer;
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();
    Rect bounds(Width, Height);

    FunctionIn();

    buf = (android_native_buffer_t *)pUnreadableBuffer;
    NAM_OSAL_Log(NAM_LOG_TRACE, "pUnreadableBuffer:0x%x, buf:0x%x, buf->handle:0x%x",
                                pUnreadableBuffer, buf, buf->handle);

    ret = mapper.lock(buf->handle, GRALLOC_USAGE_SW_WRITE_OFTEN, bounds, pVirAddrs);
    if (ret != 0) {
        NAM_OSAL_Log(NAM_LOG_ERROR, "mapper.lock Error, Error code:%d", ret);
    }
    FunctionOut();

    return ret;
}

OMX_U32 putVADDRtoANB(OMX_PTR pUnreadableBuffer)
{
    android_native_buffer_t *buf;
    void *readableBuffer;
    int ret = 0;
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();

    FunctionIn();

    buf = (android_native_buffer_t *)pUnreadableBuffer;

    FunctionOut();

    return mapper.unlock(buf->handle);
}

OMX_ERRORTYPE enableAndroidNativeBuffer(OMX_HANDLETYPE hComponent, OMX_PTR ComponentParameterStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    NAM_OMX_BASECOMPONENT *pNAMComponent = NULL;
    NAM_OMX_BASEPORT      *pNAMPort = NULL;

    EnableAndroidNativeBuffersParams *peanbp;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    peanbp = (EnableAndroidNativeBuffersParams *)ComponentParameterStructure;
    pNAMPort = &pNAMComponent->pNAMPort[peanbp->nPortIndex];

    if (peanbp->enable == OMX_FALSE) {
        NAM_OSAL_Log(NAM_LOG_TRACE, "disable AndroidNativeBuffer");
        pNAMPort->bUseAndroidNativeBuffer = OMX_FALSE;
    } else {
        NAM_OSAL_Log(NAM_LOG_TRACE, "enable AndroidNativeBuffer");
        pNAMPort->bUseAndroidNativeBuffer = OMX_TRUE;
        pNAMPort->portDefinition.format.video.eColorFormat = (OMX_COLOR_FORMATTYPE)OMX_SEC_COLOR_FormatANBYUV420SemiPlanar;
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE getAndroidNativeBuffer(OMX_HANDLETYPE hComponent, OMX_PTR ComponentParameterStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    NAM_OMX_BASECOMPONENT *pNAMComponent = NULL;
    NAM_OMX_BASEPORT      *pNAMPort = NULL;

    GetAndroidNativeBufferUsageParams *pganbp;

    FunctionIn();

    pganbp = (GetAndroidNativeBufferUsageParams *)ComponentParameterStructure;

    pganbp->nUsage = GRALLOC_USAGE_SW_WRITE_OFTEN;

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE UseBufferANB(
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

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
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
            temp_bufferHeader->nAllocLen      = nSizeBytes;
            temp_bufferHeader->pAppPrivate    = pAppPrivate;
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

OMX_ERRORTYPE useAndroidNativeBuffer(OMX_HANDLETYPE hComponent, OMX_PTR ComponentParameterStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    NAM_OMX_BASECOMPONENT *pNAMComponent = NULL;
    NAM_OMX_BASEPORT      *pNAMPort = NULL;
    OMX_U32                frameSize = 0;
    OMX_U32                bufWidth, bufHeight;
    UseAndroidNativeBufferParams *puanbp;

    FunctionIn();

    puanbp = (UseAndroidNativeBufferParams *)ComponentParameterStructure;

    OMX_PTR buffer = (void *)puanbp->nativeBuffer.get();
    android_native_buffer_t *buf = (android_native_buffer_t *)buffer;
    bufWidth = ((buf->width + 15) / 16) * 16;
    bufHeight = ((buf->height + 15) / 16) * 16;
    frameSize = (bufWidth * bufHeight * 3) / 2;
    NAM_OSAL_Log(NAM_LOG_TRACE, "buffer:0x%x, buf:0x%x, buf->handle:0x%x", buffer, buf, buf->handle);

    ret = UseBufferANB(hComponent, puanbp->bufferHeader, puanbp->nPortIndex,
                       puanbp->pAppPrivate, frameSize, (OMX_U8 *)buffer);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE enableStoreMetaDataInBuffers(OMX_HANDLETYPE hComponent, OMX_PTR ComponentParameterStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    NAM_OMX_BASECOMPONENT *pNAMComponent = NULL;
    NAM_OMX_BASEPORT      *pNAMPort = NULL;

    StoreMetaDataInBuffersParams *pStoreMetaData;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pStoreMetaData = (StoreMetaDataInBuffersParams*)ComponentParameterStructure;
    pNAMPort = &pNAMComponent->pNAMPort[pStoreMetaData->nPortIndex];

    if (pStoreMetaData->bStoreMetaData == OMX_FALSE) {
        NAM_OSAL_Log(NAM_LOG_TRACE, "disable StoreMetaDataInBuffers");
        pNAMPort->bStoreMetaDataInBuffer = OMX_FALSE;
    } else {
        NAM_OSAL_Log(NAM_LOG_TRACE, "enable StoreMetaDataInBuffers");
        pNAMPort->bStoreMetaDataInBuffer = OMX_TRUE;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_BOOL isMetadataBufferTypeGrallocSource(OMX_BYTE pInputDataBuffer)
{
    OMX_U32 type = getMetadataBufferType(pInputDataBuffer);

    if (type == kMetadataBufferTypeGrallocSource)
        return OMX_TRUE;
    else
        return OMX_FALSE;
}

OMX_ERRORTYPE preprocessMetaDataInBuffers(OMX_HANDLETYPE hComponent, OMX_BYTE pInputDataBuffer, BUFFER_ADDRESS_INFO *pInputInfo)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    NAM_OMX_BASECOMPONENT *pNAMComponent = NULL;
    NAM_OMX_BASEPORT      *pNAMPort = NULL;
    OMX_U32                type = 0;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pNAMPort = &pNAMComponent->pNAMPort[INPUT_PORT_INDEX];

    type = getMetadataBufferType(pInputDataBuffer);
    if (type == kMetadataBufferTypeCameraSource) {
        NAM_OSAL_Memcpy(&pInputInfo->YPhyAddr, pInputDataBuffer + 4, sizeof(void *));
        NAM_OSAL_Memcpy(&pInputInfo->CPhyAddr, pInputDataBuffer + 4 + sizeof(void *), sizeof(void *));
    } else if (type == kMetadataBufferTypeGrallocSource){
        IMG_gralloc_module_public_t *module = (IMG_gralloc_module_public_t *)pNAMPort->pIMGGrallocModule;
        OMX_PTR pUnreadableBuffer = NULL;
        OMX_PTR pReadableBuffer = NULL;
        OMX_PTR pVirAddrs[3];
        int err = 0;

        pVirAddrs[0] = pNAMComponent->processData[INPUT_PORT_INDEX].specificBufferHeader.YVirAddr;
        pVirAddrs[1] = pNAMComponent->processData[INPUT_PORT_INDEX].specificBufferHeader.CVirAddr;
        pVirAddrs[2] = NULL;

        if (module == NULL) {
            err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t **)&module);
            if(err) {
                NAM_OSAL_Log(NAM_LOG_ERROR, "hw_get_module failed (err=%d)\n", err);
                ret = OMX_ErrorUndefined;
                goto EXIT;
            }
            pNAMPort->pIMGGrallocModule = (OMX_PTR)module;
        }

        /**************************************/
        /*     IMG CSC RGB to NV12            */
        /**************************************/
        buffer_handle_t buf = *((buffer_handle_t *) (pInputDataBuffer + 4));
        NAM_OSAL_Log(NAM_LOG_TRACE, "buffer handle %p)\n", buf);
        err = module->Blit(module, buf, pVirAddrs, HAL_PIXEL_FORMAT_C110_NV12);
        if(err) {
            NAM_OSAL_Log(NAM_LOG_ERROR, "module->Blit() failed (err=%d)\n", err);
            ret = OMX_ErrorUndefined;
            goto EXIT;
        }

        pInputInfo->YPhyAddr = pNAMComponent->processData[INPUT_PORT_INDEX].specificBufferHeader.YPhyAddr;
        pInputInfo->CPhyAddr = pNAMComponent->processData[INPUT_PORT_INDEX].specificBufferHeader.CPhyAddr;
    } else {
        ret = OMX_ErrorNotImplemented;
        goto EXIT;
    }

EXIT:
    FunctionOut();

    return ret;
}

#endif

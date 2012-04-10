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
 * @file    NAM_OMX_H264dec.h
 * @brief
 * @author    SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version    1.0
 * @history
 *   2010.7.15 : Create
 */

#ifndef NAM_OMX_H264_DEC_COMPONENT
#define NAM_OMX_H264_DEC_COMPONENT

#include "NAM_OMX_Def.h"
#include "OMX_Component.h"
#include "OMX_Video.h"


typedef struct _NAM_DMAI_H264DEC_HANDLE
{
    OMX_HANDLETYPE hDMAIHandle;
    OMX_PTR pDMAIStreamBuffer;
    OMX_PTR pDMAIStreamPhyBuffer;
    OMX_U32    indexTimestamp;
    OMX_BOOL bConfiguredDMAI;
    OMX_BOOL bThumbnailMode;
    OMX_S32  returnCodec;
} NAM_DMAI_H264DEC_HANDLE;

typedef struct _NAM_H264DEC_HANDLE
{
    /* OMX Codec specific */
    OMX_VIDEO_PARAM_AVCTYPE AVCComponent[ALL_PORT_NUM];
    OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE errorCorrectionType[ALL_PORT_NUM];

    /* NAM DMAI Codec specific */
    NAM_DMAI_H264DEC_HANDLE hDMAIH264Handle;

    /* For Non-Block mode */
    NAM_DMAI_NBDEC_THREAD NBDecThread;
    OMX_BOOL bFirstFrame;
    DMAI_DEC_INPUT_BUFFER DMAIDecInputBuffer[DMAI_INPUT_BUFFER_NUM_MAX];
    OMX_U32  indexInputBuffer;
} NAM_H264DEC_HANDLE;

#ifdef __cplusplus
extern "C" {
#endif

OSCL_EXPORT_REF OMX_ERRORTYPE NAM_OMX_ComponentInit(OMX_HANDLETYPE hComponent, OMX_STRING componentName);
OMX_ERRORTYPE NAM_OMX_ComponentDeinit(OMX_HANDLETYPE hComponent);

#ifdef __cplusplus
};
#endif

#endif

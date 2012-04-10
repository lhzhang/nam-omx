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
 * @file        NAM_OMX_Mpeg4dec.h
 * @brief
 * @author      Yunji Kim (yunji.kim@samsung.com)
 * @version     1.0
 * @history
 *   2010.7.15 : Create
 */

#ifndef NAM_OMX_MPEG4_DEC_COMPONENT
#define NAM_OMX_MPEG4_DEC_COMPONENT

#include "NAM_OMX_Def.h"
#include "OMX_Component.h"


typedef enum _CODEC_TYPE
{
    CODEC_TYPE_H263,
    CODEC_TYPE_MPEG4
} CODEC_TYPE;

/*
 * This structure is the same as BitmapInfoHhr struct in pv_avifile_typedefs.h file
 */
typedef struct _BitmapInfoHhr
{
    OMX_U32    BiSize;
    OMX_U32    BiWidth;
    OMX_U32    BiHeight;
    OMX_U16    BiPlanes;
    OMX_U16    BiBitCount;
    OMX_U32    BiCompression;
    OMX_U32    BiSizeImage;
    OMX_U32    BiXPelsPerMeter;
    OMX_U32    BiYPelsPerMeter;
    OMX_U32    BiClrUsed;
    OMX_U32    BiClrImportant;
} BitmapInfoHhr;

typedef struct _NAM_DMAI_MPEG4_HANDLE
{
    OMX_HANDLETYPE hDMAIHandle;
    OMX_PTR        pDMAIStreamBuffer;
    OMX_PTR        pDMAIStreamPhyBuffer;
    OMX_U32        indexTimestamp;
    OMX_BOOL       bConfiguredDMAI;
    OMX_BOOL       bThumbnailMode;
    CODEC_TYPE     codecType;
    OMX_S32        returnCodec;
} NAM_DMAI_MPEG4_HANDLE;

typedef struct _NAM_MPEG4_HANDLE
{
    /* OMX Codec specific */
    OMX_VIDEO_PARAM_H263TYPE  h263Component[ALL_PORT_NUM];
    OMX_VIDEO_PARAM_MPEG4TYPE mpeg4Component[ALL_PORT_NUM];
    OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE errorCorrectionType[ALL_PORT_NUM];

    /* NAM DMAI Codec specific */
    NAM_DMAI_MPEG4_HANDLE      hDMAIMpeg4Handle;

    /* For Non-Block mode */
    NAM_DMAI_NBDEC_THREAD NBDecThread;
    OMX_BOOL bFirstFrame;
    DMAI_DEC_INPUT_BUFFER DMAIDecInputBuffer[DMAI_INPUT_BUFFER_NUM_MAX];
    OMX_U32  indexInputBuffer;
} NAM_MPEG4_HANDLE;

#ifdef __cplusplus
extern "C" {
#endif

OSCL_EXPORT_REF OMX_ERRORTYPE NAM_OMX_ComponentInit(OMX_HANDLETYPE hComponent, OMX_STRING componentName);
                OMX_ERRORTYPE NAM_OMX_ComponentDeinit(OMX_HANDLETYPE hComponent);

#ifdef __cplusplus
};
#endif

#endif

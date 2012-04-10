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
 * @file    NAM_OMX_Def.h
 * @brief    NAM_OMX specific define
 * @author    SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version    1.0
 * @history
 *   2010.7.15 : Create
 */

#ifndef NAM_OMX_DEF
#define NAM_OMX_DEF

#include "OMX_Types.h"
#include "OMX_IVCommon.h"

#define VERSIONMAJOR_NUMBER                1
#define VERSIONMINOR_NUMBER                0
#define REVISION_NUMBER                    0
#define STEP_NUMBER                        0


#define MAX_OMX_COMPONENT_NUM              20
#define MAX_OMX_COMPONENT_ROLE_NUM         10
#define MAX_OMX_COMPONENT_NAME_SIZE        OMX_MAX_STRINGNAME_SIZE
#define MAX_OMX_COMPONENT_ROLE_SIZE        OMX_MAX_STRINGNAME_SIZE
#define MAX_OMX_COMPONENT_LIBNAME_SIZE     OMX_MAX_STRINGNAME_SIZE * 2
#define MAX_OMX_MIMETYPE_SIZE              OMX_MAX_STRINGNAME_SIZE

#define MAX_TIMESTAMP        17
#define MAX_FLAGS            17

#define USE_ANDROID_EXTENSION


typedef enum _NAM_CODEC_TYPE
{
    SW_CODEC,
    HW_VIDEO_CODEC,
    HW_AUDIO_CODEC
} NAM_CODEC_TYPE;

typedef struct _NAM_OMX_PRIORITYMGMTTYPE
{
    OMX_U32 nGroupPriority; /* the value 0 represents the highest priority */
                            /* for a group of components                   */
    OMX_U32 nGroupID;
} NAM_OMX_PRIORITYMGMTTYPE;

typedef enum _NAM_OMX_INDEXTYPE
{
#define NAM_INDEX_PARAM_ENABLE_THUMBNAIL "OMX.NAM.index.ThumbnailMode"
    OMX_IndexVendorThumbnailMode        = 0x7F000001,

    /* for Android Native Window */
#define NAM_INDEX_PARAM_ENABLE_ANB "OMX.google.android.index.enableAndroidNativeBuffers"
    OMX_IndexParamEnableAndroidBuffers    = 0x7F000011,
#define NAM_INDEX_PARAM_GET_ANB "OMX.google.android.index.getAndroidNativeBufferUsage"
    OMX_IndexParamGetAndroidNativeBuffer  = 0x7F000012,
#define NAM_INDEX_PARAM_USE_ANB "OMX.google.android.index.useAndroidNativeBuffer"
    OMX_IndexParamUseAndroidNativeBuffer  = 0x7F000013,
    /* for Android Store Metadata Inbuffer */
#define NAM_INDEX_PARAM_STORE_METADATA_BUFFER "OMX.google.android.index.storeMetaDataInBuffers"
    OMX_IndexParamStoreMetaDataBuffer     = 0x7F000014,

    /* for Android PV OpenCore*/
    OMX_COMPONENT_CAPABILITY_TYPE_INDEX = 0xFF7A347
} NAM_OMX_INDEXTYPE;

typedef enum _NAM_OMX_ERRORTYPE
{
    OMX_ErrorNoEOF = 0x90000001,
    OMX_ErrorInputDataDecodeYet,
    OMX_ErrorInputDataEncodeYet,
    OMX_ErrorMFCInit
} NAM_OMX_ERRORTYPE;

typedef enum _NAM_OMX_COMMANDTYPE
{
    NAM_OMX_CommandComponentDeInit = 0x7F000001,
    NAM_OMX_CommandEmptyBuffer,
    NAM_OMX_CommandFillBuffer
} NAM_OMX_COMMANDTYPE;

typedef enum _NAM_OMX_TRANS_STATETYPE {
    NAM_OMX_TransStateInvalid,
    NAM_OMX_TransStateLoadedToIdle,
    NAM_OMX_TransStateIdleToExecuting,
    NAM_OMX_TransStateExecutingToIdle,
    NAM_OMX_TransStateIdleToLoaded,
    NAM_OMX_TransStateMax = 0X7FFFFFFF
} NAM_OMX_TRANS_STATETYPE;

typedef enum _NAM_OMX_COLOR_FORMATTYPE {
    OMX_NAM_COLOR_FormatNV12TPhysicalAddress = 0x7F000001, /**< Reserved region for introducing Vendor Extensions */
    /* for Android Native Window */
    OMX_NAM_COLOR_FormatANBYUV420SemiPlanar = 0x100,
    /* for Android surface texture encode */
    OMX_COLOR_FormatAndroidOpaque = 0x7F000789
}NAM_OMX_COLOR_FORMATTYPE;

typedef enum _NAM_OMX_SUPPORTFORMAT_TYPE
{
    supportFormat_0 = 0x00,
    supportFormat_1,
    supportFormat_2,
    supportFormat_3
} NAM_OMX_SUPPORTFORMAT_TYPE;


/* for Android */
typedef struct _OMXComponentCapabilityFlagsType
{
    /* OMX COMPONENT CAPABILITY RELATED MEMBERS */
    OMX_BOOL iIsOMXComponentMultiThreaded;
    OMX_BOOL iOMXComponentSupportsExternalOutputBufferAlloc;
    OMX_BOOL iOMXComponentSupportsExternalInputBufferAlloc;
    OMX_BOOL iOMXComponentSupportsMovableInputBuffers;
    OMX_BOOL iOMXComponentSupportsPartialFrames;
    OMX_BOOL iOMXComponentUsesNALStartCodes;
    OMX_BOOL iOMXComponentCanHandleIncompleteFrames;
    OMX_BOOL iOMXComponentUsesFullAVCFrames;
} OMXComponentCapabilityFlagsType;

typedef struct _NAM_OMX_VIDEO_PROFILELEVEL
{
    OMX_S32  profile;
    OMX_S32  level;
} NAM_OMX_VIDEO_PROFILELEVEL;


#ifndef __OMX_EXPORTS
#define __OMX_EXPORTS
#define NAM_EXPORT_REF __attribute__((visibility("default")))
#define NAM_IMPORT_REF __attribute__((visibility("default")))
#endif

#endif

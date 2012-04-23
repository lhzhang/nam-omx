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
 * @file      NAM_OMX_Mpeg4dec.c
 * @brief
 * @author    Yunji Kim (yunji.kim@samsung.com)
 * @version   1.0
 * @history
 *   2010.7.15 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "NAM_OMX_Macros.h"
#include "NAM_OMX_Basecomponent.h"
#include "NAM_OMX_Baseport.h"
#include "NAM_OMX_Vdec.h"
#include "library_register.h"
#include "NAM_OMX_Mpeg4dec.h"
#include "SsbSipDmaiApi.h"
#include "color_space_convertor.h"

#undef  NAM_LOG_TAG
#define NAM_LOG_TAG    "NAM_MPEG4_DEC"
#define NAM_LOG_OFF
#include "NAM_OSAL_Log.h"

//#define FULL_FRAME_SEARCH

/* MPEG4 Decoder Supported Levels & profiles */
NAM_OMX_VIDEO_PROFILELEVEL supportedMPEG4ProfileLevels[] ={
    {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level0},
    {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level0b},
    {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level1},
    {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level2},
    {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level3},
    {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level4},
    {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level4a},
    {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level5},
    {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level0},
    {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level0b},
    {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level1},
    {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level2},
    {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level3},
    {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level4},
    {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level4a},
    {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level5}};

/* H.263 Decoder Supported Levels & profiles */
NAM_OMX_VIDEO_PROFILELEVEL supportedH263ProfileLevels[] = {
    /* Baseline (Profile 0) */
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level10},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level20},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level30},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level40},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level45},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level50},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level60},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level70},
    /* Profile 1 */
    {OMX_VIDEO_H263ProfileH320Coding, OMX_VIDEO_H263Level10},
    {OMX_VIDEO_H263ProfileH320Coding, OMX_VIDEO_H263Level20},
    {OMX_VIDEO_H263ProfileH320Coding, OMX_VIDEO_H263Level30},
    {OMX_VIDEO_H263ProfileH320Coding, OMX_VIDEO_H263Level40},
    {OMX_VIDEO_H263ProfileH320Coding, OMX_VIDEO_H263Level45},
    {OMX_VIDEO_H263ProfileH320Coding, OMX_VIDEO_H263Level50},
    {OMX_VIDEO_H263ProfileH320Coding, OMX_VIDEO_H263Level60},
    {OMX_VIDEO_H263ProfileH320Coding, OMX_VIDEO_H263Level70},
    /* Profile 2 */
    {OMX_VIDEO_H263ProfileBackwardCompatible, OMX_VIDEO_H263Level10},
    {OMX_VIDEO_H263ProfileBackwardCompatible, OMX_VIDEO_H263Level20},
    {OMX_VIDEO_H263ProfileBackwardCompatible, OMX_VIDEO_H263Level30},
    {OMX_VIDEO_H263ProfileBackwardCompatible, OMX_VIDEO_H263Level40},
    {OMX_VIDEO_H263ProfileBackwardCompatible, OMX_VIDEO_H263Level45},
    {OMX_VIDEO_H263ProfileBackwardCompatible, OMX_VIDEO_H263Level50},
    {OMX_VIDEO_H263ProfileBackwardCompatible, OMX_VIDEO_H263Level60},
    {OMX_VIDEO_H263ProfileBackwardCompatible, OMX_VIDEO_H263Level70},
    /* Profile 3, restricted up to SD resolution */
    {OMX_VIDEO_H263ProfileISWV2, OMX_VIDEO_H263Level10},
    {OMX_VIDEO_H263ProfileISWV2, OMX_VIDEO_H263Level20},
    {OMX_VIDEO_H263ProfileISWV2, OMX_VIDEO_H263Level30},
    {OMX_VIDEO_H263ProfileISWV2, OMX_VIDEO_H263Level40},
    {OMX_VIDEO_H263ProfileISWV2, OMX_VIDEO_H263Level45},
    {OMX_VIDEO_H263ProfileISWV2, OMX_VIDEO_H263Level50},
    {OMX_VIDEO_H263ProfileISWV2, OMX_VIDEO_H263Level60},
    {OMX_VIDEO_H263ProfileISWV2, OMX_VIDEO_H263Level70}};

static OMX_HANDLETYPE ghDMAIHandle = NULL;
static OMX_BOOL gbFIMV1 = OMX_FALSE;

static int Check_Mpeg4_Frame(OMX_U8 *pInputStream, OMX_U32 buffSize, OMX_U32 flag, OMX_BOOL bPreviousFrameEOF, OMX_BOOL *pbEndOfFrame)
{
    int len, readStream;
    unsigned startCode;
    OMX_BOOL bFrameStart;

    len = 0;
    bFrameStart = OMX_FALSE;

    if (flag & OMX_BUFFERFLAG_CODECCONFIG) {
        if (*pInputStream == 0x03) { /* FIMV1 */
            if (ghDMAIHandle != NULL) {
                BitmapInfoHhr *pInfoHeader;
                SSBSIP_DMAI_IMG_RESOLUTION imgResolution;

                pInfoHeader = (BitmapInfoHhr *)(pInputStream + 1);
                imgResolution.width = pInfoHeader->BiWidth;
                imgResolution.height = pInfoHeader->BiHeight;
                SsbSipDmaiDecSetConfig(ghDMAIHandle, DMAI_DEC_SETCONF_FIMV1_WIDTH_HEIGHT, &imgResolution);

                NAM_OSAL_Log(NAM_LOG_TRACE, "width(%d), height(%d)", imgResolution.width, imgResolution.height);
                gbFIMV1 = OMX_TRUE;
                *pbEndOfFrame = OMX_TRUE;
                return buffSize;
            }
        }
    }

    if (gbFIMV1) {
        *pbEndOfFrame = OMX_TRUE;
        return buffSize;
    }

    if (bPreviousFrameEOF == OMX_FALSE)
        bFrameStart = OMX_TRUE;

    startCode = 0xFFFFFFFF;
    if (bFrameStart == OMX_FALSE) {
        /* find VOP start code */
        while(startCode != 0x1B6) {
            readStream = *(pInputStream + len);
            startCode = (startCode << 8) | readStream;
            len++;
            if (len > buffSize)
                goto EXIT;
        }
    }

    /* find next VOP start code */
    startCode = 0xFFFFFFFF;
    while ((startCode != 0x1B6)) {
        readStream = *(pInputStream + len);
        startCode = (startCode << 8) | readStream;
        len++;
        if (len > buffSize)
            goto EXIT;
    }

    *pbEndOfFrame = OMX_TRUE;

    NAM_OSAL_Log(NAM_LOG_TRACE, "1. Check_Mpeg4_Frame returned EOF = %d, len = %d, buffSize = %d", *pbEndOfFrame, len - 4, buffSize);

    return len - 4;

EXIT :
    *pbEndOfFrame = OMX_FALSE;

    NAM_OSAL_Log(NAM_LOG_TRACE, "2. Check_Mpeg4_Frame returned EOF = %d, len = %d, buffSize = %d", *pbEndOfFrame, len - 1, buffSize);

    return --len;
}

static int Check_H263_Frame(OMX_U8 *pInputStream, OMX_U32 buffSize, OMX_U32 flag, OMX_BOOL bPreviousFrameEOF, OMX_BOOL *pbEndOfFrame)
{
    int len, readStream;
    unsigned startCode;
    OMX_BOOL bFrameStart = 0;
    unsigned pTypeMask = 0x03;
    unsigned pType = 0;

    len = 0;
    bFrameStart = OMX_FALSE;

    if (bPreviousFrameEOF == OMX_FALSE)
        bFrameStart = OMX_TRUE;

    startCode = 0xFFFFFFFF;
    if (bFrameStart == OMX_FALSE) {
        /* find PSC(Picture Start Code) : 0000 0000 0000 0000 1000 00 */
        while (((startCode << 8 >> 10) != 0x20) || (pType != 0x02)) {
            readStream = *(pInputStream + len);
            startCode = (startCode << 8) | readStream;

            readStream = *(pInputStream + len + 1);
            pType = readStream & pTypeMask;

            len++;
            if (len > buffSize)
                goto EXIT;
        }
    }

    /* find next PSC */
    startCode = 0xFFFFFFFF;
    pType = 0;
    while (((startCode << 8 >> 10) != 0x20) || (pType != 0x02)) {
        readStream = *(pInputStream + len);
        startCode = (startCode << 8) | readStream;

        readStream = *(pInputStream + len + 1);
        pType = readStream & pTypeMask;

        len++;
        if (len > buffSize)
            goto EXIT;
    }

    *pbEndOfFrame = OMX_TRUE;

    NAM_OSAL_Log(NAM_LOG_TRACE, "1. Check_H263_Frame returned EOF = %d, len = %d, iBuffSize = %d", *pbEndOfFrame, len - 3, buffSize);

    return len - 3;

EXIT :

    *pbEndOfFrame = OMX_FALSE;

    NAM_OSAL_Log(NAM_LOG_TRACE, "2. Check_H263_Frame returned EOF = %d, len = %d, iBuffSize = %d", *pbEndOfFrame, len - 1, buffSize);

    return --len;
}

OMX_BOOL Check_Stream_PrefixCode(OMX_U8 *pInputStream, OMX_U32 streamSize, CODEC_TYPE codecType)
{
    switch (codecType) {
    case CODEC_TYPE_MPEG4:
        if (gbFIMV1) {
            return OMX_TRUE;
        } else {
            if (streamSize < 3) {
                return OMX_FALSE;
            } else if ((pInputStream[0] == 0x00) &&
                       (pInputStream[1] == 0x00) &&
                       (pInputStream[2] == 0x01)) {
                return OMX_TRUE;
            } else {
                return OMX_FALSE;
            }
        }
        break;
    case CODEC_TYPE_H263:
        if (streamSize > 0)
            return OMX_TRUE;
        else
            return OMX_FALSE;
    default:
        NAM_OSAL_Log(NAM_LOG_WARNING, "%s: undefined codec type (%d)", __FUNCTION__, codecType);
        return OMX_FALSE;
    }
}

OMX_ERRORTYPE NAM_DMAI_Mpeg4Dec_GetParameter(
    OMX_IN    OMX_HANDLETYPE hComponent,
    OMX_IN    OMX_INDEXTYPE  nParamIndex,
    OMX_INOUT OMX_PTR        pComponentParameterStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    NAM_OMX_BASECOMPONENT *pNAMComponent = NULL;

    FunctionIn();

    if (hComponent == NULL || pComponentParameterStructure == NULL) {
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

    switch (nParamIndex) {
    case OMX_IndexParamVideoMpeg4:
    {
        OMX_VIDEO_PARAM_MPEG4TYPE *pDstMpeg4Param = (OMX_VIDEO_PARAM_MPEG4TYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_MPEG4TYPE *pSrcMpeg4Param = NULL;
        NAM_MPEG4_HANDLE          *pMpeg4Dec = NULL;
        ret = NAM_OMX_Check_SizeVersion(pDstMpeg4Param, sizeof(OMX_VIDEO_PARAM_MPEG4TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstMpeg4Param->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Dec = (NAM_MPEG4_HANDLE *)pNAMComponent->hCodecHandle;
        pSrcMpeg4Param = &pMpeg4Dec->mpeg4Component[pDstMpeg4Param->nPortIndex];

        NAM_OSAL_Memcpy(pDstMpeg4Param, pSrcMpeg4Param, sizeof(OMX_VIDEO_PARAM_MPEG4TYPE));
    }
        break;
    case OMX_IndexParamVideoH263:
    {
        OMX_VIDEO_PARAM_H263TYPE  *pDstH263Param = (OMX_VIDEO_PARAM_H263TYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_H263TYPE  *pSrcH263Param = NULL;
        NAM_MPEG4_HANDLE          *pMpeg4Dec = NULL;
        ret = NAM_OMX_Check_SizeVersion(pDstH263Param, sizeof(OMX_VIDEO_PARAM_H263TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstH263Param->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Dec = (NAM_MPEG4_HANDLE *)pNAMComponent->hCodecHandle;
        pSrcH263Param = &pMpeg4Dec->h263Component[pDstH263Param->nPortIndex];

        NAM_OSAL_Memcpy(pDstH263Param, pSrcH263Param, sizeof(OMX_VIDEO_PARAM_H263TYPE));
    }
        break;
    case OMX_IndexParamStandardComponentRole:
    {
        OMX_S32 codecType;
        OMX_PARAM_COMPONENTROLETYPE *pComponentRole = (OMX_PARAM_COMPONENTROLETYPE *)pComponentParameterStructure;

        ret = NAM_OMX_Check_SizeVersion(pComponentRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        codecType = ((NAM_MPEG4_HANDLE *)(pNAMComponent->hCodecHandle))->hDMAIMpeg4Handle.codecType;
        if (codecType == CODEC_TYPE_MPEG4)
            NAM_OSAL_Strcpy((char *)pComponentRole->cRole, NAM_OMX_COMPONENT_MPEG4_DEC_ROLE);
        else
            NAM_OSAL_Strcpy((char *)pComponentRole->cRole, NAM_OMX_COMPONENT_H263_DEC_ROLE);
    }
        break;
    case OMX_IndexParamVideoProfileLevelQuerySupported:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pDstProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pComponentParameterStructure;
        NAM_OMX_VIDEO_PROFILELEVEL       *pProfileLevel = NULL;
        OMX_U32                           maxProfileLevelNum = 0;
        OMX_S32                           codecType;

        ret = NAM_OMX_Check_SizeVersion(pDstProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        codecType = ((NAM_MPEG4_HANDLE *)(pNAMComponent->hCodecHandle))->hDMAIMpeg4Handle.codecType;
        if (codecType == CODEC_TYPE_MPEG4) {
            pProfileLevel = supportedMPEG4ProfileLevels;
            maxProfileLevelNum = sizeof(supportedMPEG4ProfileLevels) / sizeof(NAM_OMX_VIDEO_PROFILELEVEL);
        } else {
            pProfileLevel = supportedH263ProfileLevels;
            maxProfileLevelNum = sizeof(supportedH263ProfileLevels) / sizeof(NAM_OMX_VIDEO_PROFILELEVEL);
        }

        if (pDstProfileLevel->nProfileIndex >= maxProfileLevelNum) {
            ret = OMX_ErrorNoMore;
            goto EXIT;
        }

        pProfileLevel += pDstProfileLevel->nProfileIndex;
        pDstProfileLevel->eProfile = pProfileLevel->profile;
        pDstProfileLevel->eLevel = pProfileLevel->level;
    }
        break;
    case OMX_IndexParamVideoProfileLevelCurrent:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pDstProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_MPEG4TYPE        *pSrcMpeg4Param = NULL;
        OMX_VIDEO_PARAM_H263TYPE         *pSrcH263Param = NULL;
        NAM_MPEG4_HANDLE                 *pMpeg4Dec = NULL;
        OMX_S32                           codecType;

        ret = NAM_OMX_Check_SizeVersion(pDstProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Dec = (NAM_MPEG4_HANDLE *)pNAMComponent->hCodecHandle;
        codecType = pMpeg4Dec->hDMAIMpeg4Handle.codecType;
        if (codecType == CODEC_TYPE_MPEG4) {
            pSrcMpeg4Param = &pMpeg4Dec->mpeg4Component[pDstProfileLevel->nPortIndex];
            pDstProfileLevel->eProfile = pSrcMpeg4Param->eProfile;
            pDstProfileLevel->eLevel = pSrcMpeg4Param->eLevel;
        } else {
            pSrcH263Param = &pMpeg4Dec->h263Component[pDstProfileLevel->nPortIndex];
            pDstProfileLevel->eProfile = pSrcH263Param->eProfile;
            pDstProfileLevel->eLevel = pSrcH263Param->eLevel;
        }
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = NULL;
        NAM_MPEG4_HANDLE                    *pMpeg4Dec = NULL;

        ret = NAM_OMX_Check_SizeVersion(pDstErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstErrorCorrectionType->nPortIndex != INPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Dec = (NAM_MPEG4_HANDLE *)pNAMComponent->hCodecHandle;
        pSrcErrorCorrectionType = &pMpeg4Dec->errorCorrectionType[INPUT_PORT_INDEX];

        pDstErrorCorrectionType->bEnableHEC = pSrcErrorCorrectionType->bEnableHEC;
        pDstErrorCorrectionType->bEnableResync = pSrcErrorCorrectionType->bEnableResync;
        pDstErrorCorrectionType->nResynchMarkerSpacing = pSrcErrorCorrectionType->nResynchMarkerSpacing;
        pDstErrorCorrectionType->bEnableDataPartitioning = pSrcErrorCorrectionType->bEnableDataPartitioning;
        pDstErrorCorrectionType->bEnableRVLC = pSrcErrorCorrectionType->bEnableRVLC;
    }
        break;
    default:
        ret = NAM_OMX_VideoDecodeGetParameter(hComponent, nParamIndex, pComponentParameterStructure);
        break;
    }
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_DMAI_Mpeg4Dec_SetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        pComponentParameterStructure)
{
    OMX_ERRORTYPE           ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    NAM_OMX_BASECOMPONENT *pNAMComponent = NULL;

    FunctionIn();

    if (hComponent == NULL || pComponentParameterStructure == NULL) {
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

    switch (nIndex) {
    case OMX_IndexParamVideoMpeg4:
    {
        OMX_VIDEO_PARAM_MPEG4TYPE *pDstMpeg4Param = NULL;
        OMX_VIDEO_PARAM_MPEG4TYPE *pSrcMpeg4Param = (OMX_VIDEO_PARAM_MPEG4TYPE *)pComponentParameterStructure;
        NAM_MPEG4_HANDLE          *pMpeg4Dec = NULL;

        ret = NAM_OMX_Check_SizeVersion(pSrcMpeg4Param, sizeof(OMX_VIDEO_PARAM_MPEG4TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcMpeg4Param->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Dec = (NAM_MPEG4_HANDLE *)pNAMComponent->hCodecHandle;
        pDstMpeg4Param = &pMpeg4Dec->mpeg4Component[pSrcMpeg4Param->nPortIndex];

        NAM_OSAL_Memcpy(pDstMpeg4Param, pSrcMpeg4Param, sizeof(OMX_VIDEO_PARAM_MPEG4TYPE));
    }
        break;
    case OMX_IndexParamVideoH263:
    {
        OMX_VIDEO_PARAM_H263TYPE *pDstH263Param = NULL;
        OMX_VIDEO_PARAM_H263TYPE *pSrcH263Param = (OMX_VIDEO_PARAM_H263TYPE *)pComponentParameterStructure;
        NAM_MPEG4_HANDLE         *pMpeg4Dec = NULL;

        ret = NAM_OMX_Check_SizeVersion(pSrcH263Param, sizeof(OMX_VIDEO_PARAM_H263TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcH263Param->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Dec = (NAM_MPEG4_HANDLE *)pNAMComponent->hCodecHandle;
        pDstH263Param = &pMpeg4Dec->h263Component[pSrcH263Param->nPortIndex];

        NAM_OSAL_Memcpy(pDstH263Param, pSrcH263Param, sizeof(OMX_VIDEO_PARAM_H263TYPE));
    }
        break;
    case OMX_IndexParamStandardComponentRole:
    {
        OMX_PARAM_COMPONENTROLETYPE *pComponentRole = (OMX_PARAM_COMPONENTROLETYPE*)pComponentParameterStructure;

        ret = NAM_OMX_Check_SizeVersion(pComponentRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if ((pNAMComponent->currentState != OMX_StateLoaded) && (pNAMComponent->currentState != OMX_StateWaitForResources)) {
            ret = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
        }

        if (!NAM_OSAL_Strcmp((char*)pComponentRole->cRole, NAM_OMX_COMPONENT_MPEG4_DEC_ROLE)) {
            pNAMComponent->pNAMPort[INPUT_PORT_INDEX].portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG4;
            //((NAM_MPEG4_HANDLE *)(pNAMComponent->hCodecHandle))->hDMAIMpeg4Handle.codecType = CODEC_TYPE_MPEG4;
        } else if (!NAM_OSAL_Strcmp((char*)pComponentRole->cRole, NAM_OMX_COMPONENT_H263_DEC_ROLE)) {
            pNAMComponent->pNAMPort[INPUT_PORT_INDEX].portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingH263;
            //((NAM_MPEG4_HANDLE *)(pNAMComponent->hCodecHandle))->hDMAIMpeg4Handle.codecType = CODEC_TYPE_H263;
        } else {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }
    }
        break;
    case OMX_IndexParamPortDefinition:
    {
        OMX_PARAM_PORTDEFINITIONTYPE *pPortDefinition = (OMX_PARAM_PORTDEFINITIONTYPE *)pComponentParameterStructure;
        OMX_U32                       portIndex = pPortDefinition->nPortIndex;
        NAM_OMX_BASEPORT             *pNAMPort;
        OMX_U32 width, height, size;

        if (portIndex >= pNAMComponent->portParam.nPorts) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }
        ret = NAM_OMX_Check_SizeVersion(pPortDefinition, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
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
        if (pPortDefinition->nBufferCountActual < pNAMPort->portDefinition.nBufferCountMin) {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }

        NAM_OSAL_Memcpy(&pNAMPort->portDefinition, pPortDefinition, pPortDefinition->nSize);

        width = ((pNAMPort->portDefinition.format.video.nFrameWidth + 15) & (~15));
        height = ((pNAMPort->portDefinition.format.video.nFrameHeight + 15) & (~15));
        size = (width * height * 3) / 2;
        pNAMPort->portDefinition.format.video.nStride = width;
        pNAMPort->portDefinition.format.video.nSliceHeight = height;
        pNAMPort->portDefinition.nBufferSize = (size > pNAMPort->portDefinition.nBufferSize) ? size : pNAMPort->portDefinition.nBufferSize;

        if (portIndex == INPUT_PORT_INDEX) {
            NAM_OMX_BASEPORT *pNAMOutputPort = &pNAMComponent->pNAMPort[OUTPUT_PORT_INDEX];
            pNAMOutputPort->portDefinition.format.video.nFrameWidth = pNAMPort->portDefinition.format.video.nFrameWidth;
            pNAMOutputPort->portDefinition.format.video.nFrameHeight = pNAMPort->portDefinition.format.video.nFrameHeight;
            pNAMOutputPort->portDefinition.format.video.nStride = width;
            pNAMOutputPort->portDefinition.format.video.nSliceHeight = height;

            switch (pNAMOutputPort->portDefinition.format.video.eColorFormat) {
            case OMX_COLOR_FormatYUV420Planar:
            case OMX_COLOR_FormatYUV420SemiPlanar:
            case OMX_SEC_COLOR_FormatNV12TPhysicalAddress:
            case OMX_SEC_COLOR_FormatANBYUV420SemiPlanar:
                pNAMOutputPort->portDefinition.nBufferSize = (width * height * 3) / 2;
                break;
            default:
                NAM_OSAL_Log(NAM_LOG_ERROR, "Color format is not support!! use default YUV size!!");
                ret = OMX_ErrorUnsupportedSetting;
                break;
            }
        }
    }
        break;
    case OMX_IndexParamVideoProfileLevelCurrent:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pSrcProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_MPEG4TYPE        *pDstMpeg4Param = NULL;
        OMX_VIDEO_PARAM_H263TYPE         *pDstH263Param = NULL;
        NAM_MPEG4_HANDLE                 *pMpeg4Dec = NULL;
        OMX_S32                           codecType;

        ret = NAM_OMX_Check_SizeVersion(pSrcProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pSrcProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Dec = (NAM_MPEG4_HANDLE *)pNAMComponent->hCodecHandle;
        codecType = pMpeg4Dec->hDMAIMpeg4Handle.codecType;
        if (codecType == CODEC_TYPE_MPEG4) {
            /*
             * To do: Check validity of profile & level parameters
             */

            pDstMpeg4Param = &pMpeg4Dec->mpeg4Component[pSrcProfileLevel->nPortIndex];
            pDstMpeg4Param->eProfile = pSrcProfileLevel->eProfile;
            pDstMpeg4Param->eLevel = pSrcProfileLevel->eLevel;
        } else {
            /*
             * To do: Check validity of profile & level parameters
             */

            pDstH263Param = &pMpeg4Dec->h263Component[pSrcProfileLevel->nPortIndex];
            pDstH263Param->eProfile = pSrcProfileLevel->eProfile;
            pDstH263Param->eLevel = pSrcProfileLevel->eLevel;
        }
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = NULL;
        NAM_MPEG4_HANDLE                    *pMpeg4Dec = NULL;

        ret = NAM_OMX_Check_SizeVersion(pSrcErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcErrorCorrectionType->nPortIndex != INPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Dec = (NAM_MPEG4_HANDLE *)pNAMComponent->hCodecHandle;
        pDstErrorCorrectionType = &pMpeg4Dec->errorCorrectionType[INPUT_PORT_INDEX];

        pDstErrorCorrectionType->bEnableHEC = pSrcErrorCorrectionType->bEnableHEC;
        pDstErrorCorrectionType->bEnableResync = pSrcErrorCorrectionType->bEnableResync;
        pDstErrorCorrectionType->nResynchMarkerSpacing = pSrcErrorCorrectionType->nResynchMarkerSpacing;
        pDstErrorCorrectionType->bEnableDataPartitioning = pSrcErrorCorrectionType->bEnableDataPartitioning;
        pDstErrorCorrectionType->bEnableRVLC = pSrcErrorCorrectionType->bEnableRVLC;
    }
        break;
    default:
        ret = NAM_OMX_VideoDecodeSetParameter(hComponent, nIndex, pComponentParameterStructure);
        break;
    }
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_DMAI_Mpeg4Dec_GetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nIndex,
    OMX_PTR pComponentConfigStructure)
{
    OMX_ERRORTYPE           ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    NAM_OMX_BASECOMPONENT *pNAMComponent = NULL;

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

    if (pComponentConfigStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pNAMComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nIndex) {
    default:
        ret = NAM_OMX_GetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_DMAI_Mpeg4Dec_SetConfig(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        pComponentConfigStructure)
{
    OMX_ERRORTYPE           ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    NAM_OMX_BASECOMPONENT *pNAMComponent = NULL;

    FunctionIn();

    if (hComponent == NULL || pComponentConfigStructure == NULL) {
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

    switch (nIndex) {
    case OMX_IndexVendorThumbnailMode:
    {
        NAM_MPEG4_HANDLE  *pMpeg4Dec = (NAM_MPEG4_HANDLE *)pNAMComponent->hCodecHandle;

        pMpeg4Dec->hDMAIMpeg4Handle.bThumbnailMode = *((OMX_BOOL *)pComponentConfigStructure);
    }
        break;
    default:
        ret = NAM_OMX_SetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_DMAI_Mpeg4Dec_GetExtensionIndex(
    OMX_IN OMX_HANDLETYPE  hComponent,
    OMX_IN OMX_STRING      cParameterName,
    OMX_OUT OMX_INDEXTYPE *pIndexType)
{
    OMX_ERRORTYPE           ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    NAM_OMX_BASECOMPONENT *pNAMComponent = NULL;

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

    if ((cParameterName == NULL) || (pIndexType == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pNAMComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    if (NAM_OSAL_Strcmp(cParameterName, NAM_INDEX_PARAM_ENABLE_THUMBNAIL) == 0) {
        NAM_MPEG4_HANDLE *pMpeg4Dec = (NAM_MPEG4_HANDLE *)pNAMComponent->hCodecHandle;
        *pIndexType = OMX_IndexVendorThumbnailMode;
        ret = OMX_ErrorNone;
#ifdef USE_ANDROID_EXTENSION
    } else if (NAM_OSAL_Strcmp(cParameterName, NAM_INDEX_PARAM_ENABLE_ANB) == 0) {
        *pIndexType = OMX_IndexParamEnableAndroidBuffers;
        ret = OMX_ErrorNone;
    } else if (NAM_OSAL_Strcmp(cParameterName, NAM_INDEX_PARAM_GET_ANB) == 0) {
        *pIndexType = OMX_IndexParamGetAndroidNativeBuffer;
        ret = OMX_ErrorNone;
    } else if (NAM_OSAL_Strcmp(cParameterName, NAM_INDEX_PARAM_USE_ANB) == 0) {
        *pIndexType = OMX_IndexParamUseAndroidNativeBuffer;
        ret = OMX_ErrorNone;
#endif
    } else {
        ret = NAM_OMX_GetExtensionIndex(hComponent, cParameterName, pIndexType);
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_DMAI_Mpeg4Dec_ComponentRoleEnum(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_U8        *cRole,
    OMX_IN  OMX_U32        nIndex)
{
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE       *pOMXComponent = NULL;
    NAM_OMX_BASECOMPONENT   *pNAMComponent = NULL;
    OMX_S32                  codecType;

    FunctionIn();

    if ((hComponent == NULL) || (cRole == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (nIndex != (MAX_COMPONENT_ROLE_NUM - 1)) {
        ret = OMX_ErrorNoMore;
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

    codecType = ((NAM_MPEG4_HANDLE *)(pNAMComponent->hCodecHandle))->hDMAIMpeg4Handle.codecType;
    if (codecType == CODEC_TYPE_MPEG4)
        NAM_OSAL_Strcpy((char *)cRole, NAM_OMX_COMPONENT_MPEG4_DEC_ROLE);
    else
        NAM_OSAL_Strcpy((char *)cRole, NAM_OMX_COMPONENT_H263_DEC_ROLE);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_DMAI_DecodeThread(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    NAM_OMX_BASECOMPONENT *pNAMComponent = NULL;
    NAM_MPEG4_HANDLE      *pMpeg4Dec = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pMpeg4Dec = (NAM_MPEG4_HANDLE *)pNAMComponent->hCodecHandle;

    while (pMpeg4Dec->NBDecThread.bExitDecodeThread == OMX_FALSE) {
        NAM_OSAL_SemaphoreWait(pMpeg4Dec->NBDecThread.hDecFrameStart);

        if (pMpeg4Dec->NBDecThread.bExitDecodeThread == OMX_FALSE) {
            pMpeg4Dec->hDMAIMpeg4Handle.returnCodec = SsbSipDmaiDecExe(pMpeg4Dec->hDMAIMpeg4Handle.hDMAIHandle, pMpeg4Dec->NBDecThread.oneFrameSize);
            NAM_OSAL_SemaphorePost(pMpeg4Dec->NBDecThread.hDecFrameEnd);
        }
    }

EXIT:
    NAM_OSAL_ThreadExit(NULL);
    FunctionOut();

    return ret;
}

/* DMAI Init */
OMX_ERRORTYPE NAM_DMAI_Mpeg4Dec_Init(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    NAM_OMX_BASECOMPONENT *pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    NAM_MPEG4_HANDLE      *pMpeg4Dec = NULL;
    OMX_HANDLETYPE         hDMAIHandle = NULL;
    OMX_PTR                pStreamBuffer = NULL;
    OMX_PTR                pStreamPhyBuffer = NULL;

    FunctionIn();

    pMpeg4Dec = (NAM_MPEG4_HANDLE *)pNAMComponent->hCodecHandle;
    pMpeg4Dec->hDMAIMpeg4Handle.bConfiguredDMAI = OMX_FALSE;
    pNAMComponent->bUseFlagEOF = OMX_FALSE;
    pNAMComponent->bSaveFlagEOS = OMX_FALSE;

    /* DMAI(Multi Format Codec) decoder and CMM(Codec Memory Management) driver open */
    SSBIP_DMAI_BUFFER_TYPE buf_type = CACHE;
    hDMAIHandle = (OMX_PTR)SsbSipDmaiDecOpen(&buf_type);
    if (hDMAIHandle == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    ghDMAIHandle = pMpeg4Dec->hDMAIMpeg4Handle.hDMAIHandle = hDMAIHandle;

    /* Allocate decoder's input buffer */
    pStreamBuffer = SsbSipDmaiDecGetInBuf(hDMAIHandle, &pStreamPhyBuffer, DEFAULT_DMAI_INPUT_BUFFER_SIZE);
    if (pStreamBuffer == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    pMpeg4Dec->DMAIDecInputBuffer[0].VirAddr = pStreamBuffer;
    pMpeg4Dec->DMAIDecInputBuffer[0].PhyAddr = pStreamPhyBuffer;
    pMpeg4Dec->DMAIDecInputBuffer[0].bufferSize = DEFAULT_DMAI_INPUT_BUFFER_SIZE;
    pMpeg4Dec->DMAIDecInputBuffer[0].dataSize = 0;
    pMpeg4Dec->DMAIDecInputBuffer[1].VirAddr = (unsigned char *)pStreamBuffer + pMpeg4Dec->DMAIDecInputBuffer[0].bufferSize;
    pMpeg4Dec->DMAIDecInputBuffer[1].PhyAddr = (unsigned char *)pStreamPhyBuffer + pMpeg4Dec->DMAIDecInputBuffer[0].bufferSize;
    pMpeg4Dec->DMAIDecInputBuffer[1].bufferSize = DEFAULT_DMAI_INPUT_BUFFER_SIZE;
    pMpeg4Dec->DMAIDecInputBuffer[1].dataSize = 0;
    pMpeg4Dec->indexInputBuffer = 0;

    pMpeg4Dec->bFirstFrame = OMX_TRUE;

    pMpeg4Dec->NBDecThread.bExitDecodeThread = OMX_FALSE;
    pMpeg4Dec->NBDecThread.bDecoderRun = OMX_FALSE;
    pMpeg4Dec->NBDecThread.oneFrameSize = 0;
    NAM_OSAL_SemaphoreCreate(&(pMpeg4Dec->NBDecThread.hDecFrameStart));
    NAM_OSAL_SemaphoreCreate(&(pMpeg4Dec->NBDecThread.hDecFrameEnd));
    if (OMX_ErrorNone == NAM_OSAL_ThreadCreate(&pMpeg4Dec->NBDecThread.hNBDecodeThread,
                                                NAM_DMAI_DecodeThread,
                                                pOMXComponent)) {
        pMpeg4Dec->hDMAIMpeg4Handle.returnCodec = DMAI_RET_OK;
    }

    pMpeg4Dec->hDMAIMpeg4Handle.pDMAIStreamBuffer    = pMpeg4Dec->DMAIDecInputBuffer[0].VirAddr;
    pMpeg4Dec->hDMAIMpeg4Handle.pDMAIStreamPhyBuffer = pMpeg4Dec->DMAIDecInputBuffer[0].PhyAddr;
    pNAMComponent->processData[INPUT_PORT_INDEX].dataBuffer = pMpeg4Dec->DMAIDecInputBuffer[0].VirAddr;
    pNAMComponent->processData[INPUT_PORT_INDEX].allocSize = pMpeg4Dec->DMAIDecInputBuffer[0].bufferSize;

    NAM_OSAL_Memset(pNAMComponent->timeStamp, -19771003, sizeof(OMX_TICKS) * MAX_TIMESTAMP);
    NAM_OSAL_Memset(pNAMComponent->nFlags, 0, sizeof(OMX_U32) * MAX_FLAGS);
    pMpeg4Dec->hDMAIMpeg4Handle.indexTimestamp = 0;
    pNAMComponent->getAllDelayBuffer = OMX_FALSE;

EXIT:
    FunctionOut();

    return ret;
}

/* DMAI Terminate */
OMX_ERRORTYPE NAM_DMAI_Mpeg4Dec_Terminate(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    NAM_OMX_BASECOMPONENT *pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    NAM_MPEG4_HANDLE      *pMpeg4Dec = NULL;
    OMX_HANDLETYPE         hDMAIHandle = NULL;

    FunctionIn();

    pMpeg4Dec = (NAM_MPEG4_HANDLE *)pNAMComponent->hCodecHandle;
    hDMAIHandle = pMpeg4Dec->hDMAIMpeg4Handle.hDMAIHandle;

    pMpeg4Dec->hDMAIMpeg4Handle.pDMAIStreamBuffer    = NULL;
    pMpeg4Dec->hDMAIMpeg4Handle.pDMAIStreamPhyBuffer = NULL;
    pNAMComponent->processData[INPUT_PORT_INDEX].dataBuffer = NULL;
    pNAMComponent->processData[INPUT_PORT_INDEX].allocSize = 0;

    if (pMpeg4Dec->NBDecThread.hNBDecodeThread != NULL) {
        pMpeg4Dec->NBDecThread.bExitDecodeThread = OMX_TRUE;
        NAM_OSAL_SemaphorePost(pMpeg4Dec->NBDecThread.hDecFrameStart);
        NAM_OSAL_ThreadTerminate(pMpeg4Dec->NBDecThread.hNBDecodeThread);
        pMpeg4Dec->NBDecThread.hNBDecodeThread = NULL;
    }

    if(pMpeg4Dec->NBDecThread.hDecFrameEnd != NULL) {
        NAM_OSAL_SemaphoreTerminate(pMpeg4Dec->NBDecThread.hDecFrameEnd);
        pMpeg4Dec->NBDecThread.hDecFrameEnd = NULL;
    }

    if(pMpeg4Dec->NBDecThread.hDecFrameStart != NULL) {
        NAM_OSAL_SemaphoreTerminate(pMpeg4Dec->NBDecThread.hDecFrameStart);
        pMpeg4Dec->NBDecThread.hDecFrameStart = NULL;
    }

    if (hDMAIHandle != NULL) {
        SsbSipDmaiDecClose(hDMAIHandle);
        pMpeg4Dec->hDMAIMpeg4Handle.hDMAIHandle = NULL;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_DMAI_Mpeg4_Decode(OMX_COMPONENTTYPE *pOMXComponent, NAM_OMX_DATA *pInputData, NAM_OMX_DATA *pOutputData)
{
    OMX_ERRORTYPE              ret = OMX_ErrorNone;
    NAM_OMX_BASECOMPONENT     *pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    NAM_MPEG4_HANDLE          *pMpeg4Dec = (NAM_MPEG4_HANDLE *)pNAMComponent->hCodecHandle;
    OMX_HANDLETYPE             hDMAIHandle = pMpeg4Dec->hDMAIMpeg4Handle.hDMAIHandle;
    OMX_U32                    oneFrameSize = pInputData->dataLen;
    SSBSIP_DMAI_DEC_OUTPUT_INFO outputInfo;
    OMX_S32                    configValue = 0;
    int                        bufWidth = 0;
    int                        bufHeight = 0;
    OMX_BOOL                   outputDataValid = OMX_FALSE;

    FunctionIn();

    if (pMpeg4Dec->hDMAIMpeg4Handle.bConfiguredDMAI == OMX_FALSE) {
        SSBSIP_DMAI_CODEC_TYPE DMAICodecType;
        if (pMpeg4Dec->hDMAIMpeg4Handle.codecType == CODEC_TYPE_MPEG4) {
            if (gbFIMV1)
                DMAICodecType = FIMV1_DEC;
            else
                DMAICodecType = MPEG4_DEC;
        } else {
            DMAICodecType = H263_DEC;
        }

        if ((oneFrameSize <= 0) && (pInputData->nFlags & OMX_BUFFERFLAG_EOS)) {
            pOutputData->timeStamp = pInputData->timeStamp;
            pOutputData->nFlags = pInputData->nFlags;
            ret = OMX_ErrorNone;
            goto EXIT;
        }

        /* Set the number of extra buffer to prevent tearing */
        configValue = 0;
        SsbSipDmaiDecSetConfig(hDMAIHandle, DMAI_DEC_SETCONF_EXTRA_BUFFER_NUM, &configValue);

        /* Set mpeg4 deblocking filter enable */
        configValue = 1;
        SsbSipDmaiDecSetConfig(hDMAIHandle, DMAI_DEC_SETCONF_POST_ENABLE, &configValue);

        if (pMpeg4Dec->hDMAIMpeg4Handle.bThumbnailMode == OMX_TRUE) {
            configValue = 1;    // the number that you want to delay
            SsbSipDmaiDecSetConfig(hDMAIHandle, DMAI_DEC_SETCONF_DISPLAY_DELAY, &configValue);
        }

        pMpeg4Dec->hDMAIMpeg4Handle.returnCodec = SsbSipDmaiDecInit(hDMAIHandle, DMAICodecType, oneFrameSize);
        if (pMpeg4Dec->hDMAIMpeg4Handle.returnCodec == DMAI_RET_OK) {
            SSBSIP_DMAI_IMG_RESOLUTION imgResol;
            NAM_OMX_BASEPORT *pInputPort = &pNAMComponent->pNAMPort[INPUT_PORT_INDEX];

            if (SsbSipDmaiDecGetConfig(hDMAIHandle, DMAI_DEC_GETCONF_BUF_WIDTH_HEIGHT, &imgResol) != DMAI_RET_OK) {
                ret = OMX_ErrorDMAIInit;
                NAM_OSAL_Log(NAM_LOG_ERROR, "%s: SsbSipDmaiDecGetConfig failed", __FUNCTION__);
                goto EXIT;
            }

            /** Update Frame Size **/
            if ((pInputPort->portDefinition.format.video.nFrameWidth != imgResol.width) ||
                (pInputPort->portDefinition.format.video.nFrameHeight != imgResol.height)) {
                /* change width and height information */
                pInputPort->portDefinition.format.video.nFrameWidth = imgResol.width;
                pInputPort->portDefinition.format.video.nFrameHeight = imgResol.height;
                pInputPort->portDefinition.format.video.nStride      = ((imgResol.width + 15) & (~15));
                pInputPort->portDefinition.format.video.nSliceHeight = ((imgResol.height + 15) & (~15));

                NAM_UpdateFrameSize(pOMXComponent);

                /* Send Port Settings changed call back */
                (*(pNAMComponent->pCallbacks->EventHandler))
                      (pOMXComponent,
                       pNAMComponent->callbackData,
                       OMX_EventPortSettingsChanged, // The command was completed
                       OMX_DirOutput, // This is the port index
                       0,
                       NULL);
            }

            NAM_OSAL_Log(NAM_LOG_TRACE, "nFrameWidth(%d) nFrameHeight(%d), nStride(%d), nSliceHeight(%d)",
                    pInputPort->portDefinition.format.video.nFrameWidth,  pInputPort->portDefinition.format.video.nFrameHeight,
                    pInputPort->portDefinition.format.video.nStride, pInputPort->portDefinition.format.video.nSliceHeight);

            pMpeg4Dec->hDMAIMpeg4Handle.bConfiguredDMAI = OMX_TRUE;
            if (pMpeg4Dec->hDMAIMpeg4Handle.codecType == CODEC_TYPE_MPEG4) {
                pOutputData->timeStamp = pInputData->timeStamp;
                pOutputData->nFlags = pInputData->nFlags;
                ret = OMX_ErrorNone;
            } else {
                pOutputData->dataLen = 0;
                ret = OMX_ErrorInputDataDecodeYet;
            }
            goto EXIT;
        } else {
            NAM_OSAL_Log(NAM_LOG_ERROR, "%s: SsbSipDmaiDecInit failed", __FUNCTION__);
            ret = OMX_ErrorDMAIInit;    /* OMX_ErrorUndefined */
            goto EXIT;
        }
    }

#ifndef FULL_FRAME_SEARCH
    if ((pInputData->nFlags & OMX_BUFFERFLAG_ENDOFFRAME) &&
        (pNAMComponent->bUseFlagEOF == OMX_FALSE))
        pNAMComponent->bUseFlagEOF = OMX_TRUE;
#endif

    pNAMComponent->timeStamp[pMpeg4Dec->hDMAIMpeg4Handle.indexTimestamp] = pInputData->timeStamp;
    pNAMComponent->nFlags[pMpeg4Dec->hDMAIMpeg4Handle.indexTimestamp] = pInputData->nFlags;

    if ((pMpeg4Dec->hDMAIMpeg4Handle.returnCodec == DMAI_RET_OK) &&
        (pMpeg4Dec->bFirstFrame == OMX_FALSE)) {
        SSBSIP_DMAI_DEC_OUTBUF_STATUS status;
        OMX_S32 indexTimestamp = 0;

        /* wait for dmai decode done */
        if (pMpeg4Dec->NBDecThread.bDecoderRun == OMX_TRUE) {
            NAM_OSAL_SemaphoreWait(pMpeg4Dec->NBDecThread.hDecFrameEnd);
            pMpeg4Dec->NBDecThread.bDecoderRun = OMX_FALSE;
        }

        status = SsbSipDmaiDecGetOutBuf(hDMAIHandle, &outputInfo);
        bufWidth =  (outputInfo.img_width + 15) & (~15);
        bufHeight =  (outputInfo.img_height + 15) & (~15);

        if ((SsbSipDmaiDecGetConfig(hDMAIHandle, DMAI_DEC_GETCONF_FRAME_TAG, &indexTimestamp) != DMAI_RET_OK) ||
            (((indexTimestamp < 0) || (indexTimestamp >= MAX_TIMESTAMP)))) {
            pOutputData->timeStamp = pInputData->timeStamp;
            pOutputData->nFlags = pInputData->nFlags;
        } else {
            pOutputData->timeStamp = pNAMComponent->timeStamp[indexTimestamp];
            pOutputData->nFlags = pNAMComponent->nFlags[indexTimestamp];
        }
        NAM_OSAL_Log(NAM_LOG_TRACE, "timestamp %lld us (%.2f nams)", pOutputData->timeStamp, pOutputData->timeStamp / 1E6);

        if ((status == DMAI_GETOUTBUF_DISPLAY_DECODING) ||
            (status == DMAI_GETOUTBUF_DISPLAY_ONLY)) {
            outputDataValid = OMX_TRUE;
        }
        if (pOutputData->nFlags & OMX_BUFFERFLAG_EOS)
            outputDataValid = OMX_FALSE;

        if ((status == DMAI_GETOUTBUF_DISPLAY_ONLY) ||
            (pNAMComponent->getAllDelayBuffer == OMX_TRUE))
            ret = OMX_ErrorInputDataDecodeYet;

        if (status == DMAI_GETOUTBUF_DECODING_ONLY) {
            if (((pInputData->nFlags & OMX_BUFFERFLAG_EOS) != OMX_BUFFERFLAG_EOS) &&
                (pNAMComponent->bSaveFlagEOS == OMX_TRUE)) {
                pInputData->nFlags |= OMX_BUFFERFLAG_EOS;
                pNAMComponent->getAllDelayBuffer = OMX_TRUE;
                ret = OMX_ErrorInputDataDecodeYet;
            } else {
                ret = OMX_ErrorNone;
            }
            outputDataValid = OMX_FALSE;
        }

#ifdef FULL_FRAME_SEARCH
        if (((pInputData->nFlags & OMX_BUFFERFLAG_EOS) != OMX_BUFFERFLAG_EOS) &&
            (pNAMComponent->bSaveFlagEOS == OMX_TRUE)) {
            pInputData->nFlags |= OMX_BUFFERFLAG_EOS;
            pNAMComponent->getAllDelayBuffer = OMX_TRUE;
            ret = OMX_ErrorInputDataDecodeYet;
        } else
#endif
        if ((pInputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) {
            pInputData->nFlags = (pOutputData->nFlags & (~OMX_BUFFERFLAG_EOS));
            pNAMComponent->getAllDelayBuffer = OMX_TRUE;
            ret = OMX_ErrorInputDataDecodeYet;
        } else if ((pOutputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) {
            pNAMComponent->getAllDelayBuffer = OMX_FALSE;
            ret = OMX_ErrorNone;
        }
    } else {
        pOutputData->timeStamp = pInputData->timeStamp;
        pOutputData->nFlags = pInputData->nFlags;

        if ((pNAMComponent->bSaveFlagEOS == OMX_TRUE) ||
            (pNAMComponent->getAllDelayBuffer == OMX_TRUE) ||
            (pInputData->nFlags & OMX_BUFFERFLAG_EOS)) {
            pOutputData->nFlags |= OMX_BUFFERFLAG_EOS;
            pNAMComponent->getAllDelayBuffer = OMX_FALSE;
        }
        if ((pMpeg4Dec->bFirstFrame == OMX_TRUE) &&
            ((pOutputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS)) {
            pOutputData->nFlags = (pOutputData->nFlags & (~OMX_BUFFERFLAG_EOS));
        }

        outputDataValid = OMX_FALSE;

        /* ret = OMX_ErrorUndefined; */
            ret = OMX_ErrorNone;
    }

    if (ret == OMX_ErrorInputDataDecodeYet) {
        pMpeg4Dec->DMAIDecInputBuffer[pMpeg4Dec->indexInputBuffer].dataSize = oneFrameSize;
        pMpeg4Dec->indexInputBuffer++;
        pMpeg4Dec->indexInputBuffer %= DMAI_INPUT_BUFFER_NUM_MAX;
        pMpeg4Dec->hDMAIMpeg4Handle.pDMAIStreamBuffer    = pMpeg4Dec->DMAIDecInputBuffer[pMpeg4Dec->indexInputBuffer].VirAddr;
        pMpeg4Dec->hDMAIMpeg4Handle.pDMAIStreamPhyBuffer = pMpeg4Dec->DMAIDecInputBuffer[pMpeg4Dec->indexInputBuffer].PhyAddr;
        pNAMComponent->processData[INPUT_PORT_INDEX].dataBuffer = pMpeg4Dec->DMAIDecInputBuffer[pMpeg4Dec->indexInputBuffer].VirAddr;
        pNAMComponent->processData[INPUT_PORT_INDEX].allocSize = pMpeg4Dec->DMAIDecInputBuffer[pMpeg4Dec->indexInputBuffer].bufferSize;
        oneFrameSize = pMpeg4Dec->DMAIDecInputBuffer[pMpeg4Dec->indexInputBuffer].dataSize;
        //pInputData->dataLen = oneFrameSize;
        //pInputData->remainDataLen = oneFrameSize;
    }

    if ((Check_Stream_PrefixCode(pInputData->dataBuffer, pInputData->dataLen, pMpeg4Dec->hDMAIMpeg4Handle.codecType) == OMX_TRUE) &&
        ((pOutputData->nFlags & OMX_BUFFERFLAG_EOS) != OMX_BUFFERFLAG_EOS)) {
        SsbSipDmaiDecSetConfig(hDMAIHandle, DMAI_DEC_SETCONF_FRAME_TAG, &(pMpeg4Dec->hDMAIMpeg4Handle.indexTimestamp));
        pMpeg4Dec->hDMAIMpeg4Handle.indexTimestamp++;
        pMpeg4Dec->hDMAIMpeg4Handle.indexTimestamp %= MAX_TIMESTAMP;

        SsbSipDmaiDecSetInBuf(pMpeg4Dec->hDMAIMpeg4Handle.hDMAIHandle,
                             pMpeg4Dec->hDMAIMpeg4Handle.pDMAIStreamPhyBuffer,
                             pMpeg4Dec->hDMAIMpeg4Handle.pDMAIStreamBuffer,
                             pNAMComponent->processData[INPUT_PORT_INDEX].allocSize);

        pMpeg4Dec->DMAIDecInputBuffer[pMpeg4Dec->indexInputBuffer].dataSize = oneFrameSize;
        pMpeg4Dec->NBDecThread.oneFrameSize = oneFrameSize;

        /* dmai decode start */
        NAM_OSAL_SemaphorePost(pMpeg4Dec->NBDecThread.hDecFrameStart);
        pMpeg4Dec->NBDecThread.bDecoderRun = OMX_TRUE;
        pMpeg4Dec->hDMAIMpeg4Handle.returnCodec = DMAI_RET_OK;

        pMpeg4Dec->indexInputBuffer++;
        pMpeg4Dec->indexInputBuffer %= DMAI_INPUT_BUFFER_NUM_MAX;
        pMpeg4Dec->hDMAIMpeg4Handle.pDMAIStreamBuffer    = pMpeg4Dec->DMAIDecInputBuffer[pMpeg4Dec->indexInputBuffer].VirAddr;
        pMpeg4Dec->hDMAIMpeg4Handle.pDMAIStreamPhyBuffer = pMpeg4Dec->DMAIDecInputBuffer[pMpeg4Dec->indexInputBuffer].PhyAddr;
        pNAMComponent->processData[INPUT_PORT_INDEX].dataBuffer = pMpeg4Dec->DMAIDecInputBuffer[pMpeg4Dec->indexInputBuffer].VirAddr;
        pNAMComponent->processData[INPUT_PORT_INDEX].allocSize = pMpeg4Dec->DMAIDecInputBuffer[pMpeg4Dec->indexInputBuffer].bufferSize;
        if (((pMpeg4Dec->hDMAIMpeg4Handle.bThumbnailMode == OMX_TRUE) || (pNAMComponent->bSaveFlagEOS == OMX_TRUE)) &&
            (pMpeg4Dec->bFirstFrame == OMX_TRUE) &&
            (outputDataValid == OMX_FALSE)) {
            ret = OMX_ErrorInputDataDecodeYet;
        }
        pMpeg4Dec->bFirstFrame = OMX_FALSE;
    }

    /** Fill Output Buffer **/
    if (outputDataValid == OMX_TRUE) {
        NAM_OMX_BASEPORT *pNAMInputPort = &pNAMComponent->pNAMPort[INPUT_PORT_INDEX];
        NAM_OMX_BASEPORT *pNAMOutputPort = &pNAMComponent->pNAMPort[OUTPUT_PORT_INDEX];
        void *pOutputBuf[3];

        int frameSize = bufWidth * bufHeight;
        int imageSize = outputInfo.img_width * outputInfo.img_height;

        int actualWidth  = outputInfo.img_width;
        int actualHeight = outputInfo.img_height;
        int actualImageSize = imageSize;

        pOutputBuf[0] = (void *)pOutputData->dataBuffer;
        pOutputBuf[1] = (void *)pOutputData->dataBuffer + actualImageSize;
        pOutputBuf[2] = (void *)pOutputData->dataBuffer + ((actualImageSize * 5) / 4);

#ifdef USE_ANDROID_EXTENSION
        if (pNAMOutputPort->bUseAndroidNativeBuffer == OMX_TRUE) {
            OMX_U32 retANB = 0;
            void *pVirAddrs[2];
            actualWidth  = (outputInfo.img_width + 15) & (~15);
            actualImageSize = actualWidth * actualHeight;

             retANB = getVADDRfromANB(pOutputData->dataBuffer,
                            (OMX_U32)pNAMInputPort->portDefinition.format.video.nFrameWidth,
                            (OMX_U32)pNAMInputPort->portDefinition.format.video.nFrameHeight,
                            pVirAddrs);
            if (retANB != 0) {
                NAM_OSAL_Log(NAM_LOG_ERROR, "Error getVADDRfromANB, Error code:%d", retANB);
                ret = OMX_ErrorOverflow;
                goto EXIT;
            }
            pOutputBuf[0] = pVirAddrs[0];
            pOutputBuf[1] = pVirAddrs[1];
        }
#endif
        if ((pMpeg4Dec->hDMAIMpeg4Handle.bThumbnailMode == OMX_FALSE) &&
            (pNAMOutputPort->portDefinition.format.video.eColorFormat == OMX_SEC_COLOR_FormatNV12TPhysicalAddress))
        {
            /* if use Post copy address structure */
            NAM_OSAL_Memcpy(pOutputBuf[0], &frameSize, sizeof(frameSize));
            NAM_OSAL_Memcpy(pOutputBuf[0] + sizeof(frameSize), &(outputInfo.YPhyAddr), sizeof(outputInfo.YPhyAddr));
            NAM_OSAL_Memcpy(pOutputBuf[0] + sizeof(frameSize) + (sizeof(void *) * 1), &(outputInfo.CPhyAddr), sizeof(outputInfo.CPhyAddr));
            NAM_OSAL_Memcpy(pOutputBuf[0] + sizeof(frameSize) + (sizeof(void *) * 2), &(outputInfo.YVirAddr), sizeof(outputInfo.YVirAddr));
            NAM_OSAL_Memcpy(pOutputBuf[0] + sizeof(frameSize) + (sizeof(void *) * 3), &(outputInfo.CVirAddr), sizeof(outputInfo.CVirAddr));
            pOutputData->dataLen = (bufWidth * bufHeight * 3) / 2;
        } else {
            switch (pNAMComponent->pNAMPort[OUTPUT_PORT_INDEX].portDefinition.format.video.eColorFormat) {
            case OMX_COLOR_FormatYUV420Planar:
            {
                NAM_OSAL_Log(NAM_LOG_TRACE, "YUV420P out");
                csc_tiled_to_linear(
                    (unsigned char *)pOutputBuf[0],
                    (unsigned char *)outputInfo.YVirAddr,
                    actualWidth,
                    actualHeight);
                csc_tiled_to_linear_deinterleave(
                    (unsigned char *)pOutputBuf[1],
                    (unsigned char *)pOutputBuf[2],
                    (unsigned char *)outputInfo.CVirAddr,
                    actualWidth,
                    actualHeight >> 1);
                pOutputData->dataLen = actualImageSize * 3 / 2;
            }
                break;
            case OMX_COLOR_FormatYUV420SemiPlanar:
            case OMX_SEC_COLOR_FormatANBYUV420SemiPlanar:
            default:
            {
                NAM_OSAL_Log(NAM_LOG_TRACE, "YUV420SP out");
                csc_tiled_to_linear(
                    (unsigned char *)pOutputBuf[0],
                    (unsigned char *)outputInfo.YVirAddr,
                    actualWidth,
                    actualHeight);
                csc_tiled_to_linear(
                    (unsigned char *)pOutputBuf[1],
                    (unsigned char *)outputInfo.CVirAddr,
                    actualWidth,
                    actualHeight >> 1);
                pOutputData->dataLen = actualImageSize * 3 / 2;
            }
                break;
            }
        }
#ifdef USE_ANDROID_EXTENSION
        if (pNAMOutputPort->bUseAndroidNativeBuffer == OMX_TRUE)
            putVADDRtoANB(pOutputData->dataBuffer);
#endif
    } else {
        pOutputData->dataLen = 0;
    }

EXIT:
    FunctionOut();

    return ret;
}

/* DMAI Decode */
OMX_ERRORTYPE NAM_DMAI_Mpeg4Dec_bufferProcess(OMX_COMPONENTTYPE *pOMXComponent, NAM_OMX_DATA *pInputData, NAM_OMX_DATA *pOutputData)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    NAM_OMX_BASECOMPONENT   *pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    NAM_MPEG4_HANDLE        *pMpeg4Dec = (NAM_MPEG4_HANDLE *)pNAMComponent->hCodecHandle;
    NAM_OMX_BASEPORT        *pInputPort = &pNAMComponent->pNAMPort[INPUT_PORT_INDEX];
    NAM_OMX_BASEPORT        *pOutputPort = &pNAMComponent->pNAMPort[OUTPUT_PORT_INDEX];
    OMX_BOOL                 bCheckPrefix = OMX_FALSE;

    FunctionIn();

    if ((!CHECK_PORT_ENABLED(pInputPort)) || (!CHECK_PORT_ENABLED(pOutputPort)) ||
            (!CHECK_PORT_POPULATED(pInputPort)) || (!CHECK_PORT_POPULATED(pOutputPort))) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }
    if (OMX_FALSE == NAM_Check_BufferProcess_State(pNAMComponent)) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    ret = NAM_DMAI_Mpeg4_Decode(pOMXComponent, pInputData, pOutputData);
    if (ret != OMX_ErrorNone) {
        if (ret == OMX_ErrorInputDataDecodeYet) {
            pOutputData->usedDataLen = 0;
            pOutputData->remainDataLen = pOutputData->dataLen;
        } else {
            pNAMComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                                    pNAMComponent->callbackData,
                                                    OMX_EventError, ret, 0, NULL);
        }
    } else {
        pInputData->previousDataLen = pInputData->dataLen;
        pInputData->usedDataLen += pInputData->dataLen;
        pInputData->remainDataLen = pInputData->dataLen - pInputData->usedDataLen;
        pInputData->dataLen -= pInputData->usedDataLen;
        pInputData->usedDataLen = 0;

        pOutputData->usedDataLen = 0;
        pOutputData->remainDataLen = pOutputData->dataLen;
    }

EXIT:
    FunctionOut();

    return ret;
}

OSCL_EXPORT_REF OMX_ERRORTYPE NAM_OMX_ComponentInit(OMX_HANDLETYPE hComponent, OMX_STRING componentName)
{
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE       *pOMXComponent = NULL;
    NAM_OMX_BASECOMPONENT   *pNAMComponent = NULL;
    NAM_OMX_BASEPORT        *pNAMPort = NULL;
    NAM_MPEG4_HANDLE        *pMpeg4Dec = NULL;
    OMX_S32                  codecType = -1;
    int i = 0;

    FunctionIn();

    if ((hComponent == NULL) || (componentName == NULL)) {
        ret = OMX_ErrorBadParameter;
        NAM_OSAL_Log(NAM_LOG_ERROR, "%s: parameters are null, ret: %X", __FUNCTION__, ret);
        goto EXIT;
    }
    if (NAM_OSAL_Strcmp(NAM_OMX_COMPONENT_MPEG4_DEC, componentName) == 0) {
        codecType = CODEC_TYPE_MPEG4;
    } else if (NAM_OSAL_Strcmp(NAM_OMX_COMPONENT_H263_DEC, componentName) == 0) {
        codecType = CODEC_TYPE_H263;
    } else {
        ret = OMX_ErrorBadParameter;
        NAM_OSAL_Log(NAM_LOG_ERROR, "%s: componentName(%s) error, ret: %X", __FUNCTION__, ret);
        goto EXIT;
    }

    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = NAM_OMX_VideoDecodeComponentInit(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        NAM_OSAL_Log(NAM_LOG_ERROR, "%s: NAM_OMX_VideoDecodeComponentInit error, ret: %X", __FUNCTION__, ret);
        goto EXIT;
    }
    pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pNAMComponent->codecType = HW_VIDEO_CODEC;

    pNAMComponent->componentName = (OMX_STRING)NAM_OSAL_Malloc(MAX_OMX_COMPONENT_NAME_SIZE);
    if (pNAMComponent->componentName == NULL) {
        NAM_OMX_VideoDecodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        NAM_OSAL_Log(NAM_LOG_ERROR, "%s: componentName alloc error, ret: %X", __FUNCTION__, ret);
        goto EXIT;
    }
    NAM_OSAL_Memset(pNAMComponent->componentName, 0, MAX_OMX_COMPONENT_NAME_SIZE);

    pMpeg4Dec = NAM_OSAL_Malloc(sizeof(NAM_MPEG4_HANDLE));
    if (pMpeg4Dec == NULL) {
        NAM_OMX_VideoDecodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        NAM_OSAL_Log(NAM_LOG_ERROR, "%s: NAM_MPEG4_HANDLE alloc error, ret: %X", __FUNCTION__, ret);
        goto EXIT;
    }
    NAM_OSAL_Memset(pMpeg4Dec, 0, sizeof(NAM_MPEG4_HANDLE));
    pNAMComponent->hCodecHandle = (OMX_HANDLETYPE)pMpeg4Dec;
    pMpeg4Dec->hDMAIMpeg4Handle.codecType = codecType;

    if (codecType == CODEC_TYPE_MPEG4)
        NAM_OSAL_Strcpy(pNAMComponent->componentName, NAM_OMX_COMPONENT_MPEG4_DEC);
    else
        NAM_OSAL_Strcpy(pNAMComponent->componentName, NAM_OMX_COMPONENT_H263_DEC);

    /* Set componentVersion */
    pNAMComponent->componentVersion.s.nVersionMajor = VERSIONMAJOR_NUMBER;
    pNAMComponent->componentVersion.s.nVersionMinor = VERSIONMINOR_NUMBER;
    pNAMComponent->componentVersion.s.nRevision     = REVISION_NUMBER;
    pNAMComponent->componentVersion.s.nStep         = STEP_NUMBER;
    /* Set specVersion */
    pNAMComponent->specVersion.s.nVersionMajor = VERSIONMAJOR_NUMBER;
    pNAMComponent->specVersion.s.nVersionMinor = VERSIONMINOR_NUMBER;
    pNAMComponent->specVersion.s.nRevision     = REVISION_NUMBER;
    pNAMComponent->specVersion.s.nStep         = STEP_NUMBER;

    /* Android CapabilityFlags */
    pNAMComponent->capabilityFlags.iIsOMXComponentMultiThreaded                   = OMX_TRUE;
    pNAMComponent->capabilityFlags.iOMXComponentSupportsExternalInputBufferAlloc  = OMX_TRUE;
    pNAMComponent->capabilityFlags.iOMXComponentSupportsExternalOutputBufferAlloc = OMX_TRUE;
    pNAMComponent->capabilityFlags.iOMXComponentSupportsMovableInputBuffers       = OMX_FALSE;
    pNAMComponent->capabilityFlags.iOMXComponentSupportsPartialFrames             = OMX_FALSE;
    pNAMComponent->capabilityFlags.iOMXComponentUsesNALStartCodes                 = OMX_TRUE;
    pNAMComponent->capabilityFlags.iOMXComponentCanHandleIncompleteFrames         = OMX_TRUE;
    pNAMComponent->capabilityFlags.iOMXComponentUsesFullAVCFrames                 = OMX_TRUE;

    /* Input port */
    pNAMPort = &pNAMComponent->pNAMPort[INPUT_PORT_INDEX];
    pNAMPort->portDefinition.format.video.nFrameWidth = DEFAULT_FRAME_WIDTH;
    pNAMPort->portDefinition.format.video.nFrameHeight= DEFAULT_FRAME_HEIGHT;
    pNAMPort->portDefinition.format.video.nStride = 0;
    pNAMPort->portDefinition.format.video.nSliceHeight = 0;
    pNAMPort->portDefinition.nBufferSize = DEFAULT_VIDEO_INPUT_BUFFER_SIZE;
    if (codecType == CODEC_TYPE_MPEG4) {
        pNAMPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG4;
        NAM_OSAL_Memset(pNAMPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
        NAM_OSAL_Strcpy(pNAMPort->portDefinition.format.video.cMIMEType, "video/mpeg4");
    } else {
        pNAMPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingH263;
        NAM_OSAL_Memset(pNAMPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
        NAM_OSAL_Strcpy(pNAMPort->portDefinition.format.video.cMIMEType, "video/h263");
    }
    pNAMPort->portDefinition.format.video.pNativeRender = 0;
    pNAMPort->portDefinition.format.video.bFlagErrorConcealment = OMX_FALSE;
    pNAMPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    pNAMPort->portDefinition.bEnabled = OMX_TRUE;

    /* Output port */
    pNAMPort = &pNAMComponent->pNAMPort[OUTPUT_PORT_INDEX];
    pNAMPort->portDefinition.format.video.nFrameWidth = DEFAULT_FRAME_WIDTH;
    pNAMPort->portDefinition.format.video.nFrameHeight= DEFAULT_FRAME_HEIGHT;
    pNAMPort->portDefinition.format.video.nStride = 0;
    pNAMPort->portDefinition.format.video.nSliceHeight = 0;
    pNAMPort->portDefinition.nBufferSize = DEFAULT_VIDEO_OUTPUT_BUFFER_SIZE;
    pNAMPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    NAM_OSAL_Memset(pNAMPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
    NAM_OSAL_Strcpy(pNAMPort->portDefinition.format.video.cMIMEType, "raw/video");
    pNAMPort->portDefinition.format.video.pNativeRender = 0;
    pNAMPort->portDefinition.format.video.bFlagErrorConcealment = OMX_FALSE;
    pNAMPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
    pNAMPort->portDefinition.bEnabled = OMX_TRUE;

    if (codecType == CODEC_TYPE_MPEG4) {
        for(i = 0; i < ALL_PORT_NUM; i++) {
            INIT_SET_SIZE_VERSION(&pMpeg4Dec->mpeg4Component[i], OMX_VIDEO_PARAM_MPEG4TYPE);
            pMpeg4Dec->mpeg4Component[i].nPortIndex = i;
            pMpeg4Dec->mpeg4Component[i].eProfile   = OMX_VIDEO_MPEG4ProfileSimple;
            pMpeg4Dec->mpeg4Component[i].eLevel     = OMX_VIDEO_MPEG4Level3;
        }
    } else {
        for(i = 0; i < ALL_PORT_NUM; i++) {
            INIT_SET_SIZE_VERSION(&pMpeg4Dec->h263Component[i], OMX_VIDEO_PARAM_H263TYPE);
            pMpeg4Dec->h263Component[i].nPortIndex = i;
            pMpeg4Dec->h263Component[i].eProfile   = OMX_VIDEO_H263ProfileBaseline | OMX_VIDEO_H263ProfileISWV2;
            pMpeg4Dec->h263Component[i].eLevel     = OMX_VIDEO_H263Level45;
        }
    }

    pOMXComponent->GetParameter      = &NAM_DMAI_Mpeg4Dec_GetParameter;
    pOMXComponent->SetParameter      = &NAM_DMAI_Mpeg4Dec_SetParameter;
    pOMXComponent->GetConfig         = &NAM_DMAI_Mpeg4Dec_GetConfig;
    pOMXComponent->SetConfig         = &NAM_DMAI_Mpeg4Dec_SetConfig;
    pOMXComponent->GetExtensionIndex = &NAM_DMAI_Mpeg4Dec_GetExtensionIndex;
    pOMXComponent->ComponentRoleEnum = &NAM_DMAI_Mpeg4Dec_ComponentRoleEnum;
    pOMXComponent->ComponentDeInit   = &NAM_OMX_ComponentDeinit;

    pNAMComponent->nam_dmai_componentInit      = &NAM_DMAI_Mpeg4Dec_Init;
    pNAMComponent->nam_dmai_componentTerminate = &NAM_DMAI_Mpeg4Dec_Terminate;
    pNAMComponent->nam_dmai_bufferProcess      = &NAM_DMAI_Mpeg4Dec_bufferProcess;
    if (codecType == CODEC_TYPE_MPEG4)
        pNAMComponent->nam_checkInputFrame = &Check_Mpeg4_Frame;
    else
        pNAMComponent->nam_checkInputFrame = &Check_H263_Frame;

    pNAMComponent->currentState = OMX_StateLoaded;

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_OMX_ComponentDeinit(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE       *pOMXComponent = NULL;
    NAM_OMX_BASECOMPONENT   *pNAMComponent = NULL;
    NAM_MPEG4_HANDLE        *pMpeg4Dec = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    NAM_OSAL_Free(pNAMComponent->componentName);
    pNAMComponent->componentName = NULL;

    pMpeg4Dec = (NAM_MPEG4_HANDLE *)pNAMComponent->hCodecHandle;
    if (pMpeg4Dec != NULL) {
        NAM_OSAL_Free(pMpeg4Dec);
        pNAMComponent->hCodecHandle = NULL;
    }

    ret = NAM_OMX_VideoDecodeComponentDeinit(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

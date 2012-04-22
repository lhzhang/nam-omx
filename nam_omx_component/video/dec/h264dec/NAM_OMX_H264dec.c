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
 * @file        NAM_OMX_H264dec.c
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version     1.0
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
#include "NAM_OMX_H264dec.h"
#include "color_space_convertor.h"

#undef  NAM_LOG_TAG
#define NAM_LOG_TAG    "NAM_H264_DEC"
//#define NAM_LOG_OFF
#define NAM_TRACE_ON
#include "NAM_OSAL_Log.h"

#define ENABLE_MFC 0

//#define ADD_SPS_PPS_I_FRAME
//#define FULL_FRAME_SEARCH

/* H.264 Decoder Supported Levels & profiles */
NAM_OMX_VIDEO_PROFILELEVEL supportedAVCProfileLevels[] ={
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel1},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel1b},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel11},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel12},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel13},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel2},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel21},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel22},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel3},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel31},

    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel1},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel1b},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel11},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel12},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel13},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel2},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel21},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel22},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel3},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel31},

    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel1},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel1b},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel11},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel12},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel13},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel2},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel21},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel22},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel3},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel31}};


static int Check_H264_Frame(OMX_U8 *pInputStream, int buffSize, OMX_U32 flag, OMX_BOOL bPreviousFrameEOF, OMX_BOOL *pbEndOfFrame)
{
    OMX_U32  preFourByte       = (OMX_U32)-1;
    int      accessUnitSize    = 0;
    int      frameTypeBoundary = 0;
    int      nextNaluSize      = 0;
    int      naluStart         = 0;

    if (bPreviousFrameEOF == OMX_TRUE)
        naluStart = 0;
    else
        naluStart = 1;

    while (1) {
        int inputOneByte = 0;

        if (accessUnitSize == buffSize)
            goto EXIT;

        inputOneByte = *(pInputStream++);
        accessUnitSize += 1;

        if (preFourByte == 0x00000001 || (preFourByte << 8) == 0x00000100) {
            int naluType = inputOneByte & 0x1F;

            NAM_OSAL_Log(NAM_LOG_TRACE, "NaluType : %d", naluType);
            if (naluStart == 0) {
#ifdef ADD_SPS_PPS_I_FRAME
                if (naluType == 1 || naluType == 5)
#else
                if (naluType == 1 || naluType == 5 || naluType == 7 || naluType == 8)
#endif
                    naluStart = 1;
            } else {
#ifdef OLD_DETECT
                frameTypeBoundary = (8 - naluType) & (naluType - 10); //AUD(9)
#else
                if (naluType == 9)
                    frameTypeBoundary = -2;
#endif
                if (naluType == 1 || naluType == 5) {
                    if (accessUnitSize == buffSize) {
                        accessUnitSize--;
                        goto EXIT;
                    }
                    inputOneByte = *pInputStream++;
                    accessUnitSize += 1;

                    if (inputOneByte >= 0x80)
                        frameTypeBoundary = -1;
                }
                if (frameTypeBoundary < 0) {
                    break;
                }
            }

        }
        preFourByte = (preFourByte << 8) + inputOneByte;
    }

    *pbEndOfFrame = OMX_TRUE;
    nextNaluSize = -5;
    if (frameTypeBoundary == -1)
        nextNaluSize = -6;
    if (preFourByte != 0x00000001)
        nextNaluSize++;
    return (accessUnitSize + nextNaluSize);

EXIT:
    *pbEndOfFrame = OMX_FALSE;

    return accessUnitSize;
}

OMX_BOOL Check_H264_StartCode(OMX_U8 *pInputStream, OMX_U32 streamSize)
{
    if (streamSize < 4) {
        return OMX_FALSE;
    } else if ((pInputStream[0] == 0x00) &&
              (pInputStream[1] == 0x00) &&
              (pInputStream[2] == 0x00) &&
              (pInputStream[3] != 0x00) &&
              ((pInputStream[3] >> 3) == 0x00)) {
        return OMX_TRUE;
    } else if ((pInputStream[0] == 0x00) &&
              (pInputStream[1] == 0x00) &&
              (pInputStream[2] != 0x00) &&
              ((pInputStream[2] >> 3) == 0x00)) {
        return OMX_TRUE;
    } else {
        return OMX_FALSE;
    }
}

OMX_ERRORTYPE NAM_DMAI_H264Dec_GetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nParamIndex,
    OMX_INOUT OMX_PTR     pComponentParameterStructure)
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
    case OMX_IndexParamVideoAvc:
    {
        OMX_VIDEO_PARAM_AVCTYPE *pDstAVCComponent = (OMX_VIDEO_PARAM_AVCTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_AVCTYPE *pSrcAVCComponent = NULL;
        NAM_H264DEC_HANDLE      *pH264Dec = NULL;

        ret = NAM_OMX_Check_SizeVersion(pDstAVCComponent, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstAVCComponent->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Dec = (NAM_H264DEC_HANDLE *)pNAMComponent->hCodecHandle;
        pSrcAVCComponent = &pH264Dec->AVCComponent[pDstAVCComponent->nPortIndex];

        NAM_OSAL_Memcpy(pDstAVCComponent, pSrcAVCComponent, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
    }
        break;
    case OMX_IndexParamStandardComponentRole:
    {
        OMX_PARAM_COMPONENTROLETYPE *pComponentRole = (OMX_PARAM_COMPONENTROLETYPE *)pComponentParameterStructure;
        ret = NAM_OMX_Check_SizeVersion(pComponentRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        NAM_OSAL_Strcpy((char *)pComponentRole->cRole, NAM_OMX_COMPONENT_H264_DEC_ROLE);
    }
        break;
    case OMX_IndexParamVideoProfileLevelQuerySupported:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pDstProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE*)pComponentParameterStructure;
        NAM_OMX_VIDEO_PROFILELEVEL *pProfileLevel = NULL;
        OMX_U32 maxProfileLevelNum = 0;

        ret = NAM_OMX_Check_SizeVersion(pDstProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pProfileLevel = supportedAVCProfileLevels;
        maxProfileLevelNum = sizeof(supportedAVCProfileLevels) / sizeof(NAM_OMX_VIDEO_PROFILELEVEL);

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
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pDstProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE*)pComponentParameterStructure;
        OMX_VIDEO_PARAM_AVCTYPE *pSrcAVCComponent = NULL;
        NAM_H264DEC_HANDLE      *pH264Dec = NULL;

        ret = NAM_OMX_Check_SizeVersion(pDstProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Dec = (NAM_H264DEC_HANDLE *)pNAMComponent->hCodecHandle;
        pSrcAVCComponent = &pH264Dec->AVCComponent[pDstProfileLevel->nPortIndex];

        pDstProfileLevel->eProfile = pSrcAVCComponent->eProfile;
        pDstProfileLevel->eLevel = pSrcAVCComponent->eLevel;
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = NULL;
        NAM_H264DEC_HANDLE      *pH264Dec = NULL;

        ret = NAM_OMX_Check_SizeVersion(pDstErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstErrorCorrectionType->nPortIndex != INPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Dec = (NAM_H264DEC_HANDLE *)pNAMComponent->hCodecHandle;
        pSrcErrorCorrectionType = &pH264Dec->errorCorrectionType[INPUT_PORT_INDEX];

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

OMX_ERRORTYPE NAM_DMAI_H264Dec_SetParameter(
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
    case OMX_IndexParamVideoAvc:
    {
        OMX_VIDEO_PARAM_AVCTYPE *pDstAVCComponent = NULL;
        OMX_VIDEO_PARAM_AVCTYPE *pSrcAVCComponent = (OMX_VIDEO_PARAM_AVCTYPE *)pComponentParameterStructure;
        NAM_H264DEC_HANDLE      *pH264Dec = NULL;

        ret = NAM_OMX_Check_SizeVersion(pSrcAVCComponent, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcAVCComponent->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Dec = (NAM_H264DEC_HANDLE *)pNAMComponent->hCodecHandle;
        pDstAVCComponent = &pH264Dec->AVCComponent[pSrcAVCComponent->nPortIndex];

        NAM_OSAL_Memcpy(pDstAVCComponent, pSrcAVCComponent, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
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

        if (!NAM_OSAL_Strcmp((char*)pComponentRole->cRole, NAM_OMX_COMPONENT_H264_DEC_ROLE)) {
            pNAMComponent->pNAMPort[INPUT_PORT_INDEX].portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
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
        if(pPortDefinition->nBufferCountActual < pNAMPort->portDefinition.nBufferCountMin) {
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
            case OMX_NAM_COLOR_FormatNV12TPhysicalAddress:
            case OMX_NAM_COLOR_FormatANBYUV420SemiPlanar:
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
        OMX_VIDEO_PARAM_AVCTYPE *pDstAVCComponent = NULL;
        NAM_H264DEC_HANDLE      *pH264Dec = NULL;

        ret = NAM_OMX_Check_SizeVersion(pSrcProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pSrcProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Dec = (NAM_H264DEC_HANDLE *)pNAMComponent->hCodecHandle;

        pDstAVCComponent = &pH264Dec->AVCComponent[pSrcProfileLevel->nPortIndex];
        pDstAVCComponent->eProfile = pSrcProfileLevel->eProfile;
        pDstAVCComponent->eLevel = pSrcProfileLevel->eLevel;
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = NULL;
        NAM_H264DEC_HANDLE      *pH264Dec = NULL;

        ret = NAM_OMX_Check_SizeVersion(pSrcErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcErrorCorrectionType->nPortIndex != INPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Dec = (NAM_H264DEC_HANDLE *)pNAMComponent->hCodecHandle;
        pDstErrorCorrectionType = &pH264Dec->errorCorrectionType[INPUT_PORT_INDEX];

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

OMX_ERRORTYPE NAM_DMAI_H264Dec_GetConfig(
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
    case OMX_IndexConfigCommonOutputCrop:
    {
        NAM_H264DEC_HANDLE  *pH264Dec = NULL;
        OMX_CONFIG_RECTTYPE *pSrcRectType = NULL;
        OMX_CONFIG_RECTTYPE *pDstRectType = NULL;
        pH264Dec = (NAM_H264DEC_HANDLE *)pNAMComponent->hCodecHandle;

        if (pH264Dec->hDMAIH264Handle.bConfiguredDMAI == OMX_FALSE) {
            ret = OMX_ErrorNotReady;
            break;
        }

        pDstRectType = (OMX_CONFIG_RECTTYPE *)pComponentConfigStructure;

        if ((pDstRectType->nPortIndex != INPUT_PORT_INDEX) &&
            (pDstRectType->nPortIndex != OUTPUT_PORT_INDEX)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        NAM_OMX_BASEPORT *pNAMPort = &pNAMComponent->pNAMPort[pDstRectType->nPortIndex];

        pSrcRectType = &(pNAMPort->cropRectangle);

        pDstRectType->nTop = pSrcRectType->nTop;
        pDstRectType->nLeft = pSrcRectType->nLeft;
        pDstRectType->nHeight = pSrcRectType->nHeight;
        pDstRectType->nWidth = pSrcRectType->nWidth;
    }
        break;
    default:
        ret = NAM_OMX_GetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_DMAI_H264Dec_SetConfig(
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
    case OMX_IndexVendorThumbnailMode:
    {
        NAM_H264DEC_HANDLE   *pH264Dec = (NAM_H264DEC_HANDLE *)pNAMComponent->hCodecHandle;

        pH264Dec->hDMAIH264Handle.bThumbnailMode = *((OMX_BOOL *)pComponentConfigStructure);

        ret = OMX_ErrorNone;
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

OMX_ERRORTYPE NAM_DMAI_H264Dec_GetExtensionIndex(
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
        NAM_H264DEC_HANDLE *pH264Dec = (NAM_H264DEC_HANDLE *)pNAMComponent->hCodecHandle;
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

OMX_ERRORTYPE NAM_DMAI_H264Dec_ComponentRoleEnum(OMX_HANDLETYPE hComponent, OMX_U8 *cRole, OMX_U32 nIndex)
{
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE       *pOMXComponent = NULL;
    NAM_OMX_BASECOMPONENT   *pNAMComponent = NULL;

    FunctionIn();

    if ((hComponent == NULL) || (cRole == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (nIndex == (MAX_COMPONENT_ROLE_NUM-1)) {
        NAM_OSAL_Strcpy((char *)cRole, NAM_OMX_COMPONENT_H264_DEC_ROLE);
        ret = OMX_ErrorNone;
    } else {
        ret = OMX_ErrorNoMore;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_DMAI_DecodeThread(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    NAM_OMX_BASECOMPONENT *pNAMComponent = NULL;
    NAM_H264DEC_HANDLE    *pH264Dec = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pH264Dec = (NAM_H264DEC_HANDLE *)pNAMComponent->hCodecHandle;

    while (pH264Dec->NBDecThread.bExitDecodeThread == OMX_FALSE) {
        NAM_OSAL_SemaphoreWait(pH264Dec->NBDecThread.hDecFrameStart);

        if (pH264Dec->NBDecThread.bExitDecodeThread == OMX_FALSE) {
#if ENABLE_MFC
            pH264Dec->hDMAIH264Handle.returnCodec = SsbSipDmaiDecExe(pH264Dec->hDMAIH264Handle.hDMAIHandle, pH264Dec->NBDecThread.oneFrameSize);
#endif
            NAM_OSAL_SemaphorePost(pH264Dec->NBDecThread.hDecFrameEnd);
        }
    }

EXIT:
    NAM_OSAL_ThreadExit(NULL);
    FunctionOut();

    return ret;
}

/* DMAI Init */
OMX_ERRORTYPE NAM_DMAI_H264Dec_Init(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    NAM_OMX_BASECOMPONENT *pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    NAM_H264DEC_HANDLE    *pH264Dec = NULL;

    OMX_PTR hDMAIHandle       = NULL;
    OMX_PTR pStreamBuffer    = NULL;
    OMX_PTR pStreamPhyBuffer = NULL;

    pH264Dec = (NAM_H264DEC_HANDLE *)pNAMComponent->hCodecHandle;
    pH264Dec->hDMAIH264Handle.bConfiguredDMAI = OMX_FALSE;
    pNAMComponent->bUseFlagEOF = OMX_FALSE;
    pNAMComponent->bSaveFlagEOS = OMX_FALSE;

#if ENABLE_MFC
    /* DMAI(Multi Function Codec) decoder and CMM(Codec Memory Management) driver open */
    SSBIP_DMAI_BUFFER_TYPE buf_type = CACHE;
    hDMAIHandle = (OMX_PTR)SsbSipDmaiDecOpen(&buf_type);
#endif
    if (hDMAIHandle == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    pH264Dec->hDMAIH264Handle.hDMAIHandle = hDMAIHandle;

    /* Allocate decoder's input buffer */
    pStreamBuffer = SsbSipDmaiDecGetInBuf(hDMAIHandle, &pStreamPhyBuffer, DEFAULT_DMAI_INPUT_BUFFER_SIZE * DMAI_INPUT_BUFFER_NUM_MAX);
    if (pStreamBuffer == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    pH264Dec->DMAIDecInputBuffer[0].VirAddr = pStreamBuffer;
    pH264Dec->DMAIDecInputBuffer[0].PhyAddr = pStreamPhyBuffer;
    pH264Dec->DMAIDecInputBuffer[0].bufferSize = DEFAULT_DMAI_INPUT_BUFFER_SIZE;
    pH264Dec->DMAIDecInputBuffer[0].dataSize = 0;
    pH264Dec->DMAIDecInputBuffer[1].VirAddr = (unsigned char *)pStreamBuffer + pH264Dec->DMAIDecInputBuffer[0].bufferSize;
    pH264Dec->DMAIDecInputBuffer[1].PhyAddr = (unsigned char *)pStreamPhyBuffer + pH264Dec->DMAIDecInputBuffer[0].bufferSize;
    pH264Dec->DMAIDecInputBuffer[1].bufferSize = DEFAULT_DMAI_INPUT_BUFFER_SIZE;
    pH264Dec->DMAIDecInputBuffer[1].dataSize = 0;
    pH264Dec->indexInputBuffer = 0;

    pH264Dec->bFirstFrame = OMX_TRUE;

    pH264Dec->NBDecThread.bExitDecodeThread = OMX_FALSE;
    pH264Dec->NBDecThread.bDecoderRun = OMX_FALSE;
    pH264Dec->NBDecThread.oneFrameSize = 0;
    NAM_OSAL_SemaphoreCreate(&(pH264Dec->NBDecThread.hDecFrameStart));
    NAM_OSAL_SemaphoreCreate(&(pH264Dec->NBDecThread.hDecFrameEnd));
    if (OMX_ErrorNone == NAM_OSAL_ThreadCreate(&pH264Dec->NBDecThread.hNBDecodeThread,
                                                NAM_DMAI_DecodeThread,
                                                pOMXComponent)) {
#if ENABLE_MFC
        pH264Dec->hDMAIH264Handle.returnCodec = DMAI_RET_OK;
#endif
    }

    pH264Dec->hDMAIH264Handle.pDMAIStreamBuffer    = pH264Dec->DMAIDecInputBuffer[0].VirAddr;
    pH264Dec->hDMAIH264Handle.pDMAIStreamPhyBuffer = pH264Dec->DMAIDecInputBuffer[0].PhyAddr;
    pNAMComponent->processData[INPUT_PORT_INDEX].dataBuffer = pH264Dec->DMAIDecInputBuffer[0].VirAddr;
    pNAMComponent->processData[INPUT_PORT_INDEX].allocSize  = pH264Dec->DMAIDecInputBuffer[0].bufferSize;

    NAM_OSAL_Memset(pNAMComponent->timeStamp, -19771003, sizeof(OMX_TICKS) * MAX_TIMESTAMP);
    NAM_OSAL_Memset(pNAMComponent->nFlags, 0, sizeof(OMX_U32) * MAX_FLAGS);
    pH264Dec->hDMAIH264Handle.indexTimestamp = 0;
    pNAMComponent->getAllDelayBuffer = OMX_FALSE;

EXIT:
    return ret;
}

/* DMAI Terminate */
OMX_ERRORTYPE NAM_DMAI_H264Dec_Terminate(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    NAM_OMX_BASECOMPONENT *pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    NAM_H264DEC_HANDLE    *pH264Dec = NULL;
    OMX_PTR                hDMAIHandle = NULL;

    FunctionIn();

    pH264Dec = (NAM_H264DEC_HANDLE *)pNAMComponent->hCodecHandle;
    hDMAIHandle = pH264Dec->hDMAIH264Handle.hDMAIHandle;

    pH264Dec->hDMAIH264Handle.pDMAIStreamBuffer    = NULL;
    pH264Dec->hDMAIH264Handle.pDMAIStreamPhyBuffer = NULL;
    pNAMComponent->processData[INPUT_PORT_INDEX].dataBuffer = NULL;
    pNAMComponent->processData[INPUT_PORT_INDEX].allocSize = 0;

    if (pH264Dec->NBDecThread.hNBDecodeThread != NULL) {
        pH264Dec->NBDecThread.bExitDecodeThread = OMX_TRUE;
        NAM_OSAL_SemaphorePost(pH264Dec->NBDecThread.hDecFrameStart);
        NAM_OSAL_ThreadTerminate(pH264Dec->NBDecThread.hNBDecodeThread);
        pH264Dec->NBDecThread.hNBDecodeThread = NULL;
    }

    if(pH264Dec->NBDecThread.hDecFrameEnd != NULL) {
        NAM_OSAL_SemaphoreTerminate(pH264Dec->NBDecThread.hDecFrameEnd);
        pH264Dec->NBDecThread.hDecFrameEnd = NULL;
    }

    if(pH264Dec->NBDecThread.hDecFrameStart != NULL) {
        NAM_OSAL_SemaphoreTerminate(pH264Dec->NBDecThread.hDecFrameStart);
        pH264Dec->NBDecThread.hDecFrameStart = NULL;
    }

#if ENABLE_MFC
    if (hDMAIHandle != NULL) {
        SsbSipDmaiDecClose(hDMAIHandle);
        hDMAIHandle = pH264Dec->hDMAIH264Handle.hDMAIHandle = NULL;
    }
#endif

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_DMAI_H264_Decode(OMX_COMPONENTTYPE *pOMXComponent, NAM_OMX_DATA *pInputData, NAM_OMX_DATA *pOutputData)
{
    OMX_ERRORTYPE              ret = OMX_ErrorNone;
    NAM_OMX_BASECOMPONENT     *pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    NAM_H264DEC_HANDLE        *pH264Dec = (NAM_H264DEC_HANDLE *)pNAMComponent->hCodecHandle;
    OMX_U32                    oneFrameSize = pInputData->dataLen;
#if ENABLE_MFC
    SSBSIP_DMAI_DEC_OUTPUT_INFO outputInfo;
#endif
    OMX_S32                    setConfVal = 0;
    int                        bufWidth = 0;
    int                        bufHeight = 0;
    OMX_BOOL                   outputDataValid = OMX_FALSE;

    FunctionIn();

#if ENABLE_MFC
    if (pH264Dec->hDMAIH264Handle.bConfiguredDMAI == OMX_FALSE) {
        SSBSIP_DMAI_CODEC_TYPE eCodecType = H264_DEC;
        if ((oneFrameSize <= 0) && (pInputData->nFlags & OMX_BUFFERFLAG_EOS)) {
            pOutputData->timeStamp = pInputData->timeStamp;
            pOutputData->nFlags = pInputData->nFlags;
            ret = OMX_ErrorNone;
            goto EXIT;
        }

        setConfVal = 0;
        SsbSipDmaiDecSetConfig(pH264Dec->hDMAIH264Handle.hDMAIHandle, DMAI_DEC_SETCONF_EXTRA_BUFFER_NUM, &setConfVal);

        /* Default number in the driver is optimized */
        if (pH264Dec->hDMAIH264Handle.bThumbnailMode == OMX_TRUE) {
            setConfVal = 1;
            SsbSipDmaiDecSetConfig(pH264Dec->hDMAIH264Handle.hDMAIHandle, DMAI_DEC_SETCONF_DISPLAY_DELAY, &setConfVal);
        } else {
            setConfVal = 8;
            SsbSipDmaiDecSetConfig(pH264Dec->hDMAIH264Handle.hDMAIHandle, DMAI_DEC_SETCONF_DISPLAY_DELAY, &setConfVal);
        }

        pH264Dec->hDMAIH264Handle.returnCodec = SsbSipDmaiDecInit(pH264Dec->hDMAIH264Handle.hDMAIHandle, eCodecType, oneFrameSize);
        if (pH264Dec->hDMAIH264Handle.returnCodec == DMAI_RET_OK) {
            SSBSIP_DMAI_IMG_RESOLUTION imgResol;
            SSBSIP_DMAI_CROP_INFORMATION cropInfo;
            NAM_OMX_BASEPORT *namInputPort = &pNAMComponent->pNAMPort[INPUT_PORT_INDEX];
            NAM_OMX_BASEPORT *namOutputPort = &pNAMComponent->pNAMPort[OUTPUT_PORT_INDEX];

            SsbSipDmaiDecGetConfig(pH264Dec->hDMAIH264Handle.hDMAIHandle, DMAI_DEC_GETCONF_BUF_WIDTH_HEIGHT, &imgResol);
            NAM_OSAL_Log(NAM_LOG_TRACE, "set width height information : %d, %d",
                            namInputPort->portDefinition.format.video.nFrameWidth,
                            namInputPort->portDefinition.format.video.nFrameHeight);
            NAM_OSAL_Log(NAM_LOG_TRACE, "dmai width height information : %d, %d",
                            imgResol.width, imgResol.height);

            SsbSipDmaiDecGetConfig(pH264Dec->hDMAIH264Handle.hDMAIHandle, DMAI_DEC_GETCONF_CROP_INFO, &cropInfo);
            NAM_OSAL_Log(NAM_LOG_TRACE, "dmai crop_top crop_bottom crop_left crop_right :  %d, %d, %d, %d",
                            cropInfo.crop_top_offset , cropInfo.crop_bottom_offset ,
                            cropInfo.crop_left_offset , cropInfo.crop_right_offset);

            namOutputPort->cropRectangle.nTop    = cropInfo.crop_top_offset;
            namOutputPort->cropRectangle.nLeft   = cropInfo.crop_left_offset;
            namOutputPort->cropRectangle.nWidth  = imgResol.width - cropInfo.crop_left_offset - cropInfo.crop_right_offset;
            namOutputPort->cropRectangle.nHeight = imgResol.height - cropInfo.crop_top_offset - cropInfo.crop_bottom_offset;

            pH264Dec->hDMAIH264Handle.bConfiguredDMAI = OMX_TRUE;

            /** Update Frame Size **/
            if ((cropInfo.crop_left_offset != 0) || (cropInfo.crop_right_offset != 0) ||
                (cropInfo.crop_top_offset != 0) || (cropInfo.crop_bottom_offset != 0)) {
                /* change width and height information */
                namInputPort->portDefinition.format.video.nFrameWidth = imgResol.width;
                namInputPort->portDefinition.format.video.nFrameHeight = imgResol.height;
                namInputPort->portDefinition.format.video.nStride      = ((imgResol.width + 15) & (~15));
                namInputPort->portDefinition.format.video.nSliceHeight = ((imgResol.height + 15) & (~15));

                NAM_UpdateFrameSize(pOMXComponent);

                /** Send crop info call back */
                (*(pNAMComponent->pCallbacks->EventHandler))
                      (pOMXComponent,
                       pNAMComponent->callbackData,
                       OMX_EventPortSettingsChanged, /* The command was completed */
                       OMX_DirOutput, /* This is the port index */
                       OMX_IndexConfigCommonOutputCrop,
                       NULL);
            } else if((namInputPort->portDefinition.format.video.nFrameWidth != imgResol.width) ||
                      (namInputPort->portDefinition.format.video.nFrameHeight != imgResol.height)) {
                NAM_OSAL_Log(NAM_LOG_TRACE, "change width height information : OMX_EventPortSettingsChanged");
                /* change width and height information */
                namInputPort->portDefinition.format.video.nFrameWidth = imgResol.width;
                namInputPort->portDefinition.format.video.nFrameHeight = imgResol.height;
                namInputPort->portDefinition.format.video.nStride      = ((imgResol.width + 15) & (~15));
                namInputPort->portDefinition.format.video.nSliceHeight = ((imgResol.height + 15) & (~15));

                NAM_UpdateFrameSize(pOMXComponent);

                /** Send Port Settings changed call back */
                (*(pNAMComponent->pCallbacks->EventHandler))
                      (pOMXComponent,
                       pNAMComponent->callbackData,
                       OMX_EventPortSettingsChanged, /* The command was completed */
                       OMX_DirOutput, /* This is the port index */
                       0,
                       NULL);
            }

#ifdef ADD_SPS_PPS_I_FRAME
            ret = OMX_ErrorInputDataDecodeYet;
#else
            pOutputData->timeStamp = pInputData->timeStamp;
            pOutputData->nFlags = pInputData->nFlags;

            ret = OMX_ErrorNone;
#endif
            goto EXIT;
        } else {
            ret = OMX_ErrorDMAIInit;
            goto EXIT;
        }
    }

#ifndef FULL_FRAME_SEARCH
    if ((pInputData->nFlags & OMX_BUFFERFLAG_ENDOFFRAME) &&
        (pNAMComponent->bUseFlagEOF == OMX_FALSE))
        pNAMComponent->bUseFlagEOF = OMX_TRUE;
#endif

    pNAMComponent->timeStamp[pH264Dec->hDMAIH264Handle.indexTimestamp] = pInputData->timeStamp;
    pNAMComponent->nFlags[pH264Dec->hDMAIH264Handle.indexTimestamp] = pInputData->nFlags;

    if ((pH264Dec->hDMAIH264Handle.returnCodec == DMAI_RET_OK) &&
        (pH264Dec->bFirstFrame == OMX_FALSE)) {
        SSBSIP_DMAI_DEC_OUTBUF_STATUS status;
        OMX_S32 indexTimestamp = 0;

        /* wait for dmai decode done */
        if (pH264Dec->NBDecThread.bDecoderRun == OMX_TRUE) {
            NAM_OSAL_SemaphoreWait(pH264Dec->NBDecThread.hDecFrameEnd);
            pH264Dec->NBDecThread.bDecoderRun = OMX_FALSE;
        }

        status = SsbSipDmaiDecGetOutBuf(pH264Dec->hDMAIH264Handle.hDMAIHandle, &outputInfo);
        bufWidth  = (outputInfo.img_width + 15) & (~15);
        bufHeight = (outputInfo.img_height + 15) & (~15);

        if ((SsbSipDmaiDecGetConfig(pH264Dec->hDMAIH264Handle.hDMAIHandle, DMAI_DEC_GETCONF_FRAME_TAG, &indexTimestamp) != DMAI_RET_OK) ||
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

        if(status == DMAI_GETOUTBUF_DECODING_ONLY) {
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
        if ((pH264Dec->bFirstFrame == OMX_TRUE) &&
            ((pOutputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS)) {
            pOutputData->nFlags = (pOutputData->nFlags & (~OMX_BUFFERFLAG_EOS));
        }

        outputDataValid = OMX_FALSE;

        /* ret = OMX_ErrorUndefined; */
        ret = OMX_ErrorNone;
    }

    if (ret == OMX_ErrorInputDataDecodeYet) {
        pH264Dec->DMAIDecInputBuffer[pH264Dec->indexInputBuffer].dataSize = oneFrameSize;
        pH264Dec->indexInputBuffer++;
        pH264Dec->indexInputBuffer %= DMAI_INPUT_BUFFER_NUM_MAX;
        pH264Dec->hDMAIH264Handle.pDMAIStreamBuffer    = pH264Dec->DMAIDecInputBuffer[pH264Dec->indexInputBuffer].VirAddr;
        pH264Dec->hDMAIH264Handle.pDMAIStreamPhyBuffer = pH264Dec->DMAIDecInputBuffer[pH264Dec->indexInputBuffer].PhyAddr;
        pNAMComponent->processData[INPUT_PORT_INDEX].dataBuffer = pH264Dec->DMAIDecInputBuffer[pH264Dec->indexInputBuffer].VirAddr;
        pNAMComponent->processData[INPUT_PORT_INDEX].allocSize = pH264Dec->DMAIDecInputBuffer[pH264Dec->indexInputBuffer].bufferSize;
        oneFrameSize = pH264Dec->DMAIDecInputBuffer[pH264Dec->indexInputBuffer].dataSize;
        //pInputData->dataLen = oneFrameSize;
        //pInputData->remainDataLen = oneFrameSize;
    }

    if ((Check_H264_StartCode(pInputData->dataBuffer, pInputData->dataLen) == OMX_TRUE) &&
        ((pOutputData->nFlags & OMX_BUFFERFLAG_EOS) != OMX_BUFFERFLAG_EOS)) {
        SsbSipDmaiDecSetConfig(pH264Dec->hDMAIH264Handle.hDMAIHandle, DMAI_DEC_SETCONF_FRAME_TAG, &(pH264Dec->hDMAIH264Handle.indexTimestamp));
        pH264Dec->hDMAIH264Handle.indexTimestamp++;
        pH264Dec->hDMAIH264Handle.indexTimestamp %= MAX_TIMESTAMP;

        SsbSipDmaiDecSetInBuf(pH264Dec->hDMAIH264Handle.hDMAIHandle,
                             pH264Dec->hDMAIH264Handle.pDMAIStreamPhyBuffer,
                             pH264Dec->hDMAIH264Handle.pDMAIStreamBuffer,
                             pNAMComponent->processData[INPUT_PORT_INDEX].allocSize);

        pH264Dec->DMAIDecInputBuffer[pH264Dec->indexInputBuffer].dataSize = oneFrameSize;
        pH264Dec->NBDecThread.oneFrameSize = oneFrameSize;

        /* dmai decode start */
        NAM_OSAL_SemaphorePost(pH264Dec->NBDecThread.hDecFrameStart);
        pH264Dec->NBDecThread.bDecoderRun = OMX_TRUE;
        pH264Dec->hDMAIH264Handle.returnCodec = DMAI_RET_OK;

        pH264Dec->indexInputBuffer++;
        pH264Dec->indexInputBuffer %= DMAI_INPUT_BUFFER_NUM_MAX;
        pH264Dec->hDMAIH264Handle.pDMAIStreamBuffer    = pH264Dec->DMAIDecInputBuffer[pH264Dec->indexInputBuffer].VirAddr;
        pH264Dec->hDMAIH264Handle.pDMAIStreamPhyBuffer = pH264Dec->DMAIDecInputBuffer[pH264Dec->indexInputBuffer].PhyAddr;
        pNAMComponent->processData[INPUT_PORT_INDEX].dataBuffer = pH264Dec->DMAIDecInputBuffer[pH264Dec->indexInputBuffer].VirAddr;
        pNAMComponent->processData[INPUT_PORT_INDEX].allocSize = pH264Dec->DMAIDecInputBuffer[pH264Dec->indexInputBuffer].bufferSize;
        if (((pH264Dec->hDMAIH264Handle.bThumbnailMode == OMX_TRUE) || (pNAMComponent->bSaveFlagEOS == OMX_TRUE)) &&
            (pH264Dec->bFirstFrame == OMX_TRUE) &&
            (outputDataValid == OMX_FALSE)) {
            ret = OMX_ErrorInputDataDecodeYet;
        }
        pH264Dec->bFirstFrame = OMX_FALSE;
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

            retANB = getVADDRfromANB (pOutputData->dataBuffer,
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
        if ((pH264Dec->hDMAIH264Handle.bThumbnailMode == OMX_FALSE) &&
            (pNAMOutputPort->portDefinition.format.video.eColorFormat == OMX_NAM_COLOR_FormatNV12TPhysicalAddress))
        {
            /* if use Post copy address structure */
            NAM_OSAL_Memcpy(pOutputBuf[0], &frameSize, sizeof(frameSize));
            NAM_OSAL_Memcpy(pOutputBuf[0] + sizeof(frameSize), &(outputInfo.YPhyAddr), sizeof(outputInfo.YPhyAddr));
            NAM_OSAL_Memcpy(pOutputBuf[0] + sizeof(frameSize) + (sizeof(void *) * 1), &(outputInfo.CPhyAddr), sizeof(outputInfo.CPhyAddr));
            NAM_OSAL_Memcpy(pOutputBuf[0] + sizeof(frameSize) + (sizeof(void *) * 2), &(outputInfo.YVirAddr), sizeof(outputInfo.YVirAddr));
            NAM_OSAL_Memcpy(pOutputBuf[0] + sizeof(frameSize) + (sizeof(void *) * 3), &(outputInfo.CVirAddr), sizeof(outputInfo.CVirAddr));
            pOutputData->dataLen = (bufWidth * bufHeight * 3) / 2;
        } else {
            switch (pNAMOutputPort->portDefinition.format.video.eColorFormat) {
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
            case OMX_NAM_COLOR_FormatANBYUV420SemiPlanar:
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

#endif // #if ENABLE_MFC

EXIT:
    FunctionOut();

    return ret;
}

/* DMAI Decode */
OMX_ERRORTYPE NAM_DMAI_H264Dec_bufferProcess(OMX_COMPONENTTYPE *pOMXComponent, NAM_OMX_DATA *pInputData, NAM_OMX_DATA *pOutputData)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    NAM_OMX_BASECOMPONENT   *pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    NAM_H264DEC_HANDLE      *pH264Dec = (NAM_H264DEC_HANDLE *)pNAMComponent->hCodecHandle;
    NAM_OMX_BASEPORT        *pNAMInputPort = &pNAMComponent->pNAMPort[INPUT_PORT_INDEX];
    NAM_OMX_BASEPORT        *pNAMOutputPort = &pNAMComponent->pNAMPort[OUTPUT_PORT_INDEX];
    OMX_BOOL                 endOfFrame = OMX_FALSE;

    FunctionIn();

    if ((!CHECK_PORT_ENABLED(pNAMInputPort)) || (!CHECK_PORT_ENABLED(pNAMOutputPort)) ||
            (!CHECK_PORT_POPULATED(pNAMInputPort)) || (!CHECK_PORT_POPULATED(pNAMOutputPort))) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }
    if (OMX_FALSE == NAM_Check_BufferProcess_State(pNAMComponent)) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    ret = NAM_DMAI_H264_Decode(pOMXComponent, pInputData, pOutputData);
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
    NAM_H264DEC_HANDLE      *pH264Dec = NULL;
    int i = 0;

    FunctionIn();

    if ((hComponent == NULL) || (componentName == NULL)) {
        ret = OMX_ErrorBadParameter;
        NAM_OSAL_Log(NAM_LOG_ERROR, "OMX_ErrorBadParameter, Line:%d", __LINE__);
        goto EXIT;
    }
    if (NAM_OSAL_Strcmp(NAM_OMX_COMPONENT_H264_DEC, componentName) != 0) {
        ret = OMX_ErrorBadParameter;
        NAM_OSAL_Log(NAM_LOG_ERROR, "OMX_ErrorBadParameter, componentName:%s, Line:%d", componentName, __LINE__);
        goto EXIT;
    }

    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = NAM_OMX_VideoDecodeComponentInit(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        NAM_OSAL_Log(NAM_LOG_ERROR, "OMX_Error, Line:%d", __LINE__);
        goto EXIT;
    }
    pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pNAMComponent->codecType = HW_VIDEO_CODEC;

    pNAMComponent->componentName = (OMX_STRING)NAM_OSAL_Malloc(MAX_OMX_COMPONENT_NAME_SIZE);
    if (pNAMComponent->componentName == NULL) {
        NAM_OMX_VideoDecodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        NAM_OSAL_Log(NAM_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    NAM_OSAL_Memset(pNAMComponent->componentName, 0, MAX_OMX_COMPONENT_NAME_SIZE);

    pH264Dec = NAM_OSAL_Malloc(sizeof(NAM_H264DEC_HANDLE));
    if (pH264Dec == NULL) {
        NAM_OMX_VideoDecodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        NAM_OSAL_Log(NAM_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    NAM_OSAL_Memset(pH264Dec, 0, sizeof(NAM_H264DEC_HANDLE));
    pNAMComponent->hCodecHandle = (OMX_HANDLETYPE)pH264Dec;

    NAM_OSAL_Strcpy(pNAMComponent->componentName, NAM_OMX_COMPONENT_H264_DEC);
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
    pNAMPort->portDefinition.format.video.nStride = 0; /*DEFAULT_FRAME_WIDTH;*/
    pNAMPort->portDefinition.format.video.nSliceHeight = 0;
    pNAMPort->portDefinition.nBufferSize = DEFAULT_VIDEO_INPUT_BUFFER_SIZE;
    pNAMPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
    NAM_OSAL_Memset(pNAMPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
    NAM_OSAL_Strcpy(pNAMPort->portDefinition.format.video.cMIMEType, "video/avc");
    pNAMPort->portDefinition.format.video.pNativeRender = 0;
    pNAMPort->portDefinition.format.video.bFlagErrorConcealment = OMX_FALSE;
    pNAMPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    pNAMPort->portDefinition.bEnabled = OMX_TRUE;

    /* Output port */
    pNAMPort = &pNAMComponent->pNAMPort[OUTPUT_PORT_INDEX];
    pNAMPort->portDefinition.format.video.nFrameWidth = DEFAULT_FRAME_WIDTH;
    pNAMPort->portDefinition.format.video.nFrameHeight= DEFAULT_FRAME_HEIGHT;
    pNAMPort->portDefinition.format.video.nStride = 0; /*DEFAULT_FRAME_WIDTH;*/
    pNAMPort->portDefinition.format.video.nSliceHeight = 0;
    pNAMPort->portDefinition.nBufferSize = DEFAULT_VIDEO_OUTPUT_BUFFER_SIZE;
    pNAMPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    NAM_OSAL_Memset(pNAMPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
    NAM_OSAL_Strcpy(pNAMPort->portDefinition.format.video.cMIMEType, "raw/video");
    pNAMPort->portDefinition.format.video.pNativeRender = 0;
    pNAMPort->portDefinition.format.video.bFlagErrorConcealment = OMX_FALSE;
    pNAMPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
    pNAMPort->portDefinition.bEnabled = OMX_TRUE;

    for(i = 0; i < ALL_PORT_NUM; i++) {
        INIT_SET_SIZE_VERSION(&pH264Dec->AVCComponent[i], OMX_VIDEO_PARAM_AVCTYPE);
        pH264Dec->AVCComponent[i].nPortIndex = i;
        pH264Dec->AVCComponent[i].eProfile   = OMX_VIDEO_AVCProfileBaseline;
        pH264Dec->AVCComponent[i].eLevel     = OMX_VIDEO_AVCLevel4;
    }

    pOMXComponent->GetParameter      = &NAM_DMAI_H264Dec_GetParameter;
    pOMXComponent->SetParameter      = &NAM_DMAI_H264Dec_SetParameter;
    pOMXComponent->GetConfig         = &NAM_DMAI_H264Dec_GetConfig;
    pOMXComponent->SetConfig         = &NAM_DMAI_H264Dec_SetConfig;
    pOMXComponent->GetExtensionIndex = &NAM_DMAI_H264Dec_GetExtensionIndex;
    pOMXComponent->ComponentRoleEnum = &NAM_DMAI_H264Dec_ComponentRoleEnum;
    pOMXComponent->ComponentDeInit   = &NAM_OMX_ComponentDeinit;

    pNAMComponent->nam_dmai_componentInit      = &NAM_DMAI_H264Dec_Init;
    pNAMComponent->nam_dmai_componentTerminate = &NAM_DMAI_H264Dec_Terminate;
    pNAMComponent->nam_dmai_bufferProcess      = &NAM_DMAI_H264Dec_bufferProcess;
    pNAMComponent->nam_checkInputFrame        = &Check_H264_Frame;

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
    NAM_H264DEC_HANDLE      *pH264Dec = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    NAM_OSAL_Free(pNAMComponent->componentName);
    pNAMComponent->componentName = NULL;

    pH264Dec = (NAM_H264DEC_HANDLE *)pNAMComponent->hCodecHandle;
    if (pH264Dec != NULL) {
        NAM_OSAL_Free(pH264Dec);
        pH264Dec = pNAMComponent->hCodecHandle = NULL;
    }

    ret = NAM_OMX_VideoDecodeComponentDeinit(pOMXComponent);
    if(ret != OMX_ErrorNone) {
        goto EXIT;
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

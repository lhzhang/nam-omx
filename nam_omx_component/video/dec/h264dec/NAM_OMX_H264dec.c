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
#include "NAM_OSAL_Semaphore.h"
#include "NAM_OSAL_Thread.h"
#include "library_register.h"
#include "NAM_OMX_H264dec.h"
#include "NAM_OSAL_ETC.h"
#include "color_space_convertor.h"

#include "ticodec.h"
#include "ti_dmai_iface.h"

#undef  NAM_LOG_TAG
#define NAM_LOG_TAG    "NAM_H264_DEC"
//#define NAM_LOG_OFF
#define NAM_TRACE_ON
#include "NAM_OSAL_Log.h"

#define BUFSIZEALIGN 128
#define FIX_CALC_DATA_LEN 1

/* As 0 is not a valid buffer id in XDM 1.0, we need macros for easy access */
//#define GETID(x)  ((x) + 1)
#define GETID(x)  ((x))
#define GETIDX(x) ((x) - 1)

//#define ADD_SPS_PPS_I_FRAME
//#define FULL_FRAME_SEARCH

#define DMAI_RET_OK Dmai_EOK

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
            case OMX_SEC_COLOR_FormatNV12TPhysicalAddress:
            case OMX_SEC_COLOR_FormatANBYUV420SemiPlanar:
                pNAMOutputPort->portDefinition.nBufferSize = (width * height * 3) / 2;
                break;
            case OMX_COLOR_FormatCbYCrY:
		// VIDDEC_COLORFORMAT422, or XDM_YUV_422ILE
                pNAMOutputPort->portDefinition.nBufferSize = width * height * 2;
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
            NAM_OSAL_Log(NAM_LOG_TRACE, "== == 11 NAM_DMAI_DecodeThread--Decode start, input size: %d", Buffer_getNumBytesUsed(pH264Dec->hDMAIH264Handle.hInBuf));
            pH264Dec->hDMAIH264Handle.returnCodec = Vdec2_process(pH264Dec->hDMAIH264Handle.hVd2,
			pH264Dec->hDMAIH264Handle.hInBuf, pH264Dec->hDMAIH264Handle.hDstBuf);
	    NAM_OSAL_Log(NAM_LOG_TRACE, "== == 22 NAM_DMAI_DecodeThread--Decode end, returnCodec: %d", pH264Dec->hDMAIH264Handle.returnCodec);
            NAM_OSAL_SemaphorePost(pH264Dec->NBDecThread.hDecFrameEnd);
        }
    }

EXIT:
    NAM_OSAL_ThreadExit(NULL);
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_DMAI_H264Dec_AllocateBuffer(
    OMX_COMPONENTTYPE                *pOMXComponent,
    OMX_U32                          nPortIndex,
    OMX_IN OMX_U32                   nSizeBytes)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    NAM_OMX_BASECOMPONENT *pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    NAM_OMX_BASEPORT      *pNAMOutputPort = &pNAMComponent->pNAMPort[OUTPUT_PORT_INDEX];
    NAM_H264DEC_HANDLE    *pH264Dec = (NAM_H264DEC_HANDLE *)pNAMComponent->hCodecHandle;

    Buffer_Handle hBuf;
    Int32 frameSize;
    Int32 bufSize;
    Int numBufs, numCodecBuffers, numExpBufs;
    Int displayBufs = 0;
    Vdec2_Handle            hVd2		= NULL;
    BufTab_Handle           hOutBufTab		= NULL;
    BufferGfx_Attrs         gfxAttrs            = BufferGfx_Attrs_DEFAULT;
    VIDDEC2_Params          params              = Vdec2_Params_DEFAULT;

    FunctionIn();

#if 0
    if (pH264Dec->hDMAIH264Handle.hVd2 == NULL && pH264Dec->bDecTerminate == FALSE) {
        NAM_OSAL_SemaphoreWait(pH264Dec->hDMAIH264Handle->hDecCreated);
    }
#endif

    /* Wait until decoder created. it is a temporary solution, should call NAM_OSAL_SemaphoreWait or NAM_OSAL_SignalWait fxns */
    while(pH264Dec->hDMAIH264Handle.bConfiguredDMAI == OMX_FALSE && pH264Dec->bDecTerminate == OMX_FALSE) {
        //NAM_OSAL_Log(NAM_LOG_TRACE, "Sleep...");
    	NAM_OSAL_SleepMillinam(2);  
    }

    if(pH264Dec->bDecTerminate == OMX_TRUE)
    {
            /* ret = OMX_ErrorUndefined; */
            /* ret = OMX_ErrorInsufficientResources; */
            ret = OMX_ErrorNone;
            NAM_OSAL_Log(NAM_LOG_ERROR, "Failed to allocate BufTab for H264Dec as it is terminated, Line: %d", __LINE__);
            goto EXIT;
    }


    hVd2 = pH264Dec->hDMAIH264Handle.hVd2;

    if(pH264Dec->hDMAIH264Handle.bCreateBufTabOneShot) {
        NAM_OSAL_Log(NAM_LOG_TRACE, "Create BufTab OneShot for H264Dec, Line: %d", __LINE__);

        pH264Dec->hDMAIH264Handle.bCreateBufTabOneShot = OMX_FALSE;

        /* ...specify actual frame size */
        params.maxWidth = pNAMOutputPort->portDefinition.format.video.nStride;
        params.maxHeight = pNAMOutputPort->portDefinition.format.video.nSliceHeight;
        params.forceChromaFormat = XDM_YUV_422ILE;

        bufSize = Vdec2_getOutBufSize(hVd2);
        //bufSize = Dmai_roundUp(Vdec2_getOutBufSize(hVd), 32);
	//bufSize = Dmai_roundUp(Vdec2_getOutBufSize(hVd2), BUFSIZEALIGN); /* BUFSIZEALIGN = 128 */
        NAM_OSAL_Log(NAM_LOG_TRACE, "params.maxWidth: %d, params.maxHeight: %d, bufSize: %d, nSizeBytes: %d", params.maxWidth, params.maxHeight, bufSize, nSizeBytes);
	if(nSizeBytes != bufSize) {
            ret = OMX_ErrorInsufficientResources;
            NAM_OSAL_Log(NAM_LOG_ERROR, "Failed to create output buffers, the given[size: %d] does not meet the needs of the decoder[size: %d], Line: %d", __LINE__);
            goto EXIT;
	}

        gfxAttrs.colorSpace = ColorSpace_UYVY;
        gfxAttrs.dim.width = params.maxWidth;
        gfxAttrs.dim.height = params.maxHeight;
        gfxAttrs.dim.lineLength = BufferGfx_calcLineLength (gfxAttrs.dim.width, gfxAttrs.colorSpace);
        gfxAttrs.bAttrs.useMask = OMX_DSP_CODEC_MASK | OMX_DSP_DISPLAY_MASK;

        /* Create buffers table (number of buffers are the same as the output port) */
        hOutBufTab = BufTab_create (OMX_DSP_OUTPUT_BUFTAB_NUMBER, bufSize, BufferGfx_getBufferAttrs(&gfxAttrs));
        if(hOutBufTab == NULL) {
            ret = OMX_ErrorInsufficientResources;
            NAM_OSAL_Log(NAM_LOG_ERROR, "Failed to create output buffers table, Line: %d", __LINE__);
            goto EXIT;
        }
        /* And bind it to decoder display buffers */
        Vdec2_setBufTab (hVd2, hOutBufTab);
        pH264Dec->hDMAIH264Handle.hOutBufTab = hOutBufTab;
    }

    if(pH264Dec->hDMAIH264Handle.bResizeBufTab) {
        NAM_OSAL_Log(NAM_LOG_TRACE, "Resize BufTab for H264Dec, Line: %d", __LINE__);

        pH264Dec->hDMAIH264Handle.bResizeBufTab = OMX_FALSE;

        hOutBufTab = pH264Dec->hDMAIH264Handle.hOutBufTab;

	/* How many buffers can the codec keep at one time? */
        numCodecBuffers = Vdec2_getMinOutBufs(hVd2);
        if(numCodecBuffers < 0) {
            ret = OMX_ErrorInsufficientResources;
            NAM_OSAL_Log(NAM_LOG_ERROR, "Failed to get buffer requirements: %d, Line: %d", numCodecBuffers, __LINE__);
            goto EXIT;
	}

        /*
         * Total number of frames needed are the number of buffers the codec
         * can keep at any time, plus the number of frames in the display pipe.
         */
        numBufs = numCodecBuffers + displayBufs;

        /* Get the size of output buffers needed from codec */
        frameSize = Vdec2_getOutBufSize(hVd2);

        /*
         * Get the first buffer of the BufTab to determine buffer characteristics.
         * All buffers in a BufTab share the same characteristics.
         */
        hBuf = BufTab_getBuf(hOutBufTab, 0);
        /* Do we need to resize the BufTab? */
        if (numBufs > BufTab_getNumBufs(hOutBufTab) || frameSize < Buffer_getSize(hBuf)) {

            /* Should we break the current buffers in to many smaller buffers? */
            if(frameSize < Buffer_getSize(hBuf)) {

                /*
                 * Chunk the larger buffers of the BufTab in to smaller buffers
                 * to accomodate the codec requirements.
                 */
                numExpBufs = BufTab_chunk(hOutBufTab, numBufs, frameSize);

                if(numExpBufs < 0) {
                    ret = OMX_ErrorInsufficientResources;
                    NAM_OSAL_Log(NAM_LOG_ERROR, "Failed to chunk %d bufs size %ld to %d bufs size %ld, Line: %d",
                        BufTab_getNumBufs(hOutBufTab), Buffer_getSize(hBuf), numBufs, frameSize, __LINE__);
                    goto EXIT;
                }

                /*
                 * Did the current BufTab fit the chunked buffers,
                 * or do we need to expand the BufTab (numExpBufs > 0)?
                 */
                if(BufTab_expand(hOutBufTab, numExpBufs) < 0) {
                    ret = OMX_ErrorInsufficientResources;
                    NAM_OSAL_Log(NAM_LOG_ERROR, "Failed to expand BufTab with %d buffers after chunk,  Line: %d",
                        numExpBufs, __LINE__);
                    goto EXIT;
                }
            } else {
                /* Just expand the BufTab with more buffers */
                if(BufTab_expand(hOutBufTab, (numBufs - BufTab_getNumBufs(hOutBufTab))) < 0) {
                    ret = OMX_ErrorInsufficientResources;
                    NAM_OSAL_Log(NAM_LOG_ERROR, "Failed to expand BufTab with %d buffers,  Line: %d",
                        (numBufs - BufTab_getNumBufs(hOutBufTab)), __LINE__);
                    goto EXIT;
                }
            }
        }
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_DMAI_H264Dec_ReleaseBuffer(
    OMX_COMPONENTTYPE                *pOMXComponent,
    OMX_BUFFERHEADERTYPE             *pBufferHeader)
{
    OMX_ERRORTYPE         ret = OMX_ErrorNone;
    Buffer_Handle         hDispBuf = (Buffer_Handle)pBufferHeader->pPlatformPrivate;

    if(hDispBuf != NULL) {
        NAM_OSAL_Log(NAM_LOG_TRACE, "== == Release Buffer(set UseMark)");
        /* Mark the buffer is not owned by renderer anymore */
        Buffer_freeUseMask(hDispBuf, OMX_DSP_DISPLAY_MASK);
    } else {
        NAM_OSAL_Log(NAM_LOG_TRACE, "== == Release Buffer(do Nothing)");
    }

EXIT:
    FunctionOut();

    return ret;
}

/* Release all allocated resources */
static OMX_ERRORTYPE NAM_DMAI_H264Dec_CleanUp(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    NAM_OMX_BASECOMPONENT *pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    NAM_H264DEC_HANDLE    *pH264Dec = NULL;
    OMX_U32                i;

    FunctionIn();

    pH264Dec = (NAM_H264DEC_HANDLE *)pNAMComponent->hCodecHandle;

    /* Delete decoder */
    if(pH264Dec->hDMAIH264Handle.hVd2 != NULL) {
        Vdec2_delete(pH264Dec->hDMAIH264Handle.hVd2);
        pH264Dec->hDMAIH264Handle.hVd2 = NULL;
    }

    /* Delete engine */
    if(pH264Dec->hDMAIH264Handle.hEngine != NULL) {
        TIDMmaiFreeHandle(pH264Dec->hDMAIH264Handle.hEngine);
        pH264Dec->hDMAIH264Handle.hEngine = NULL;
    }

    /* Delete input codec buffer */
    for(i = 0; i < DMAI_INPUT_BUFFER_NUM_MAX; i++) {
        if(pH264Dec->DMAIDecInputBuffer[i].hBuffer != NULL) {
            Buffer_delete(pH264Dec->DMAIDecInputBuffer[i].hBuffer);
            pH264Dec->DMAIDecInputBuffer[i].hBuffer = NULL;
        }
    }

    /* Delete output codec buffers */
    if (pH264Dec->hDMAIH264Handle.hOutBufTab != NULL) {
        /* Mark all buffers as free, destroy any resizing data and destroy codec engine */
        BufTab_freeAll(pH264Dec->hDMAIH264Handle.hOutBufTab);
        BufTab_collapse(pH264Dec->hDMAIH264Handle.hOutBufTab);
        /* Delete output buffer table if needed (TBD) */
        BufTab_delete(pH264Dec->hDMAIH264Handle.hOutBufTab);
        pH264Dec->hDMAIH264Handle.hOutBufTab = NULL;
    }

EXIT:
    FunctionOut();

    return ret;
}
/* DMAI Init */
OMX_ERRORTYPE NAM_DMAI_H264Dec_Init(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    NAM_OMX_BASECOMPONENT *pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    NAM_OMX_BASEPORT      *pNAMOutputPort = &pNAMComponent->pNAMPort[OUTPUT_PORT_INDEX];
    NAM_H264DEC_HANDLE    *pH264Dec = NULL;
    OMX_U32                i;
    
    OMX_U32 DSPBufferSize;
    Engine_Error ec;
    Engine_Handle hEngine    = NULL;
    Vdec2_Handle hVd2        = NULL;
    Buffer_Handle hDSPBuffer = NULL;
    BufTab_Handle hOutBufTab = NULL;
    Buffer_Attrs            bAttrs	        = Buffer_Attrs_DEFAULT;
    VIDDEC2_Params          params              = Vdec2_Params_DEFAULT;
    VIDDEC2_DynamicParams   dynParams           = Vdec2_DynamicParams_DEFAULT;

    FunctionIn();

    pH264Dec = (NAM_H264DEC_HANDLE *)pNAMComponent->hCodecHandle;
    pH264Dec->hDMAIH264Handle.bConfiguredDMAI = OMX_FALSE;
    pH264Dec->hDMAIH264Handle.hEngine = NULL;
    pH264Dec->hDMAIH264Handle.hVd2 = NULL;
    pH264Dec->hDMAIH264Handle.hDSPBuffer = NULL;
    pH264Dec->hDMAIH264Handle.pDSPBuffer = NULL;
    pH264Dec->hDMAIH264Handle.bCreateBufTabOneShot = OMX_TRUE;
    pH264Dec->hDMAIH264Handle.bResizeBufTab = OMX_FALSE;

    pNAMComponent->bUseFlagEOF = OMX_FALSE;
    pNAMComponent->bSaveFlagEOS = OMX_FALSE;

    hEngine = TIDmaiDetHandle();
    if (hEngine == NULL) {
        NAM_OSAL_Log(NAM_LOG_ERROR, "Invalid DSP Engine Handle. Line:%d", __LINE__);
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    pH264Dec->hDMAIH264Handle.hEngine = hEngine;

    /* ...specify actual frame size */
    params.maxWidth = pNAMOutputPort->portDefinition.format.video.nStride;
    params.maxHeight = pNAMOutputPort->portDefinition.format.video.nSliceHeight;
    params.forceChromaFormat = XDM_YUV_422ILE;

    hVd2 = Vdec2_create(hEngine, pH264Dec->hDMAIH264Handle.decoderName, &params, &dynParams);
    if (hVd2 == NULL) {
        ret = OMX_ErrorInsufficientResources;
        NAM_OSAL_Log(NAM_LOG_ERROR, "Failed to open decoder %s, Line: %d", pH264Dec->hDMAIH264Handle.decoderName, __LINE__);
        goto EXIT;
    }
    pH264Dec->hDMAIH264Handle.hVd2 = hVd2;

    DSPBufferSize = Vdec2_getInBufSize(hVd2);

    for (i = 0; i < DMAI_INPUT_BUFFER_NUM_MAX; i++) {
        /* Allocate decoder's input buffer, only one */
        hDSPBuffer = Buffer_create (DSPBufferSize, &bAttrs);
        if (hDSPBuffer == NULL) {
            ret = OMX_ErrorInsufficientResources;
            NAM_OSAL_Log(NAM_LOG_ERROR, "Failed to create input buffer, Line: %d", __LINE__);
            goto EXIT;
        }

        pH264Dec->DMAIDecInputBuffer[i].hBuffer = hDSPBuffer;
        pH264Dec->DMAIDecInputBuffer[i].pBuffer = (OMX_U8 *)Buffer_getUserPtr(hDSPBuffer);
        pH264Dec->DMAIDecInputBuffer[i].bufferSize = DSPBufferSize;
        pH264Dec->DMAIDecInputBuffer[i].dataSize = 0;

        NAM_OSAL_Log(NAM_LOG_TRACE, "Successfully create input DSP buffer[%d], size: %d", i, DSPBufferSize);
    }

    pH264Dec->bFirstFrame = OMX_TRUE;

    pH264Dec->NBDecThread.bExitDecodeThread = OMX_FALSE;
    pH264Dec->NBDecThread.bDecoderRun = OMX_FALSE;
    pH264Dec->NBDecThread.oneFrameSize = 0;
    NAM_OSAL_SemaphoreCreate(&(pH264Dec->NBDecThread.hDecFrameStart));
    NAM_OSAL_SemaphoreCreate(&(pH264Dec->NBDecThread.hDecFrameEnd));
    if (OMX_ErrorNone == NAM_OSAL_ThreadCreate(&pH264Dec->NBDecThread.hNBDecodeThread,
                                                NAM_DMAI_DecodeThread,
                                                pOMXComponent)) {
        pH264Dec->hDMAIH264Handle.returnCodec = Dmai_EOK;
    }

    pH264Dec->hDMAIH264Handle.hDSPBuffer = pH264Dec->DMAIDecInputBuffer[0].hBuffer;
    pH264Dec->hDMAIH264Handle.pDSPBuffer = pH264Dec->DMAIDecInputBuffer[0].pBuffer;
    //pH264Dec->hDMAIH264Handle.DSPBufferSize = pH264Dec->DMAIDecInputBuffer[0].bufferSize;
    pNAMComponent->processData[INPUT_PORT_INDEX].hBuffer    = pH264Dec->DMAIDecInputBuffer[0].hBuffer;
    pNAMComponent->processData[INPUT_PORT_INDEX].dataBuffer = pH264Dec->DMAIDecInputBuffer[0].pBuffer;
    pNAMComponent->processData[INPUT_PORT_INDEX].allocSize  = pH264Dec->DMAIDecInputBuffer[0].bufferSize;

    NAM_OSAL_Memset(pNAMComponent->timeStamp, -19771003, sizeof(OMX_TICKS) * MAX_TIMESTAMP);
    NAM_OSAL_Memset(pNAMComponent->nFlags, 0, sizeof(OMX_U32) * MAX_FLAGS);
    pH264Dec->hDMAIH264Handle.indexTimestamp = 0;
    pNAMComponent->getAllDelayBuffer = OMX_FALSE;

    pH264Dec->hDMAIH264Handle.bConfiguredDMAI = OMX_TRUE;

EXIT:
    if(pH264Dec->hDMAIH264Handle.bConfiguredDMAI == OMX_FALSE) {
        /* Initialization failed, perform cleanup */
        NAM_DMAI_H264Dec_CleanUp(pOMXComponent);
    }

    FunctionOut();

    return ret;
}

/* DMAI Terminate */
OMX_ERRORTYPE NAM_DMAI_H264Dec_Terminate(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    NAM_OMX_BASECOMPONENT *pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    NAM_H264DEC_HANDLE    *pH264Dec = NULL;

    FunctionIn();

    pH264Dec = (NAM_H264DEC_HANDLE *)pNAMComponent->hCodecHandle;

    pH264Dec->bDecTerminate = OMX_TRUE;

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

    NAM_DMAI_H264Dec_CleanUp(pOMXComponent);

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
    OMX_S32                    indexTimestamp;
    OMX_U32                    numBytesUsed;

    Buffer_Handle              hFreeBuf = NULL;
    Buffer_Handle              hInBuf = NULL;
    Buffer_Handle              hDstBuf = NULL;
    Buffer_Handle              hOutBuf = NULL;
    OMX_S32                    setConfVal = 0;
    int                        bufWidth = 0;
    int                        bufHeight = 0;
    OMX_BOOL                   outputDataValid = OMX_FALSE;
    OMX_BOOL                   bGotOutBuf = OMX_FALSE;
    OMX_BOOL                   bRegetFreeBuf = OMX_TRUE;
    

    FunctionIn();

    if (pH264Dec->hDMAIH264Handle.bConfiguredDMAI == OMX_FALSE) {
        NAM_OSAL_Log(NAM_LOG_ERROR, "Error, DMAI decoder is not ready, Line: %d", __LINE__);
        ret = OMX_ErrorNotReady;
        goto EXIT;
    }
    
#if 0          
    if ((oneFrameSize <= 0) && (pInputData->nFlags & OMX_BUFFERFLAG_EOS)) {
        pOutputData->timeStamp = pInputData->timeStamp;
        pOutputData->nFlags = pInputData->nFlags;
        ret = OMX_ErrorNone;
        goto EXIT;
    }
#endif
#if 0
    if((namInputPort->portDefinition.format.video.nFrameWidth != imgResol.width) ||
        (namInputPort->portDefinition.format.video.nFrameHeight != imgResol.height)) {
        NAM_OSAL_Log(NAM_LOG_TRACE, "change width height information : OMX_EventPortSettingsChanged");
        /* change width and height information */
        namInputPort->portDefinition.format.video.nFrameWidth = imgResol.width;
        namInputPort->portDefinition.format.video.nFrameHeight = imgResol.height;
        namInputPort->portDefinition.format.video.nStride      = ((imgResol.width + 15) & (~15));
        namInputPort->portDefinition.format.video.nSliceHeight = ((imgResol.height + 15) & (~15));

        NAM_UpdateFrameSize(pOMXComponent);

        /* Send Port Settings changed call back */
        (*(pNAMComponent->pCallbacks->EventHandler))
        	(pOMXComponent,
        	pNAMComponent->callbackData,
        	OMX_EventPortSettingsChanged, /* The command was completed */
        	OMX_DirOutput, /* This is the port index */
        	0,
       		NULL);
    }
#endif

#ifndef FULL_FRAME_SEARCH
    if ((pInputData->nFlags & OMX_BUFFERFLAG_ENDOFFRAME) &&
        (pNAMComponent->bUseFlagEOF == OMX_FALSE))
        pNAMComponent->bUseFlagEOF = OMX_TRUE;
#endif

    //if ((pH264Dec->hDMAIH264Handle.returnCodec == Dmai_EOK) &&
    //    (pH264Dec->bFirstFrame == OMX_FALSE)) {
    if (pH264Dec->bFirstFrame == OMX_FALSE) {
        NAM_OSAL_Log(NAM_LOG_TRACE, "== == 33 == ready to wait");
        /* wait for dmai decode done */
        if (pH264Dec->NBDecThread.bDecoderRun == OMX_TRUE) {
            NAM_OSAL_Log(NAM_LOG_TRACE, "== == 44 == wait");
            NAM_OSAL_SemaphoreWait(pH264Dec->NBDecThread.hDecFrameEnd);
            pH264Dec->NBDecThread.bDecoderRun = OMX_FALSE;
        }
        NAM_OSAL_Log(NAM_LOG_TRACE, "== == 55 == wait end");

        if (pH264Dec->hDMAIH264Handle.returnCodec == Dmai_EOK) {
	    //OMX_S32 indexTimestamp = 0;
            indexTimestamp = 0;

            NAM_OSAL_Log(NAM_LOG_TRACE, "== == 66 == decode ok");
            /* Get a buffer for display from the codec */
            hOutBuf = Vdec2_getDisplayBuf(pH264Dec->hDMAIH264Handle.hVd2);
            if(hOutBuf == NULL) {
                NAM_OSAL_Log(NAM_LOG_ERROR, "== == Unable to get display buffer");
                bGotOutBuf = OMX_FALSE;
                hFreeBuf = Vdec2_getFreeBuf(pH264Dec->hDMAIH264Handle.hVd2);
                if(hFreeBuf != NULL) {
                    Buffer_setUseMask(hFreeBuf, 0);
                }
            } else {
                NAM_OSAL_Log(NAM_LOG_ERROR, "== == Successfully get display buffer");
                bGotOutBuf = OMX_TRUE;
            }
            /* Get a buffer to free from the codec */
            hFreeBuf = Vdec2_getFreeBuf(pH264Dec->hDMAIH264Handle.hVd2);
            while (hFreeBuf) {
                /* The codec is no longer using the buffer */
                Buffer_freeUseMask(hFreeBuf, OMX_DSP_CODEC_MASK);
                hFreeBuf = Vdec2_getFreeBuf(pH264Dec->hDMAIH264Handle.hVd2);
            }

            if (bGotOutBuf == OMX_TRUE) {
                indexTimestamp = GETID(Buffer_getId(hOutBuf));
                if ((indexTimestamp < 0) || (indexTimestamp >= MAX_TIMESTAMP)) {
                    NAM_OSAL_Log(NAM_LOG_TRACE, "== == 77 == bGotOutBuf OK, indexTimestamp ERR: %d", indexTimestamp);
                    pOutputData->timeStamp = pInputData->timeStamp;
                    pOutputData->nFlags = pInputData->nFlags;
                } else {
                    NAM_OSAL_Log(NAM_LOG_TRACE, "== == 88 == bGotOutBuf OK, indexTimestamp OK: %d", indexTimestamp);
                    pOutputData->timeStamp = pNAMComponent->timeStamp[indexTimestamp];
                    pOutputData->nFlags = pNAMComponent->nFlags[indexTimestamp];
                }

                outputDataValid = OMX_TRUE;
            } else {
                NAM_OSAL_Log(NAM_LOG_TRACE, "== == 99 == bGotOutBuf ERR");
                pOutputData->timeStamp = pInputData->timeStamp;
                pOutputData->nFlags = pInputData->nFlags;

                outputDataValid = OMX_FALSE;
            }
            NAM_OSAL_Log(NAM_LOG_TRACE, "== == timestamp %lld us (%.2f nams)", pOutputData->timeStamp, pOutputData->timeStamp / 1E6);

            if (pOutputData->nFlags & OMX_BUFFERFLAG_EOS) 
                outputDataValid = OMX_FALSE;

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
            if (pH264Dec->hDMAIH264Handle.returnCodec == Dmai_EFIRSTFIELD) {
                bRegetFreeBuf = OMX_FALSE;
                NAM_OSAL_Log(NAM_LOG_WARNING, "== == 100 == Dmai_EFIRSTFIELD");
            } else if (pH264Dec->hDMAIH264Handle.returnCodec == Dmai_EBITERROR) {
                /* If no encoded data was used we cannot find the next frame */
                NAM_OSAL_Log(NAM_LOG_ERROR, "== == 100 == Dmai_EBITERROR");
                /* if retrun code == 0, Fatal bit error */
                numBytesUsed = Buffer_getNumBytesUsed(pH264Dec->hDMAIH264Handle.hInBuf);
                if (numBytesUsed == 0) {
                    NAM_OSAL_Log(NAM_LOG_ERROR, "== == 100 == Dmai_EBITERROR, Fatal bit error");
                    BufTab_freeBuf(pH264Dec->hDMAIH264Handle.hDstBuf);
                    ret = OMX_ErrorUndefined;
                    goto EXIT;
                } else {
                    NAM_OSAL_Log(NAM_LOG_ERROR, "== == 100 == Dmai_EBITERROR, No Fatal bit error, please Check BitStream, numBytesUsed: %d", numBytesUsed);
                    bRegetFreeBuf = OMX_FALSE;
                }
            } else {
                NAM_OSAL_Log(NAM_LOG_WARNING, "== == 100 == Unknown Error: %d", pH264Dec->hDMAIH264Handle.returnCodec);
                ret = OMX_ErrorUndefined;
                goto EXIT;
            }

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
    }
#if 0
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
#endif
    if ((Check_H264_StartCode(pInputData->dataBuffer, pInputData->dataLen) == OMX_TRUE) &&
        ((pOutputData->nFlags & OMX_BUFFERFLAG_EOS) != OMX_BUFFERFLAG_EOS)) {
        NAM_OSAL_Log(NAM_LOG_TRACE, "== == 11 == ready to post, RegetFreeBuf: %d", bRegetFreeBuf);
        if (bRegetFreeBuf == OMX_TRUE) {
            /* Get a free buffer from the BufTab to give to the codec */
            hDstBuf = BufTab_getFreeBuf(pH264Dec->hDMAIH264Handle.hOutBufTab);
            if (hDstBuf == NULL) {
                NAM_OSAL_Log(NAM_LOG_ERROR, "== == Failed to get free buffer from BufTab");
                BufTab_print(pH264Dec->hDMAIH264Handle.hOutBufTab);
                ret = OMX_ErrorNone;	// ??
                goto EXIT;
            } else {
                indexTimestamp = GETID(Buffer_getId(hDstBuf));
                pH264Dec->hDMAIH264Handle.indexTimestamp = indexTimestamp;
                pNAMComponent->timeStamp[pH264Dec->hDMAIH264Handle.indexTimestamp] = pInputData->timeStamp;
                pNAMComponent->nFlags[pH264Dec->hDMAIH264Handle.indexTimestamp] = pInputData->nFlags;
                NAM_OSAL_Log(NAM_LOG_TRACE, "== == Successfully get free buffer from BufTab, indexTimestamp: %d", indexTimestamp);

                /* Make sure the whole buffer is used for output */
                BufferGfx_resetDimensions(hDstBuf);
            }
        } else {
            hDstBuf = pH264Dec->hDMAIH264Handle.hDstBuf;
        }

	/* input and output buffers in decoding thread. Similar to the function: SsbSipMfcDecSetInBuf */
        pH264Dec->hDMAIH264Handle.hInBuf  = pH264Dec->hDMAIH264Handle.hDSPBuffer;
        pH264Dec->hDMAIH264Handle.hDstBuf  = hDstBuf;

        pH264Dec->DMAIDecInputBuffer[pH264Dec->indexInputBuffer].dataSize = oneFrameSize;
        pH264Dec->NBDecThread.oneFrameSize = oneFrameSize;

        NAM_OSAL_Log(NAM_LOG_TRACE, "== == 22 == post");
        /* dmai decode start */
        NAM_OSAL_SemaphorePost(pH264Dec->NBDecThread.hDecFrameStart);
        pH264Dec->NBDecThread.bDecoderRun = OMX_TRUE;
        pH264Dec->hDMAIH264Handle.returnCodec = DMAI_RET_OK;

        pH264Dec->indexInputBuffer++;
        pH264Dec->indexInputBuffer %= DMAI_INPUT_BUFFER_NUM_MAX;
        pH264Dec->hDMAIH264Handle.hDSPBuffer  = pH264Dec->DMAIDecInputBuffer[pH264Dec->indexInputBuffer].hBuffer;
        pH264Dec->hDMAIH264Handle.pDSPBuffer  = pH264Dec->DMAIDecInputBuffer[pH264Dec->indexInputBuffer].pBuffer;
        pNAMComponent->processData[INPUT_PORT_INDEX].hBuffer = pH264Dec->DMAIDecInputBuffer[pH264Dec->indexInputBuffer].hBuffer;
        pNAMComponent->processData[INPUT_PORT_INDEX].dataBuffer = pH264Dec->DMAIDecInputBuffer[pH264Dec->indexInputBuffer].pBuffer;
        pNAMComponent->processData[INPUT_PORT_INDEX].allocSize = pH264Dec->DMAIDecInputBuffer[pH264Dec->indexInputBuffer].bufferSize;
#if 0
        if (((pH264Dec->hDMAIH264Handle.bThumbnailMode == OMX_TRUE) || (pNAMComponent->bSaveFlagEOS == OMX_TRUE)) &&
            (pH264Dec->bFirstFrame == OMX_TRUE) &&
            (outputDataValid == OMX_FALSE)) {
            ret = OMX_ErrorInputDataDecodeYet;
        }
#endif
        pH264Dec->bFirstFrame = OMX_FALSE;
    }

#if 0
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

#endif
    /** Fill Output Buffer **/
    if (outputDataValid == OMX_TRUE) {
        NAM_OMX_BASEPORT *pNAMInputPort = &pNAMComponent->pNAMPort[INPUT_PORT_INDEX];
        NAM_OMX_BASEPORT *pNAMOutputPort = &pNAMComponent->pNAMPort[OUTPUT_PORT_INDEX];
        //void *pOutputBuf[3];
        void *pOutputBuf;
        OMX_U32 width, height;

        //width = ((pNAMOutputPort->portDefinition.format.video.nFrameWidth + 15) & (~15));
        //height = ((pNAMOutputPort->portDefinition.format.video.nFrameHeight + 15) & (~15));
        width = pNAMOutputPort->portDefinition.format.video.nStride;
        height = pNAMOutputPort->portDefinition.format.video.nSliceHeight;

        pOutputBuf = (void *)pOutputData->dataBuffer;

        switch (pNAMOutputPort->portDefinition.format.video.eColorFormat) {
        case OMX_COLOR_FormatCbYCrY:
        {
            //NAM_OSAL_Log(NAM_LOG_TRACE, "== == CbYCrY out");
            NAM_OSAL_Log(NAM_LOG_TRACE, "== == CbYCrY out, hOutBuf id: %d, size: %d", Buffer_getId(hOutBuf), Buffer_getNumBytesUsed(hOutBuf));
            pOutputData->hBuffer = hOutBuf;
            pOutputData->dataBuffer = (OMX_U8 *)Buffer_getUserPtr(hOutBuf);
            //pOutputData->dataLen = width * height * 2;
            pOutputData->dataLen = Buffer_getNumBytesUsed(hOutBuf);
        }
            break;
        default:
        {
            NAM_OSAL_Log(NAM_LOG_TRACE, "== == No Format out");
	}
            break;
        }
    } else {
        NAM_OSAL_Log(NAM_LOG_TRACE, "== == No Data out");
        pOutputData->dataLen = 0;
    }

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

    NAM_OSAL_Log(NAM_LOG_TRACE, "== == NAM_DMAI_H264Dec_bufferProcess pInputData->datalen: %d, nFlags: %X", pInputData->dataLen, pInputData->nFlags);
    
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
#if FIX_CALC_DATA_LEN
        NAM_OSAL_Log(NAM_LOG_TRACE, "== == 11 NAM_DMAI_H264Dec_bufferProcess pInputData previousDataLen: %d, usedDataLen: %d, remainDataLen: %d, dataLen: %d", 
		pInputData->previousDataLen, pInputData->usedDataLen, pInputData->remainDataLen, pInputData->dataLen);
        pInputData->previousDataLen = pInputData->dataLen;
        pInputData->usedDataLen += pInputData->dataLen;
        pInputData->remainDataLen -= pInputData->usedDataLen;
        pInputData->dataLen -= pInputData->usedDataLen;	// ???
        pInputData->usedDataLen = 0;
        NAM_OSAL_Log(NAM_LOG_TRACE, "== == 22 NAM_DMAI_H264Dec_bufferProcess pInputData previousDataLen: %d, usedDataLen: %d, remainDataLen: %d, dataLen: %d", 
		pInputData->previousDataLen, pInputData->usedDataLen, pInputData->remainDataLen, pInputData->dataLen);
#else
        pInputData->previousDataLen = pInputData->dataLen;
        pInputData->usedDataLen += pInputData->dataLen;
        pInputData->remainDataLen = pInputData->dataLen - pInputData->usedDataLen;
        pInputData->dataLen -= pInputData->usedDataLen;
        pInputData->usedDataLen = 0;
#endif

        NAM_OSAL_Log(NAM_LOG_TRACE, "== == 33 NAM_DMAI_H264Dec_bufferProcess pOutputData dataLen: %d", pOutputData->dataLen);

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
    OMX_STRING              decoderName = NULL;
    Cpu_Device              device;
    int nameLen;
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

    decoderName = (OMX_STRING)NAM_OSAL_Malloc(OMX_MAX_STRINGNAME_SIZE);
    if(decoderName == NULL) {
        ret = OMX_ErrorInsufficientResources;
        NAM_OSAL_Log(NAM_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    NAM_OSAL_Strcpy(decoderName, DMAI_H264_DECODER_NAME);
    pH264Dec->hDMAIH264Handle.decoderName = decoderName;

    pH264Dec->bDecTerminate = OMX_FALSE;

        /* Determine which device type */
    if(Cpu_getDevice(NULL, &device) < 0) {
        ret = OMX_ErrorInsufficientResources;
        NAM_OSAL_Log(NAM_LOG_ERROR, "Cann't get the device type, Line:%d", __LINE__);
        goto EXIT;
    }

    switch (device) {
    case Cpu_Device_DM3730:
        NAM_OSAL_Log(NAM_LOG_TRACE, "Device type is DM3730");
        //numActualBufs = OMX_DSP_OUTPUT_BUFFER_NUMBER_HP;
        break;
    case Cpu_Device_OMAP3530:
        NAM_OSAL_Log(NAM_LOG_TRACE, "Device type is DM3530");
        //numActualBufs = OMX_DSP_OUTPUT_BUFFER_NUMBER;
        break;
    default:
        ret = OMX_ErrorInsufficientResources;
        NAM_OSAL_Log(NAM_LOG_ERROR, "The device type (%d) is not supported", device);
        goto EXIT;
    }

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
    pNAMComponent->nam_checkInputFrame         = &Check_H264_Frame;
    pNAMComponent->nam_dmai_allocate_buffer    = &NAM_DMAI_H264Dec_AllocateBuffer;
    pNAMComponent->nam_dmai_release_buffer     = &NAM_DMAI_H264Dec_ReleaseBuffer;

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
        if(pH264Dec->hDMAIH264Handle.decoderName) {
            NAM_OSAL_Free(pH264Dec->hDMAIH264Handle.decoderName);
        }
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

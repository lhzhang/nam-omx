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
 * @file        NAM_OMX_H264enc.c
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
#include "NAM_OMX_Venc.h"
#include "library_register.h"
#include "NAM_OMX_H264enc.h"
#include "SsbSipDmaiApi.h"
#include "color_space_convertor.h"

#undef  NAM_LOG_TAG
#define NAM_LOG_TAG    "NAM_H264_ENC"
#define NAM_LOG_OFF
#include "NAM_OSAL_Log.h"


/* H.264 Encoder Supported Levels & profiles */
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
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel32},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel4},

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
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel32},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel4},

    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel1},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel1b},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel11},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel12},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel13},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel2},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel21},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel22},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel3},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel31},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel32},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel4}};


OMX_U32 OMXAVCProfileToProfileIDC(OMX_VIDEO_AVCPROFILETYPE profile)
{
    OMX_U32 ret = 0; //default OMX_VIDEO_AVCProfileMain

    if (profile == OMX_VIDEO_AVCProfileMain)
        ret = 0;
    else if (profile == OMX_VIDEO_AVCProfileHigh)
        ret = 1;
    else if (profile == OMX_VIDEO_AVCProfileBaseline)
        ret = 2;

    return ret;
}

OMX_U32 OMXAVCLevelToLevelIDC(OMX_VIDEO_AVCLEVELTYPE level)
{
    OMX_U32 ret = 40; //default OMX_VIDEO_AVCLevel4

    if (level == OMX_VIDEO_AVCLevel1)
        ret = 10;
    else if (level == OMX_VIDEO_AVCLevel1b)
        ret = 9;
    else if (level == OMX_VIDEO_AVCLevel11)
        ret = 11;
    else if (level == OMX_VIDEO_AVCLevel12)
        ret = 12;
    else if (level == OMX_VIDEO_AVCLevel13)
        ret = 13;
    else if (level == OMX_VIDEO_AVCLevel2)
        ret = 20;
    else if (level == OMX_VIDEO_AVCLevel21)
        ret = 21;
    else if (level == OMX_VIDEO_AVCLevel22)
        ret = 22;
    else if (level == OMX_VIDEO_AVCLevel3)
        ret = 30;
    else if (level == OMX_VIDEO_AVCLevel31)
        ret = 31;
    else if (level == OMX_VIDEO_AVCLevel32)
        ret = 32;
    else if (level == OMX_VIDEO_AVCLevel4)
        ret = 40;

    return ret;
}

OMX_U8 *FindDelimiter(OMX_U8 *pBuffer, OMX_U32 size)
{
    int i;

    for (i = 0; i < size - 3; i++) {
        if ((pBuffer[i] == 0x00)   &&
            (pBuffer[i+1] == 0x00) &&
            (pBuffer[i+2] == 0x00) &&
            (pBuffer[i+3] == 0x01))
            return (pBuffer + i);
    }

    return NULL;
}

void H264PrintParams(SSBSIP_DMAI_ENC_H264_PARAM h264Arg)
{
    NAM_OSAL_Log(NAM_LOG_TRACE, "SourceWidth             : %d\n", h264Arg.SourceWidth);
    NAM_OSAL_Log(NAM_LOG_TRACE, "SourceHeight            : %d\n", h264Arg.SourceHeight);
    NAM_OSAL_Log(NAM_LOG_TRACE, "ProfileIDC              : %d\n", h264Arg.ProfileIDC);
    NAM_OSAL_Log(NAM_LOG_TRACE, "LevelIDC                : %d\n", h264Arg.LevelIDC);
    NAM_OSAL_Log(NAM_LOG_TRACE, "IDRPeriod               : %d\n", h264Arg.IDRPeriod);
    NAM_OSAL_Log(NAM_LOG_TRACE, "NumberReferenceFrames   : %d\n", h264Arg.NumberReferenceFrames);
    NAM_OSAL_Log(NAM_LOG_TRACE, "NumberRefForPframes     : %d\n", h264Arg.NumberRefForPframes);
    NAM_OSAL_Log(NAM_LOG_TRACE, "SliceMode               : %d\n", h264Arg.SliceMode);
    NAM_OSAL_Log(NAM_LOG_TRACE, "SliceArgument           : %d\n", h264Arg.SliceArgument);
    NAM_OSAL_Log(NAM_LOG_TRACE, "NumberBFrames           : %d\n", h264Arg.NumberBFrames);
    NAM_OSAL_Log(NAM_LOG_TRACE, "LoopFilterDisable       : %d\n", h264Arg.LoopFilterDisable);
    NAM_OSAL_Log(NAM_LOG_TRACE, "LoopFilterAlphaC0Offset : %d\n", h264Arg.LoopFilterAlphaC0Offset);
    NAM_OSAL_Log(NAM_LOG_TRACE, "LoopFilterBetaOffset    : %d\n", h264Arg.LoopFilterBetaOffset);
    NAM_OSAL_Log(NAM_LOG_TRACE, "SymbolMode              : %d\n", h264Arg.SymbolMode);
    NAM_OSAL_Log(NAM_LOG_TRACE, "PictureInterlace        : %d\n", h264Arg.PictureInterlace);
    NAM_OSAL_Log(NAM_LOG_TRACE, "Transform8x8Mode        : %d\n", h264Arg.Transform8x8Mode);
    NAM_OSAL_Log(NAM_LOG_TRACE, "RandomIntraMBRefresh    : %d\n", h264Arg.RandomIntraMBRefresh);
    NAM_OSAL_Log(NAM_LOG_TRACE, "PadControlOn            : %d\n", h264Arg.PadControlOn);
    NAM_OSAL_Log(NAM_LOG_TRACE, "LumaPadVal              : %d\n", h264Arg.LumaPadVal);
    NAM_OSAL_Log(NAM_LOG_TRACE, "CbPadVal                : %d\n", h264Arg.CbPadVal);
    NAM_OSAL_Log(NAM_LOG_TRACE, "CrPadVal                : %d\n", h264Arg.CrPadVal);
    NAM_OSAL_Log(NAM_LOG_TRACE, "EnableFRMRateControl    : %d\n", h264Arg.EnableFRMRateControl);
    NAM_OSAL_Log(NAM_LOG_TRACE, "EnableMBRateControl     : %d\n", h264Arg.EnableMBRateControl);
    NAM_OSAL_Log(NAM_LOG_TRACE, "FrameRate               : %d\n", h264Arg.FrameRate);
    NAM_OSAL_Log(NAM_LOG_TRACE, "Bitrate                 : %d\n", h264Arg.Bitrate);
    NAM_OSAL_Log(NAM_LOG_TRACE, "FrameQp                 : %d\n", h264Arg.FrameQp);
    NAM_OSAL_Log(NAM_LOG_TRACE, "QSCodeMax               : %d\n", h264Arg.QSCodeMax);
    NAM_OSAL_Log(NAM_LOG_TRACE, "QSCodeMin               : %d\n", h264Arg.QSCodeMin);
    NAM_OSAL_Log(NAM_LOG_TRACE, "CBRPeriodRf             : %d\n", h264Arg.CBRPeriodRf);
    NAM_OSAL_Log(NAM_LOG_TRACE, "DarkDisable             : %d\n", h264Arg.DarkDisable);
    NAM_OSAL_Log(NAM_LOG_TRACE, "SmoothDisable           : %d\n", h264Arg.SmoothDisable);
    NAM_OSAL_Log(NAM_LOG_TRACE, "StaticDisable           : %d\n", h264Arg.StaticDisable);
    NAM_OSAL_Log(NAM_LOG_TRACE, "ActivityDisable         : %d\n", h264Arg.ActivityDisable);
}

void Set_H264ENC_Param(SSBSIP_DMAI_ENC_H264_PARAM *pH264Arg, NAM_OMX_BASECOMPONENT *pNAMComponent)
{
    NAM_OMX_BASEPORT          *pNAMInputPort = NULL;
    NAM_OMX_BASEPORT          *pNAMOutputPort = NULL;
    NAM_H264ENC_HANDLE        *pH264Enc = NULL;

    pH264Enc = (NAM_H264ENC_HANDLE *)pNAMComponent->hCodecHandle;
    pNAMInputPort = &pNAMComponent->pNAMPort[INPUT_PORT_INDEX];
    pNAMOutputPort = &pNAMComponent->pNAMPort[OUTPUT_PORT_INDEX];

    pH264Arg->codecType    = H264_ENC;
    pH264Arg->SourceWidth  = pNAMOutputPort->portDefinition.format.video.nFrameWidth;
    pH264Arg->SourceHeight = pNAMOutputPort->portDefinition.format.video.nFrameHeight;
    pH264Arg->IDRPeriod    = pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].nPFrames + 1;
    pH264Arg->SliceMode    = 0;
    pH264Arg->RandomIntraMBRefresh = 0;
    pH264Arg->EnableFRMRateControl = 1;        // 0: Disable, 1: Frame level RC
    pH264Arg->Bitrate      = pNAMOutputPort->portDefinition.format.video.nBitrate;
    pH264Arg->FrameQp      = 20;
    pH264Arg->FrameQp_P    = 20;
    pH264Arg->QSCodeMax    = 30;
    pH264Arg->QSCodeMin    = 10;
    pH264Arg->CBRPeriodRf  = 100;
    pH264Arg->PadControlOn = 0;             // 0: disable, 1: enable
    pH264Arg->LumaPadVal   = 0;
    pH264Arg->CbPadVal     = 0;
    pH264Arg->CrPadVal     = 0;

    pH264Arg->ProfileIDC   = OMXAVCProfileToProfileIDC(pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].eProfile); //0;  //(OMX_VIDEO_AVCProfileMain)
    pH264Arg->LevelIDC     = OMXAVCLevelToLevelIDC(pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].eLevel);       //40; //(OMX_VIDEO_AVCLevel4)
    pH264Arg->FrameQp_B    = 20;
    pH264Arg->FrameRate    = (pNAMInputPort->portDefinition.format.video.xFramerate) >> 16;
    pH264Arg->SliceArgument = 0;          // Slice mb/byte size number
    pH264Arg->NumberBFrames = 0;            // 0 ~ 2
    pH264Arg->NumberReferenceFrames = 1;
    pH264Arg->NumberRefForPframes   = 1;
    pH264Arg->LoopFilterDisable     = 1;    // 1: Loop Filter Disable, 0: Filter Enable
    pH264Arg->LoopFilterAlphaC0Offset = 0;
    pH264Arg->LoopFilterBetaOffset    = 0;
    pH264Arg->SymbolMode       = 0;         // 0: CAVLC, 1: CABAC
    pH264Arg->PictureInterlace = 0;
    pH264Arg->Transform8x8Mode = 0;         // 0: 4x4, 1: allow 8x8
    pH264Arg->EnableMBRateControl = 0;        // 0: Disable, 1:MB level RC
    pH264Arg->DarkDisable     = 1;
    pH264Arg->SmoothDisable   = 1;
    pH264Arg->StaticDisable   = 1;
    pH264Arg->ActivityDisable = 1;

    switch ((NAM_OMX_COLOR_FORMATTYPE)pNAMInputPort->portDefinition.format.video.eColorFormat) {
    case OMX_COLOR_FormatYUV420SemiPlanar:
        pH264Arg->FrameMap = NV12_LINEAR;
        break;
    case OMX_SEC_COLOR_FormatNV12TPhysicalAddress:
    default:
        pH264Arg->FrameMap = NV12_TILE;
        break;
    }

#ifdef USE_ANDROID_EXTENSION
    if (pNAMInputPort->bStoreMetaDataInBuffer != OMX_FALSE) {
        NAM_OMX_DATA *pInputData = &pNAMComponent->processData[INPUT_PORT_INDEX];
        if(isMetadataBufferTypeGrallocSource(pInputData->dataBuffer) == OMX_TRUE)
            pH264Arg->FrameMap = NV12_LINEAR;
        else
            pH264Arg->FrameMap = NV12_TILE;
    }
#endif

/*
    NAM_OSAL_Log(NAM_LOG_TRACE, "pNAMPort->eControlRate: 0x%x", pNAMOutputPort->eControlRate);
    switch (pNAMOutputPort->eControlRate) {
    case OMX_Video_ControlRateVariable:
        NAM_OSAL_Log(NAM_LOG_TRACE, "Video Encode VBR");
        pH264Arg->EnableFRMRateControl = 0;        // 0: Disable, 1: Frame level RC
        pH264Arg->EnableMBRateControl  = 0;        // 0: Disable, 1:MB level RC
        pH264Arg->CBRPeriodRf  = 100;
        break;
    case OMX_Video_ControlRateConstant:
        NAM_OSAL_Log(NAM_LOG_TRACE, "Video Encode CBR");
        pH264Arg->EnableFRMRateControl = 1;        // 0: Disable, 1: Frame level RC
        pH264Arg->EnableMBRateControl  = 1;        // 0: Disable, 1:MB level RC
        pH264Arg->CBRPeriodRf  = 10;
        break;
    case OMX_Video_ControlRateDisable:
    default: //Android default
        NAM_OSAL_Log(NAM_LOG_TRACE, "Video Encode VBR");
        pH264Arg->EnableFRMRateControl = 0;
        pH264Arg->EnableMBRateControl  = 0;
        pH264Arg->CBRPeriodRf  = 100;
        break;
    }
*/
    H264PrintParams(*pH264Arg);
}

OMX_ERRORTYPE NAM_DMAI_H264Enc_GetParameter(
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
        NAM_H264ENC_HANDLE      *pH264Enc = NULL;

        ret = NAM_OMX_Check_SizeVersion(pDstAVCComponent, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstAVCComponent->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Enc = (NAM_H264ENC_HANDLE *)pNAMComponent->hCodecHandle;
        pSrcAVCComponent = &pH264Enc->AVCComponent[pDstAVCComponent->nPortIndex];

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

        NAM_OSAL_Strcpy((char *)pComponentRole->cRole, NAM_OMX_COMPONENT_H264_ENC_ROLE);
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
        NAM_H264ENC_HANDLE        *pH264Enc = NULL;

        ret = NAM_OMX_Check_SizeVersion(pDstProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Enc = (NAM_H264ENC_HANDLE *)pNAMComponent->hCodecHandle;
        pSrcAVCComponent = &pH264Enc->AVCComponent[pDstProfileLevel->nPortIndex];

        pDstProfileLevel->eProfile = pSrcAVCComponent->eProfile;
        pDstProfileLevel->eLevel = pSrcAVCComponent->eLevel;
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = NULL;
        NAM_H264ENC_HANDLE        *pH264Enc = NULL;

        ret = NAM_OMX_Check_SizeVersion(pDstErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstErrorCorrectionType->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Enc = (NAM_H264ENC_HANDLE *)pNAMComponent->hCodecHandle;
        pSrcErrorCorrectionType = &pH264Enc->errorCorrectionType[OUTPUT_PORT_INDEX];

        pDstErrorCorrectionType->bEnableHEC = pSrcErrorCorrectionType->bEnableHEC;
        pDstErrorCorrectionType->bEnableResync = pSrcErrorCorrectionType->bEnableResync;
        pDstErrorCorrectionType->nResynchMarkerSpacing = pSrcErrorCorrectionType->nResynchMarkerSpacing;
        pDstErrorCorrectionType->bEnableDataPartitioning = pSrcErrorCorrectionType->bEnableDataPartitioning;
        pDstErrorCorrectionType->bEnableRVLC = pSrcErrorCorrectionType->bEnableRVLC;
    }
        break;
    default:
        ret = NAM_OMX_VideoEncodeGetParameter(hComponent, nParamIndex, pComponentParameterStructure);
        break;
    }
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_DMAI_H264Enc_SetParameter(
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
        NAM_H264ENC_HANDLE      *pH264Enc = NULL;

        ret = NAM_OMX_Check_SizeVersion(pSrcAVCComponent, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcAVCComponent->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Enc = (NAM_H264ENC_HANDLE *)pNAMComponent->hCodecHandle;
        pDstAVCComponent = &pH264Enc->AVCComponent[pSrcAVCComponent->nPortIndex];

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

        if (!NAM_OSAL_Strcmp((char*)pComponentRole->cRole, NAM_OMX_COMPONENT_H264_ENC_ROLE)) {
            pNAMComponent->pNAMPort[OUTPUT_PORT_INDEX].portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
        } else {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }
    }
        break;
    case OMX_IndexParamVideoProfileLevelCurrent:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pSrcProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_AVCTYPE *pDstAVCComponent = NULL;
        NAM_H264ENC_HANDLE        *pH264Enc = NULL;

        ret = NAM_OMX_Check_SizeVersion(pSrcProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Enc = (NAM_H264ENC_HANDLE *)pNAMComponent->hCodecHandle;

        pDstAVCComponent = &pH264Enc->AVCComponent[pSrcProfileLevel->nPortIndex];
        pDstAVCComponent->eProfile = pSrcProfileLevel->eProfile;
        pDstAVCComponent->eLevel = pSrcProfileLevel->eLevel;
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = NULL;
        NAM_H264ENC_HANDLE        *pH264Enc = NULL;

        ret = NAM_OMX_Check_SizeVersion(pSrcErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcErrorCorrectionType->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Enc = (NAM_H264ENC_HANDLE *)pNAMComponent->hCodecHandle;
        pDstErrorCorrectionType = &pH264Enc->errorCorrectionType[OUTPUT_PORT_INDEX];

        pDstErrorCorrectionType->bEnableHEC = pSrcErrorCorrectionType->bEnableHEC;
        pDstErrorCorrectionType->bEnableResync = pSrcErrorCorrectionType->bEnableResync;
        pDstErrorCorrectionType->nResynchMarkerSpacing = pSrcErrorCorrectionType->nResynchMarkerSpacing;
        pDstErrorCorrectionType->bEnableDataPartitioning = pSrcErrorCorrectionType->bEnableDataPartitioning;
        pDstErrorCorrectionType->bEnableRVLC = pSrcErrorCorrectionType->bEnableRVLC;
    }
        break;
    default:
        ret = NAM_OMX_VideoEncodeSetParameter(hComponent, nIndex, pComponentParameterStructure);
        break;
    }
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_DMAI_H264Enc_GetConfig(
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

OMX_ERRORTYPE NAM_DMAI_H264Enc_SetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nIndex,
    OMX_PTR pComponentConfigStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
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
    default:
        ret = NAM_OMX_SetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_DMAI_H264Enc_GetExtensionIndex(
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

#ifdef USE_ANDROID_EXTENSION
    if (NAM_OSAL_Strcmp(cParameterName, NAM_INDEX_PARAM_STORE_METADATA_BUFFER) == 0) {
        *pIndexType = OMX_IndexParamStoreMetaDataBuffer;
        ret = OMX_ErrorNone;
    } else {
        ret = NAM_OMX_GetExtensionIndex(hComponent, cParameterName, pIndexType);
    }
#else
    ret = NAM_OMX_GetExtensionIndex(hComponent, cParameterName, pIndexType);
#endif

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_DMAI_H264Enc_ComponentRoleEnum(OMX_HANDLETYPE hComponent, OMX_U8 *cRole, OMX_U32 nIndex)
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
        NAM_OSAL_Strcpy((char *)cRole, NAM_OMX_COMPONENT_H264_ENC_ROLE);
        ret = OMX_ErrorNone;
    } else {
        ret = OMX_ErrorNoMore;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_DMAI_EncodeThread(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    NAM_OMX_BASECOMPONENT *pNAMComponent = NULL;
    NAM_H264ENC_HANDLE    *pH264Enc = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pH264Enc = (NAM_H264ENC_HANDLE *)pNAMComponent->hCodecHandle;

    while (pH264Enc->NBEncThread.bExitEncodeThread == OMX_FALSE) {
        NAM_OSAL_SemaphoreWait(pH264Enc->NBEncThread.hEncFrameStart);

        if (pH264Enc->NBEncThread.bExitEncodeThread == OMX_FALSE) {
            pH264Enc->hDMAIH264Handle.returnCodec = SsbSipDmaiEncExe(pH264Enc->hDMAIH264Handle.hDMAIHandle);
            NAM_OSAL_SemaphorePost(pH264Enc->NBEncThread.hEncFrameEnd);
        }
    }

EXIT:
    FunctionOut();
    NAM_OSAL_ThreadExit(NULL);

    return ret;
}

/* DMAI Init */
OMX_ERRORTYPE NAM_DMAI_H264Enc_Init(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE              ret = OMX_ErrorNone;
    NAM_OMX_BASECOMPONENT     *pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    NAM_OMX_BASEPORT          *pNAMInputPort = &pNAMComponent->pNAMPort[INPUT_PORT_INDEX];
    NAM_OMX_BASEPORT          *pNAMOutputPort = &pNAMComponent->pNAMPort[OUTPUT_PORT_INDEX];
    NAM_H264ENC_HANDLE        *pH264Enc = NULL;
    OMX_PTR                    hDMAIHandle = NULL;
    OMX_S32                    returnCodec = 0;

    FunctionIn();

    pH264Enc = (NAM_H264ENC_HANDLE *)pNAMComponent->hCodecHandle;
    pH264Enc->hDMAIH264Handle.bConfiguredDMAI = OMX_FALSE;
    pNAMComponent->bUseFlagEOF = OMX_FALSE;
    pNAMComponent->bSaveFlagEOS = OMX_FALSE;

    /* DMAI(Multi Function Codec) encoder and CMM(Codec Memory Management) driver open */
    SSBIP_DMAI_BUFFER_TYPE buf_type = CACHE;
    hDMAIHandle = (OMX_PTR)SsbSipDmaiEncOpen(&buf_type);
    if (hDMAIHandle == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    pH264Enc->hDMAIH264Handle.hDMAIHandle = hDMAIHandle;

    SsbSipDmaiEncSetSize(hDMAIHandle, H264_ENC,
                        pNAMOutputPort->portDefinition.format.video.nFrameWidth,
                        pNAMOutputPort->portDefinition.format.video.nFrameHeight);

    /* Allocate encoder's input buffer */
    returnCodec = SsbSipDmaiEncGetInBuf(hDMAIHandle, &(pH264Enc->hDMAIH264Handle.inputInfo));
    if (returnCodec != DMAI_RET_OK) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    pH264Enc->DMAIEncInputBuffer[0].YPhyAddr = pH264Enc->hDMAIH264Handle.inputInfo.YPhyAddr;
    pH264Enc->DMAIEncInputBuffer[0].CPhyAddr = pH264Enc->hDMAIH264Handle.inputInfo.CPhyAddr;
    pH264Enc->DMAIEncInputBuffer[0].YVirAddr = pH264Enc->hDMAIH264Handle.inputInfo.YVirAddr;
    pH264Enc->DMAIEncInputBuffer[0].CVirAddr = pH264Enc->hDMAIH264Handle.inputInfo.CVirAddr;
    pH264Enc->DMAIEncInputBuffer[0].YBufferSize = pH264Enc->hDMAIH264Handle.inputInfo.YSize;
    pH264Enc->DMAIEncInputBuffer[0].CBufferSize = pH264Enc->hDMAIH264Handle.inputInfo.CSize;
    pH264Enc->DMAIEncInputBuffer[0].YDataSize = 0;
    pH264Enc->DMAIEncInputBuffer[0].CDataSize = 0;
    NAM_OSAL_Log(NAM_LOG_TRACE, "pH264Enc->hDMAIH264Handle.inputInfo.YVirAddr : 0x%x", pH264Enc->hDMAIH264Handle.inputInfo.YVirAddr);
    NAM_OSAL_Log(NAM_LOG_TRACE, "pH264Enc->hDMAIH264Handle.inputInfo.CVirAddr : 0x%x", pH264Enc->hDMAIH264Handle.inputInfo.CVirAddr);

    returnCodec = SsbSipDmaiEncGetInBuf(hDMAIHandle, &(pH264Enc->hDMAIH264Handle.inputInfo));
    if (returnCodec != DMAI_RET_OK) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    pH264Enc->DMAIEncInputBuffer[1].YPhyAddr = pH264Enc->hDMAIH264Handle.inputInfo.YPhyAddr;
    pH264Enc->DMAIEncInputBuffer[1].CPhyAddr = pH264Enc->hDMAIH264Handle.inputInfo.CPhyAddr;
    pH264Enc->DMAIEncInputBuffer[1].YVirAddr = pH264Enc->hDMAIH264Handle.inputInfo.YVirAddr;
    pH264Enc->DMAIEncInputBuffer[1].CVirAddr = pH264Enc->hDMAIH264Handle.inputInfo.CVirAddr;
    pH264Enc->DMAIEncInputBuffer[1].YBufferSize = pH264Enc->hDMAIH264Handle.inputInfo.YSize;
    pH264Enc->DMAIEncInputBuffer[1].CBufferSize = pH264Enc->hDMAIH264Handle.inputInfo.CSize;
    pH264Enc->DMAIEncInputBuffer[1].YDataSize = 0;
    pH264Enc->DMAIEncInputBuffer[1].CDataSize = 0;
    NAM_OSAL_Log(NAM_LOG_TRACE, "pH264Enc->hDMAIH264Handle.inputInfo.YVirAddr : 0x%x", pH264Enc->hDMAIH264Handle.inputInfo.YVirAddr);
    NAM_OSAL_Log(NAM_LOG_TRACE, "pH264Enc->hDMAIH264Handle.inputInfo.CVirAddr : 0x%x", pH264Enc->hDMAIH264Handle.inputInfo.CVirAddr);

    pNAMComponent->processData[INPUT_PORT_INDEX].specificBufferHeader.YPhyAddr = pH264Enc->DMAIEncInputBuffer[0].YPhyAddr;
    pNAMComponent->processData[INPUT_PORT_INDEX].specificBufferHeader.CPhyAddr = pH264Enc->DMAIEncInputBuffer[0].CPhyAddr;
    pNAMComponent->processData[INPUT_PORT_INDEX].specificBufferHeader.YVirAddr = pH264Enc->DMAIEncInputBuffer[0].YVirAddr;
    pNAMComponent->processData[INPUT_PORT_INDEX].specificBufferHeader.CVirAddr = pH264Enc->DMAIEncInputBuffer[0].CVirAddr;
    pNAMComponent->processData[INPUT_PORT_INDEX].specificBufferHeader.YSize = pH264Enc->DMAIEncInputBuffer[0].YBufferSize;
    pNAMComponent->processData[INPUT_PORT_INDEX].specificBufferHeader.CSize = pH264Enc->DMAIEncInputBuffer[0].CBufferSize;

    pH264Enc->indexInputBuffer = 0;
    pH264Enc->bFirstFrame = OMX_TRUE;

    pH264Enc->NBEncThread.bExitEncodeThread = OMX_FALSE;
    pH264Enc->NBEncThread.bEncoderRun = OMX_FALSE;
    NAM_OSAL_SemaphoreCreate(&(pH264Enc->NBEncThread.hEncFrameStart));
    NAM_OSAL_SemaphoreCreate(&(pH264Enc->NBEncThread.hEncFrameEnd));
    if (OMX_ErrorNone == NAM_OSAL_ThreadCreate(&pH264Enc->NBEncThread.hNBEncodeThread,
                                                NAM_DMAI_EncodeThread,
                                                pOMXComponent)) {
        pH264Enc->hDMAIH264Handle.returnCodec = DMAI_RET_OK;
    }

    NAM_OSAL_Memset(pNAMComponent->timeStamp, -19771003, sizeof(OMX_TICKS) * MAX_TIMESTAMP);
    NAM_OSAL_Memset(pNAMComponent->nFlags, 0, sizeof(OMX_U32) * MAX_FLAGS);
    pH264Enc->hDMAIH264Handle.indexTimestamp = 0;

EXIT:
    FunctionOut();

    return ret;
}

/* DMAI Terminate */
OMX_ERRORTYPE NAM_DMAI_H264Enc_Terminate(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    NAM_OMX_BASECOMPONENT *pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    NAM_H264ENC_HANDLE    *pH264Enc = NULL;
    OMX_PTR                hDMAIHandle = NULL;

    FunctionIn();

    pH264Enc = (NAM_H264ENC_HANDLE *)pNAMComponent->hCodecHandle;

    if (pH264Enc->NBEncThread.hNBEncodeThread != NULL) {
        pH264Enc->NBEncThread.bExitEncodeThread = OMX_TRUE;
        NAM_OSAL_SemaphorePost(pH264Enc->NBEncThread.hEncFrameStart);
        NAM_OSAL_ThreadTerminate(pH264Enc->NBEncThread.hNBEncodeThread);
        pH264Enc->NBEncThread.hNBEncodeThread = NULL;
    }

    if(pH264Enc->NBEncThread.hEncFrameEnd != NULL) {
        NAM_OSAL_SemaphoreTerminate(pH264Enc->NBEncThread.hEncFrameEnd);
        pH264Enc->NBEncThread.hEncFrameEnd = NULL;
    }

    if(pH264Enc->NBEncThread.hEncFrameStart != NULL) {
        NAM_OSAL_SemaphoreTerminate(pH264Enc->NBEncThread.hEncFrameStart);
        pH264Enc->NBEncThread.hEncFrameStart = NULL;
    }

    hDMAIHandle = pH264Enc->hDMAIH264Handle.hDMAIHandle;
    if (hDMAIHandle != NULL) {
        SsbSipDmaiEncClose(hDMAIHandle);
        hDMAIHandle = pH264Enc->hDMAIH264Handle.hDMAIHandle = NULL;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_DMAI_H264_Encode(OMX_COMPONENTTYPE *pOMXComponent, NAM_OMX_DATA *pInputData, NAM_OMX_DATA *pOutputData)
{
    OMX_ERRORTYPE              ret = OMX_ErrorNone;
    NAM_OMX_BASECOMPONENT     *pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    NAM_H264ENC_HANDLE        *pH264Enc = (NAM_H264ENC_HANDLE *)pNAMComponent->hCodecHandle;
    SSBSIP_DMAI_ENC_INPUT_INFO *pInputInfo = &pH264Enc->hDMAIH264Handle.inputInfo;
    SSBSIP_DMAI_ENC_OUTPUT_INFO outputInfo;
    NAM_OMX_BASEPORT          *pNAMPort = NULL;
    DMAI_ENC_ADDR_INFO          addrInfo;
    OMX_U32                    oneFrameSize = pInputData->dataLen;

    FunctionIn();

    if (pH264Enc->hDMAIH264Handle.bConfiguredDMAI == OMX_FALSE) {
        Set_H264ENC_Param(&(pH264Enc->hDMAIH264Handle.dmaiVideoAvc), pNAMComponent);
        pH264Enc->hDMAIH264Handle.returnCodec = SsbSipDmaiEncInit(pH264Enc->hDMAIH264Handle.hDMAIHandle, &(pH264Enc->hDMAIH264Handle.dmaiVideoAvc));
        if (pH264Enc->hDMAIH264Handle.returnCodec != DMAI_RET_OK) {
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }

        pH264Enc->hDMAIH264Handle.returnCodec = SsbSipDmaiEncGetOutBuf(pH264Enc->hDMAIH264Handle.hDMAIHandle, &outputInfo);
        if (pH264Enc->hDMAIH264Handle.returnCodec != DMAI_RET_OK)
        {
            NAM_OSAL_Log(NAM_LOG_TRACE, "%s - SsbSipDmaiEncGetOutBuf Failed\n", __func__);
            ret = OMX_ErrorUndefined;
            goto EXIT;
        } else {
            char *p = NULL;
            int iSpsSize = 0;
            int iPpsSize = 0;

            p = FindDelimiter((OMX_U8 *)outputInfo.StrmVirAddr + 4, outputInfo.headerSize - 4);

            iSpsSize = (unsigned int)p - (unsigned int)outputInfo.StrmVirAddr;
            pH264Enc->hDMAIH264Handle.headerData.pHeaderSPS = (OMX_PTR)outputInfo.StrmVirAddr;
            pH264Enc->hDMAIH264Handle.headerData.SPSLen = iSpsSize;

            iPpsSize = outputInfo.headerSize - iSpsSize;
            pH264Enc->hDMAIH264Handle.headerData.pHeaderPPS = outputInfo.StrmVirAddr + iSpsSize;
            pH264Enc->hDMAIH264Handle.headerData.PPSLen = iPpsSize;
        }

        /* NAM_OSAL_Memcpy((void*)(pOutputData->dataBuffer), (const void*)(outputInfo.StrmVirAddr), outputInfo.headerSize); */
        pOutputData->dataBuffer = outputInfo.StrmVirAddr;
        pOutputData->allocSize = outputInfo.headerSize;
        pOutputData->dataLen = outputInfo.headerSize;
        pOutputData->timeStamp = 0;
        pOutputData->nFlags |= OMX_BUFFERFLAG_CODECCONFIG;
        pOutputData->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;

        pH264Enc->hDMAIH264Handle.bConfiguredDMAI = OMX_TRUE;

        ret = OMX_ErrorInputDataEncodeYet;
        goto EXIT;
    }

    if ((pInputData->nFlags & OMX_BUFFERFLAG_ENDOFFRAME) &&
        (pNAMComponent->bUseFlagEOF == OMX_FALSE)) {
        pNAMComponent->bUseFlagEOF = OMX_TRUE;
    }

    if (oneFrameSize <= 0) {
        pOutputData->timeStamp = pInputData->timeStamp;
        pOutputData->nFlags = pInputData->nFlags;

        ret = OMX_ErrorNone;
        goto EXIT;
    }

    pNAMPort = &pNAMComponent->pNAMPort[INPUT_PORT_INDEX];
    if (((pInputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) ||
        (pNAMComponent->getAllDelayBuffer == OMX_TRUE)){
        /* Dummy input data for get out encoded last frame */
        pInputInfo->YPhyAddr = pH264Enc->DMAIEncInputBuffer[pH264Enc->indexInputBuffer].YPhyAddr;
        pInputInfo->CPhyAddr = pH264Enc->DMAIEncInputBuffer[pH264Enc->indexInputBuffer].CPhyAddr;
        pInputInfo->YVirAddr = pH264Enc->DMAIEncInputBuffer[pH264Enc->indexInputBuffer].YVirAddr;
        pInputInfo->CVirAddr = pH264Enc->DMAIEncInputBuffer[pH264Enc->indexInputBuffer].CVirAddr;
    } else if (pNAMPort->portDefinition.format.video.eColorFormat == OMX_SEC_COLOR_FormatNV12TPhysicalAddress) {
        NAM_OSAL_Memcpy(&addrInfo.pAddrY, pInputData->dataBuffer, sizeof(addrInfo.pAddrY));
        NAM_OSAL_Memcpy(&addrInfo.pAddrC, pInputData->dataBuffer + sizeof(addrInfo.pAddrY), sizeof(addrInfo.pAddrC));
        pInputInfo->YPhyAddr = addrInfo.pAddrY;
        pInputInfo->CPhyAddr = addrInfo.pAddrC;
#ifdef USE_ANDROID_EXTENSION
    } else if (pNAMPort->bStoreMetaDataInBuffer != OMX_FALSE) {
        ret = preprocessMetaDataInBuffers(pOMXComponent, pInputData->dataBuffer, pInputInfo);
        if (ret != OMX_ErrorNone)
            goto EXIT;
#endif
    } else {
        /* Real input data */
        pInputInfo->YPhyAddr = pNAMComponent->processData[INPUT_PORT_INDEX].specificBufferHeader.YPhyAddr;
        pInputInfo->CPhyAddr = pNAMComponent->processData[INPUT_PORT_INDEX].specificBufferHeader.CPhyAddr;
    }

    pNAMComponent->timeStamp[pH264Enc->hDMAIH264Handle.indexTimestamp] = pInputData->timeStamp;
    pNAMComponent->nFlags[pH264Enc->hDMAIH264Handle.indexTimestamp] = pInputData->nFlags;

    if ((pH264Enc->hDMAIH264Handle.returnCodec == DMAI_RET_OK) &&
        (pH264Enc->bFirstFrame == OMX_FALSE)) {
        OMX_S32 indexTimestamp = 0;

        /* wait for dmai encode done */
        if (pH264Enc->NBEncThread.bEncoderRun != OMX_FALSE) {
            NAM_OSAL_SemaphoreWait(pH264Enc->NBEncThread.hEncFrameEnd);
            pH264Enc->NBEncThread.bEncoderRun = OMX_FALSE;
        }

        pH264Enc->hDMAIH264Handle.returnCodec = SsbSipDmaiEncGetOutBuf(pH264Enc->hDMAIH264Handle.hDMAIHandle, &outputInfo);
        if ((SsbSipDmaiEncGetConfig(pH264Enc->hDMAIH264Handle.hDMAIHandle, DMAI_ENC_GETCONF_FRAME_TAG, &indexTimestamp) != DMAI_RET_OK) ||
            (((indexTimestamp < 0) || (indexTimestamp >= MAX_TIMESTAMP)))){
            pOutputData->timeStamp = pInputData->timeStamp;
            pOutputData->nFlags = pInputData->nFlags;
        } else {
            pOutputData->timeStamp = pNAMComponent->timeStamp[indexTimestamp];
            pOutputData->nFlags = pNAMComponent->nFlags[indexTimestamp];
        }

        if (pH264Enc->hDMAIH264Handle.returnCodec == DMAI_RET_OK) {
            /** Fill Output Buffer **/
            pOutputData->dataBuffer = outputInfo.StrmVirAddr;
            pOutputData->allocSize = outputInfo.dataSize;
            pOutputData->dataLen = outputInfo.dataSize;
            pOutputData->usedDataLen = 0;

            pOutputData->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
            if (outputInfo.frameType == DMAI_FRAME_TYPE_I_FRAME)
                    pOutputData->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;

            NAM_OSAL_Log(NAM_LOG_TRACE, "DMAI Encode OK!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

            ret = OMX_ErrorNone;
        } else {
            NAM_OSAL_Log(NAM_LOG_ERROR, "%s: SsbSipDmaiEncGetOutBuf failed, ret:%d", __FUNCTION__, pH264Enc->hDMAIH264Handle.returnCodec);
            ret = OMX_ErrorUndefined;
            goto EXIT;
        }

        if (pNAMComponent->getAllDelayBuffer == OMX_TRUE) {
            ret = OMX_ErrorInputDataEncodeYet;
        }
        if ((pInputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) {
            pInputData->nFlags = (pOutputData->nFlags & (~OMX_BUFFERFLAG_EOS));
            pNAMComponent->getAllDelayBuffer = OMX_TRUE;
            ret = OMX_ErrorInputDataEncodeYet;
        }
        if ((pOutputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) {
            pNAMComponent->getAllDelayBuffer = OMX_FALSE;
            pOutputData->dataLen = 0;
            pOutputData->usedDataLen = 0;
            NAM_OSAL_Log(NAM_LOG_TRACE, "OMX_BUFFERFLAG_EOS!!!");
            ret = OMX_ErrorNone;
        }
    }
    if (pH264Enc->hDMAIH264Handle.returnCodec != DMAI_RET_OK) {
        NAM_OSAL_Log(NAM_LOG_ERROR, "In %s : SsbSipDmaiEncExe Failed!!!\n", __func__);
        ret = OMX_ErrorUndefined;
    }

    pH264Enc->hDMAIH264Handle.returnCodec = SsbSipDmaiEncSetInBuf(pH264Enc->hDMAIH264Handle.hDMAIHandle, pInputInfo);
    if (pH264Enc->hDMAIH264Handle.returnCodec != DMAI_RET_OK) {
        NAM_OSAL_Log(NAM_LOG_TRACE, "Error : SsbSipDmaiEncSetInBuf() \n");
        ret = OMX_ErrorUndefined;
        goto EXIT;
    } else {
        pH264Enc->indexInputBuffer++;
        pH264Enc->indexInputBuffer %= DMAI_INPUT_BUFFER_NUM_MAX;
        pNAMComponent->processData[INPUT_PORT_INDEX].specificBufferHeader.YPhyAddr = pH264Enc->DMAIEncInputBuffer[pH264Enc->indexInputBuffer].YPhyAddr;
        pNAMComponent->processData[INPUT_PORT_INDEX].specificBufferHeader.CPhyAddr = pH264Enc->DMAIEncInputBuffer[pH264Enc->indexInputBuffer].CPhyAddr;
        pNAMComponent->processData[INPUT_PORT_INDEX].specificBufferHeader.YVirAddr = pH264Enc->DMAIEncInputBuffer[pH264Enc->indexInputBuffer].YVirAddr;
        pNAMComponent->processData[INPUT_PORT_INDEX].specificBufferHeader.CVirAddr = pH264Enc->DMAIEncInputBuffer[pH264Enc->indexInputBuffer].CVirAddr;
        pNAMComponent->processData[INPUT_PORT_INDEX].specificBufferHeader.YSize = pH264Enc->DMAIEncInputBuffer[pH264Enc->indexInputBuffer].YBufferSize;
        pNAMComponent->processData[INPUT_PORT_INDEX].specificBufferHeader.CSize = pH264Enc->DMAIEncInputBuffer[pH264Enc->indexInputBuffer].CBufferSize;
    }

    SsbSipDmaiEncSetConfig(pH264Enc->hDMAIH264Handle.hDMAIHandle, DMAI_ENC_SETCONF_FRAME_TAG, &(pH264Enc->hDMAIH264Handle.indexTimestamp));

    /* dmai encode start */
    NAM_OSAL_SemaphorePost(pH264Enc->NBEncThread.hEncFrameStart);
    pH264Enc->NBEncThread.bEncoderRun = OMX_TRUE;
    pH264Enc->hDMAIH264Handle.indexTimestamp++;
    pH264Enc->hDMAIH264Handle.indexTimestamp %= MAX_TIMESTAMP;
    pH264Enc->bFirstFrame = OMX_FALSE;

EXIT:
    FunctionOut();

    return ret;
}

/* DMAI Encode */
OMX_ERRORTYPE NAM_DMAI_H264Enc_bufferProcess(OMX_COMPONENTTYPE *pOMXComponent, NAM_OMX_DATA *pInputData, NAM_OMX_DATA *pOutputData)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    NAM_OMX_BASECOMPONENT   *pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    NAM_H264ENC_HANDLE      *pH264Enc = (NAM_H264ENC_HANDLE *)pNAMComponent->hCodecHandle;
    NAM_OMX_BASEPORT        *pNAMInputPort = &pNAMComponent->pNAMPort[INPUT_PORT_INDEX];
    NAM_OMX_BASEPORT        *pNAMOutputPort = &pNAMComponent->pNAMPort[OUTPUT_PORT_INDEX];
    OMX_BOOL                 endOfFrame = OMX_FALSE;
    OMX_BOOL                 flagEOS = OMX_FALSE;

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

    ret = NAM_DMAI_H264_Encode(pOMXComponent, pInputData, pOutputData);
    if (ret != OMX_ErrorNone) {
        if (ret == OMX_ErrorInputDataEncodeYet) {
            pOutputData->usedDataLen = 0;
            pOutputData->remainDataLen = pOutputData->dataLen;
        } else {
            pNAMComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                            pNAMComponent->callbackData,
                                            OMX_EventError, ret, 0, NULL);
        }
    } else {
        pInputData->usedDataLen += pInputData->dataLen;
        pInputData->remainDataLen = pInputData->dataLen - pInputData->usedDataLen;
        pInputData->dataLen -= pInputData->usedDataLen;
        pInputData->usedDataLen = 0;

        /* pOutputData->usedDataLen = 0; */
        pOutputData->remainDataLen = pOutputData->dataLen - pOutputData->usedDataLen;
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
    NAM_H264ENC_HANDLE      *pH264Enc = NULL;
    int i = 0;

    FunctionIn();

    if ((hComponent == NULL) || (componentName == NULL)) {
        ret = OMX_ErrorBadParameter;
        NAM_OSAL_Log(NAM_LOG_ERROR, "OMX_ErrorBadParameter, Line:%d", __LINE__);
        goto EXIT;
    }
    if (NAM_OSAL_Strcmp(NAM_OMX_COMPONENT_H264_ENC, componentName) != 0) {
        ret = OMX_ErrorBadParameter;
        NAM_OSAL_Log(NAM_LOG_ERROR, "OMX_ErrorBadParameter, componentName:%s, Line:%d", componentName, __LINE__);
        goto EXIT;
    }

    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = NAM_OMX_VideoEncodeComponentInit(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        NAM_OSAL_Log(NAM_LOG_ERROR, "OMX_Error, Line:%d", __LINE__);
        goto EXIT;
    }
    pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pNAMComponent->codecType = HW_VIDEO_CODEC;

    pNAMComponent->componentName = (OMX_STRING)NAM_OSAL_Malloc(MAX_OMX_COMPONENT_NAME_SIZE);
    if (pNAMComponent->componentName == NULL) {
        NAM_OMX_VideoEncodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        NAM_OSAL_Log(NAM_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    NAM_OSAL_Memset(pNAMComponent->componentName, 0, MAX_OMX_COMPONENT_NAME_SIZE);

    pH264Enc = NAM_OSAL_Malloc(sizeof(NAM_H264ENC_HANDLE));
    if (pH264Enc == NULL) {
        NAM_OMX_VideoEncodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        NAM_OSAL_Log(NAM_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    NAM_OSAL_Memset(pH264Enc, 0, sizeof(NAM_H264ENC_HANDLE));
    pNAMComponent->hCodecHandle = (OMX_HANDLETYPE)pH264Enc;

    NAM_OSAL_Strcpy(pNAMComponent->componentName, NAM_OMX_COMPONENT_H264_ENC);
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
    pNAMPort->portDefinition.nBufferSize = DEFAULT_VIDEO_INPUT_BUFFER_SIZE;
    pNAMPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    NAM_OSAL_Memset(pNAMPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
    NAM_OSAL_Strcpy(pNAMPort->portDefinition.format.video.cMIMEType, "raw/video");
    pNAMPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
    pNAMPort->portDefinition.bEnabled = OMX_TRUE;

    /* Output port */
    pNAMPort = &pNAMComponent->pNAMPort[OUTPUT_PORT_INDEX];
    pNAMPort->portDefinition.format.video.nFrameWidth = DEFAULT_FRAME_WIDTH;
    pNAMPort->portDefinition.format.video.nFrameHeight= DEFAULT_FRAME_HEIGHT;
    pNAMPort->portDefinition.format.video.nStride = 0; /*DEFAULT_FRAME_WIDTH;*/
    pNAMPort->portDefinition.nBufferSize = DEFAULT_VIDEO_OUTPUT_BUFFER_SIZE;
    pNAMPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
    NAM_OSAL_Memset(pNAMPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
    NAM_OSAL_Strcpy(pNAMPort->portDefinition.format.video.cMIMEType, "video/avc");
    pNAMPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    pNAMPort->portDefinition.bEnabled = OMX_TRUE;

    for(i = 0; i < ALL_PORT_NUM; i++) {
        INIT_SET_SIZE_VERSION(&pH264Enc->AVCComponent[i], OMX_VIDEO_PARAM_AVCTYPE);
        pH264Enc->AVCComponent[i].nPortIndex = i;
        pH264Enc->AVCComponent[i].eProfile   = OMX_VIDEO_AVCProfileBaseline;
        pH264Enc->AVCComponent[i].eLevel     = OMX_VIDEO_AVCLevel31;
    }

    pOMXComponent->GetParameter      = &NAM_DMAI_H264Enc_GetParameter;
    pOMXComponent->SetParameter      = &NAM_DMAI_H264Enc_SetParameter;
    pOMXComponent->GetConfig         = &NAM_DMAI_H264Enc_GetConfig;
    pOMXComponent->SetConfig         = &NAM_DMAI_H264Enc_SetConfig;
    pOMXComponent->GetExtensionIndex = &NAM_DMAI_H264Enc_GetExtensionIndex;
    pOMXComponent->ComponentRoleEnum = &NAM_DMAI_H264Enc_ComponentRoleEnum;
    pOMXComponent->ComponentDeInit   = &NAM_OMX_ComponentDeinit;

    pNAMComponent->nam_dmai_componentInit      = &NAM_DMAI_H264Enc_Init;
    pNAMComponent->nam_dmai_componentTerminate = &NAM_DMAI_H264Enc_Terminate;
    pNAMComponent->nam_dmai_bufferProcess      = &NAM_DMAI_H264Enc_bufferProcess;
    pNAMComponent->nam_checkInputFrame        = NULL;

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
    NAM_H264ENC_HANDLE      *pH264Enc = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    NAM_OSAL_Free(pNAMComponent->componentName);
    pNAMComponent->componentName = NULL;

    pH264Enc = (NAM_H264ENC_HANDLE *)pNAMComponent->hCodecHandle;
    if (pH264Enc != NULL) {
        NAM_OSAL_Free(pH264Enc);
        pH264Enc = pNAMComponent->hCodecHandle = NULL;
    }

    ret = NAM_OMX_VideoEncodeComponentDeinit(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

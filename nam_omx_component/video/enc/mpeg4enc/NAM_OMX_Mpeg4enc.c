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
 * @file        NAM_OMX_Mpeg4enc.c
 * @brief
 * @author      Yunji Kim (yunji.kim@samsung.com)
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
#include "NAM_OMX_Mpeg4enc.h"
#include "SsbSipDmaiApi.h"
#include "color_space_convertor.h"

#undef  NAM_LOG_TAG
#define NAM_LOG_TAG    "NAM_MPEG4_ENC"
#define NAM_LOG_OFF
#include "NAM_OSAL_Log.h"


/* MPEG4 Encoder Supported Levels & profiles */
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

/* H.263 Encoder Supported Levels & profiles */
NAM_OMX_VIDEO_PROFILELEVEL supportedH263ProfileLevels[] = {
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level10},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level20},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level30},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level40},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level45},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level50},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level60},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level70}};

OMX_U32 OMXMpeg4ProfileToDMAIProfile(OMX_VIDEO_MPEG4PROFILETYPE profile)
{
    OMX_U32 ret;

    switch (profile) {
    case OMX_VIDEO_MPEG4ProfileSimple:
        ret = 0;
        break;
    case OMX_VIDEO_MPEG4ProfileAdvancedSimple:
        ret = 1;
        break;
    default:
        ret = 0;
    };

    return ret;
}
OMX_U32 OMXMpeg4LevelToDMAILevel(OMX_VIDEO_MPEG4LEVELTYPE level)
{
    OMX_U32 ret;

    switch (level) {
    case OMX_VIDEO_MPEG4Level0:
        ret = 0;
        break;
    case OMX_VIDEO_MPEG4Level0b:
        ret = 9;
        break;
    case OMX_VIDEO_MPEG4Level1:
        ret = 1;
        break;
    case OMX_VIDEO_MPEG4Level2:
        ret = 2;
        break;
    case OMX_VIDEO_MPEG4Level3:
        ret = 3;
        break;
    case OMX_VIDEO_MPEG4Level4:
    case OMX_VIDEO_MPEG4Level4a:
        ret = 4;
        break;
    case OMX_VIDEO_MPEG4Level5:
        ret = 5;
        break;
    default:
        ret = 0;
    };

    return ret;
}

void Mpeg4PrintParams(SSBSIP_DMAI_ENC_MPEG4_PARAM mpeg4Param)
{
    NAM_OSAL_Log(NAM_LOG_TRACE, "SourceWidth             : %d\n", mpeg4Param.SourceWidth);
    NAM_OSAL_Log(NAM_LOG_TRACE, "SourceHeight            : %d\n", mpeg4Param.SourceHeight);
    NAM_OSAL_Log(NAM_LOG_TRACE, "IDRPeriod               : %d\n", mpeg4Param.IDRPeriod);
    NAM_OSAL_Log(NAM_LOG_TRACE, "SliceMode               : %d\n", mpeg4Param.SliceMode);
    NAM_OSAL_Log(NAM_LOG_TRACE, "RandomIntraMBRefresh    : %d\n", mpeg4Param.RandomIntraMBRefresh);
    NAM_OSAL_Log(NAM_LOG_TRACE, "EnableFRMRateControl    : %d\n", mpeg4Param.EnableFRMRateControl);
    NAM_OSAL_Log(NAM_LOG_TRACE, "Bitrate                 : %d\n", mpeg4Param.Bitrate);
    NAM_OSAL_Log(NAM_LOG_TRACE, "FrameQp                 : %d\n", mpeg4Param.FrameQp);
    NAM_OSAL_Log(NAM_LOG_TRACE, "FrameQp_P               : %d\n", mpeg4Param.FrameQp_P);
    NAM_OSAL_Log(NAM_LOG_TRACE, "QSCodeMax               : %d\n", mpeg4Param.QSCodeMax);
    NAM_OSAL_Log(NAM_LOG_TRACE, "QSCodeMin               : %d\n", mpeg4Param.QSCodeMin);
    NAM_OSAL_Log(NAM_LOG_TRACE, "CBRPeriodRf             : %d\n", mpeg4Param.CBRPeriodRf);
    NAM_OSAL_Log(NAM_LOG_TRACE, "PadControlOn            : %d\n", mpeg4Param.PadControlOn);
    NAM_OSAL_Log(NAM_LOG_TRACE, "LumaPadVal              : %d\n", mpeg4Param.LumaPadVal);
    NAM_OSAL_Log(NAM_LOG_TRACE, "CbPadVal                : %d\n", mpeg4Param.CbPadVal);
    NAM_OSAL_Log(NAM_LOG_TRACE, "CrPadVal                : %d\n", mpeg4Param.CrPadVal);

    /* MPEG4 specific parameters */
    NAM_OSAL_Log(NAM_LOG_TRACE, "ProfileIDC              : %d\n", mpeg4Param.ProfileIDC);
    NAM_OSAL_Log(NAM_LOG_TRACE, "LevelIDC                : %d\n", mpeg4Param.LevelIDC);
    NAM_OSAL_Log(NAM_LOG_TRACE, "FrameQp_B               : %d\n", mpeg4Param.FrameQp_B);
    NAM_OSAL_Log(NAM_LOG_TRACE, "TimeIncreamentRes       : %d\n", mpeg4Param.TimeIncreamentRes);
    NAM_OSAL_Log(NAM_LOG_TRACE, "VopTimeIncreament       : %d\n", mpeg4Param.VopTimeIncreament);
    NAM_OSAL_Log(NAM_LOG_TRACE, "SliceArgument           : %d\n", mpeg4Param.SliceArgument);
    NAM_OSAL_Log(NAM_LOG_TRACE, "NumberBFrames           : %d\n", mpeg4Param.NumberBFrames);
    NAM_OSAL_Log(NAM_LOG_TRACE, "DisableQpelME           : %d\n", mpeg4Param.DisableQpelME);
}

void H263PrintParams(SSBSIP_DMAI_ENC_H263_PARAM h263Param)
{
    NAM_OSAL_Log(NAM_LOG_TRACE, "SourceWidth             : %d\n", h263Param.SourceWidth);
    NAM_OSAL_Log(NAM_LOG_TRACE, "SourceHeight            : %d\n", h263Param.SourceHeight);
    NAM_OSAL_Log(NAM_LOG_TRACE, "IDRPeriod               : %d\n", h263Param.IDRPeriod);
    NAM_OSAL_Log(NAM_LOG_TRACE, "SliceMode               : %d\n", h263Param.SliceMode);
    NAM_OSAL_Log(NAM_LOG_TRACE, "RandomIntraMBRefresh    : %d\n", h263Param.RandomIntraMBRefresh);
    NAM_OSAL_Log(NAM_LOG_TRACE, "EnableFRMRateControl    : %d\n", h263Param.EnableFRMRateControl);
    NAM_OSAL_Log(NAM_LOG_TRACE, "Bitrate                 : %d\n", h263Param.Bitrate);
    NAM_OSAL_Log(NAM_LOG_TRACE, "FrameQp                 : %d\n", h263Param.FrameQp);
    NAM_OSAL_Log(NAM_LOG_TRACE, "FrameQp_P               : %d\n", h263Param.FrameQp_P);
    NAM_OSAL_Log(NAM_LOG_TRACE, "QSCodeMax               : %d\n", h263Param.QSCodeMax);
    NAM_OSAL_Log(NAM_LOG_TRACE, "QSCodeMin               : %d\n", h263Param.QSCodeMin);
    NAM_OSAL_Log(NAM_LOG_TRACE, "CBRPeriodRf             : %d\n", h263Param.CBRPeriodRf);
    NAM_OSAL_Log(NAM_LOG_TRACE, "PadControlOn            : %d\n", h263Param.PadControlOn);
    NAM_OSAL_Log(NAM_LOG_TRACE, "LumaPadVal              : %d\n", h263Param.LumaPadVal);
    NAM_OSAL_Log(NAM_LOG_TRACE, "CbPadVal                : %d\n", h263Param.CbPadVal);
    NAM_OSAL_Log(NAM_LOG_TRACE, "CrPadVal                : %d\n", h263Param.CrPadVal);

    /* H.263 specific parameters */
    NAM_OSAL_Log(NAM_LOG_TRACE, "FrameRate               : %d\n", h263Param.FrameRate);
}

void Set_Mpeg4Enc_Param(SSBSIP_DMAI_ENC_MPEG4_PARAM *pMpeg4Param, NAM_OMX_BASECOMPONENT *pNAMComponent)
{
    NAM_OMX_BASEPORT    *pNAMInputPort = NULL;
    NAM_OMX_BASEPORT    *pNAMOutputPort = NULL;
    NAM_MPEG4ENC_HANDLE *pMpeg4Enc = NULL;

    pMpeg4Enc = (NAM_MPEG4ENC_HANDLE *)pNAMComponent->hCodecHandle;
    pNAMInputPort = &pNAMComponent->pNAMPort[INPUT_PORT_INDEX];
    pNAMOutputPort = &pNAMComponent->pNAMPort[OUTPUT_PORT_INDEX];

    pMpeg4Param->codecType            = MPEG4_ENC;
    pMpeg4Param->SourceWidth          = pNAMOutputPort->portDefinition.format.video.nFrameWidth;
    pMpeg4Param->SourceHeight         = pNAMOutputPort->portDefinition.format.video.nFrameHeight;
    pMpeg4Param->IDRPeriod            = pMpeg4Enc->mpeg4Component[OUTPUT_PORT_INDEX].nPFrames + 1;
    pMpeg4Param->SliceMode            = 0;
    pMpeg4Param->RandomIntraMBRefresh = 0;
    pMpeg4Param->EnableFRMRateControl = 1;    /* 0: disable, 1: enable */
    pMpeg4Param->Bitrate              = pNAMOutputPort->portDefinition.format.video.nBitrate;
    pMpeg4Param->FrameQp              = 20;
    pMpeg4Param->FrameQp_P            = 20;
    pMpeg4Param->QSCodeMax            = 30;
    pMpeg4Param->QSCodeMin            = 10;
    pMpeg4Param->CBRPeriodRf          = 10;
    pMpeg4Param->PadControlOn         = 0;    /* 0: Use boundary pixel, 1: Use the below setting value */
    pMpeg4Param->LumaPadVal           = 0;
    pMpeg4Param->CbPadVal             = 0;
    pMpeg4Param->CrPadVal             = 0;

    pMpeg4Param->ProfileIDC           = OMXMpeg4ProfileToDMAIProfile(pMpeg4Enc->mpeg4Component[OUTPUT_PORT_INDEX].eProfile);
    pMpeg4Param->LevelIDC             = OMXMpeg4LevelToDMAILevel(pMpeg4Enc->mpeg4Component[OUTPUT_PORT_INDEX].eLevel);
    pMpeg4Param->FrameQp_B            = 20;
    pMpeg4Param->TimeIncreamentRes    = (pNAMInputPort->portDefinition.format.video.xFramerate) >> 16;
    pMpeg4Param->VopTimeIncreament    = 1;
    pMpeg4Param->SliceArgument        = 0;    /* MB number or byte number */
    pMpeg4Param->NumberBFrames        = 0;    /* 0(not used) ~ 2 */
    pMpeg4Param->DisableQpelME        = 1;

    switch ((NAM_OMX_COLOR_FORMATTYPE)pNAMInputPort->portDefinition.format.video.eColorFormat) {
    case OMX_COLOR_FormatYUV420SemiPlanar:
        pMpeg4Param->FrameMap = NV12_LINEAR;
        break;
    case OMX_NAM_COLOR_FormatNV12TPhysicalAddress:
    default:
        pMpeg4Param->FrameMap = NV12_TILE;
        break;
    }

#ifdef USE_ANDROID_EXTENSION
    if (pNAMInputPort->bStoreMetaDataInBuffer != OMX_FALSE) {
        NAM_OMX_DATA *pInputData = &pNAMComponent->processData[INPUT_PORT_INDEX];
        if(isMetadataBufferTypeGrallocSource(pInputData->dataBuffer) == OMX_TRUE)
            pMpeg4Param->FrameMap = NV12_LINEAR;
        else
            pMpeg4Param->FrameMap = NV12_TILE;
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
    Mpeg4PrintParams(*pMpeg4Param);
}

void Set_H263Enc_Param(SSBSIP_DMAI_ENC_H263_PARAM *pH263Param, NAM_OMX_BASECOMPONENT *pNAMComponent)
{
    NAM_OMX_BASEPORT    *pNAMInputPort = NULL;
    NAM_OMX_BASEPORT    *pNAMOutputPort = NULL;
    NAM_MPEG4ENC_HANDLE *pMpeg4Enc = NULL;

    pMpeg4Enc = (NAM_MPEG4ENC_HANDLE *)pNAMComponent->hCodecHandle;
    pNAMInputPort = &pNAMComponent->pNAMPort[INPUT_PORT_INDEX];
    pNAMOutputPort = &pNAMComponent->pNAMPort[OUTPUT_PORT_INDEX];

    pH263Param->codecType            = H263_ENC;
    pH263Param->SourceWidth          = pNAMOutputPort->portDefinition.format.video.nFrameWidth;
    pH263Param->SourceHeight         = pNAMOutputPort->portDefinition.format.video.nFrameHeight;
    pH263Param->IDRPeriod            = pMpeg4Enc->h263Component[OUTPUT_PORT_INDEX].nPFrames + 1;
    pH263Param->SliceMode            = 0;
    pH263Param->RandomIntraMBRefresh = 0;
    pH263Param->EnableFRMRateControl = 1;    /* 0: disable, 1: enable */
    pH263Param->Bitrate              = pNAMOutputPort->portDefinition.format.video.nBitrate;
    pH263Param->FrameQp              = 20;
    pH263Param->FrameQp_P            = 20;
    pH263Param->QSCodeMax            = 30;
    pH263Param->QSCodeMin            = 10;
    pH263Param->CBRPeriodRf          = 10;
    pH263Param->PadControlOn         = 0;    /* 0: Use boundary pixel, 1: Use the below setting value */
    pH263Param->LumaPadVal           = 0;
    pH263Param->CbPadVal             = 0;
    pH263Param->CrPadVal             = 0;

    pH263Param->FrameRate            = (pNAMInputPort->portDefinition.format.video.xFramerate) >> 16;

    switch ((NAM_OMX_COLOR_FORMATTYPE)pNAMInputPort->portDefinition.format.video.eColorFormat) {
    case OMX_COLOR_FormatYUV420SemiPlanar:
        pH263Param->FrameMap = NV12_LINEAR;
        break;
    case OMX_NAM_COLOR_FormatNV12TPhysicalAddress:
    default:
        pH263Param->FrameMap = NV12_TILE;
        break;
    }

#ifdef USE_ANDROID_EXTENSION
    if (pNAMInputPort->bStoreMetaDataInBuffer != OMX_FALSE) {
        NAM_OMX_DATA *pInputData = &pNAMComponent->processData[INPUT_PORT_INDEX];
        if(isMetadataBufferTypeGrallocSource(pInputData->dataBuffer) == OMX_TRUE)
            pH263Param->FrameMap = NV12_LINEAR;
        else
            pH263Param->FrameMap = NV12_TILE;
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
    H263PrintParams(*pH263Param);
}

OMX_ERRORTYPE NAM_DMAI_Mpeg4Enc_GetParameter(
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
        NAM_MPEG4ENC_HANDLE       *pMpeg4Enc = NULL;

        ret = NAM_OMX_Check_SizeVersion(pDstMpeg4Param, sizeof(OMX_VIDEO_PARAM_MPEG4TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstMpeg4Param->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Enc = (NAM_MPEG4ENC_HANDLE *)pNAMComponent->hCodecHandle;
        pSrcMpeg4Param = &pMpeg4Enc->mpeg4Component[pDstMpeg4Param->nPortIndex];

        NAM_OSAL_Memcpy(pDstMpeg4Param, pSrcMpeg4Param, sizeof(OMX_VIDEO_PARAM_MPEG4TYPE));
    }
        break;
    case OMX_IndexParamVideoH263:
    {
        OMX_VIDEO_PARAM_H263TYPE  *pDstH263Param = (OMX_VIDEO_PARAM_H263TYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_H263TYPE  *pSrcH263Param = NULL;
        NAM_MPEG4ENC_HANDLE       *pMpeg4Enc = NULL;

        ret = NAM_OMX_Check_SizeVersion(pDstH263Param, sizeof(OMX_VIDEO_PARAM_H263TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstH263Param->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Enc = (NAM_MPEG4ENC_HANDLE *)pNAMComponent->hCodecHandle;
        pSrcH263Param = &pMpeg4Enc->h263Component[pDstH263Param->nPortIndex];

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

        codecType = ((NAM_MPEG4ENC_HANDLE *)(pNAMComponent->hCodecHandle))->hDMAIMpeg4Handle.codecType;
        if (codecType == CODEC_TYPE_MPEG4)
            NAM_OSAL_Strcpy((char *)pComponentRole->cRole, NAM_OMX_COMPONENT_MPEG4_ENC_ROLE);
        else
            NAM_OSAL_Strcpy((char *)pComponentRole->cRole, NAM_OMX_COMPONENT_H263_ENC_ROLE);
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

        codecType = ((NAM_MPEG4ENC_HANDLE *)(pNAMComponent->hCodecHandle))->hDMAIMpeg4Handle.codecType;
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
        NAM_MPEG4ENC_HANDLE              *pMpeg4Enc = NULL;
        OMX_S32                           codecType;

        ret = NAM_OMX_Check_SizeVersion(pDstProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Enc = (NAM_MPEG4ENC_HANDLE *)pNAMComponent->hCodecHandle;
        codecType = pMpeg4Enc->hDMAIMpeg4Handle.codecType;
        if (codecType == CODEC_TYPE_MPEG4) {
            pSrcMpeg4Param = &pMpeg4Enc->mpeg4Component[pDstProfileLevel->nPortIndex];
            pDstProfileLevel->eProfile = pSrcMpeg4Param->eProfile;
            pDstProfileLevel->eLevel = pSrcMpeg4Param->eLevel;
        } else {
            pSrcH263Param = &pMpeg4Enc->h263Component[pDstProfileLevel->nPortIndex];
            pDstProfileLevel->eProfile = pSrcH263Param->eProfile;
            pDstProfileLevel->eLevel = pSrcH263Param->eLevel;
        }
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = NULL;
        NAM_MPEG4ENC_HANDLE                 *pMpeg4Enc = NULL;

        ret = NAM_OMX_Check_SizeVersion(pDstErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstErrorCorrectionType->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Enc = (NAM_MPEG4ENC_HANDLE *)pNAMComponent->hCodecHandle;
        pSrcErrorCorrectionType = &pMpeg4Enc->errorCorrectionType[OUTPUT_PORT_INDEX];

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

OMX_ERRORTYPE NAM_DMAI_Mpeg4Enc_SetParameter(
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
        NAM_MPEG4ENC_HANDLE       *pMpeg4Enc = NULL;

        ret = NAM_OMX_Check_SizeVersion(pSrcMpeg4Param, sizeof(OMX_VIDEO_PARAM_MPEG4TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcMpeg4Param->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Enc = (NAM_MPEG4ENC_HANDLE *)pNAMComponent->hCodecHandle;
        pDstMpeg4Param = &pMpeg4Enc->mpeg4Component[pSrcMpeg4Param->nPortIndex];

        NAM_OSAL_Memcpy(pDstMpeg4Param, pSrcMpeg4Param, sizeof(OMX_VIDEO_PARAM_MPEG4TYPE));
    }
        break;
    case OMX_IndexParamVideoH263:
    {
        OMX_VIDEO_PARAM_H263TYPE *pDstH263Param = NULL;
        OMX_VIDEO_PARAM_H263TYPE *pSrcH263Param = (OMX_VIDEO_PARAM_H263TYPE *)pComponentParameterStructure;
        NAM_MPEG4ENC_HANDLE      *pMpeg4Enc = NULL;

        ret = NAM_OMX_Check_SizeVersion(pSrcH263Param, sizeof(OMX_VIDEO_PARAM_H263TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcH263Param->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Enc = (NAM_MPEG4ENC_HANDLE *)pNAMComponent->hCodecHandle;
        pDstH263Param = &pMpeg4Enc->h263Component[pSrcH263Param->nPortIndex];

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

        if (!NAM_OSAL_Strcmp((char*)pComponentRole->cRole, NAM_OMX_COMPONENT_MPEG4_ENC_ROLE)) {
            pNAMComponent->pNAMPort[OUTPUT_PORT_INDEX].portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG4;
            //((NAM_MPEG4ENC_HANDLE *)(pNAMComponent->hCodecHandle))->hDMAIMpeg4Handle.codecType = CODEC_TYPE_MPEG4;
        } else if (!NAM_OSAL_Strcmp((char*)pComponentRole->cRole, NAM_OMX_COMPONENT_H263_ENC_ROLE)) {
            pNAMComponent->pNAMPort[OUTPUT_PORT_INDEX].portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingH263;
            //((NAM_MPEG4ENC_HANDLE *)(pNAMComponent->hCodecHandle))->hDMAIMpeg4Handle.codecType = CODEC_TYPE_H263;
        } else {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }
    }
        break;
    case OMX_IndexParamVideoProfileLevelCurrent:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pSrcProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_MPEG4TYPE        *pDstMpeg4Param = NULL;
        OMX_VIDEO_PARAM_H263TYPE         *pDstH263Param = NULL;
        NAM_MPEG4ENC_HANDLE              *pMpeg4Enc = NULL;
        OMX_S32                           codecType;

        ret = NAM_OMX_Check_SizeVersion(pSrcProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Enc = (NAM_MPEG4ENC_HANDLE *)pNAMComponent->hCodecHandle;
        codecType = pMpeg4Enc->hDMAIMpeg4Handle.codecType;
        if (codecType == CODEC_TYPE_MPEG4) {
            /*
             * To do: Check validity of profile & level parameters
             */

            pDstMpeg4Param = &pMpeg4Enc->mpeg4Component[pSrcProfileLevel->nPortIndex];
            pDstMpeg4Param->eProfile = pSrcProfileLevel->eProfile;
            pDstMpeg4Param->eLevel = pSrcProfileLevel->eLevel;
        } else {
            /*
             * To do: Check validity of profile & level parameters
             */

            pDstH263Param = &pMpeg4Enc->h263Component[pSrcProfileLevel->nPortIndex];
            pDstH263Param->eProfile = pSrcProfileLevel->eProfile;
            pDstH263Param->eLevel = pSrcProfileLevel->eLevel;
        }
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = NULL;
        NAM_MPEG4ENC_HANDLE                 *pMpeg4Enc = NULL;

        ret = NAM_OMX_Check_SizeVersion(pSrcErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcErrorCorrectionType->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Enc = (NAM_MPEG4ENC_HANDLE *)pNAMComponent->hCodecHandle;
        pDstErrorCorrectionType = &pMpeg4Enc->errorCorrectionType[OUTPUT_PORT_INDEX];

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

OMX_ERRORTYPE NAM_DMAI_Mpeg4Enc_GetConfig(
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

OMX_ERRORTYPE NAM_DMAI_Mpeg4Enc_SetConfig(
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
    default:
        ret = NAM_OMX_SetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_DMAI_Mpeg4Enc_GetExtensionIndex(
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

OMX_ERRORTYPE NAM_DMAI_Mpeg4Enc_ComponentRoleEnum(
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

    codecType = ((NAM_MPEG4ENC_HANDLE *)(pNAMComponent->hCodecHandle))->hDMAIMpeg4Handle.codecType;
    if (codecType == CODEC_TYPE_MPEG4)
        NAM_OSAL_Strcpy((char *)cRole, NAM_OMX_COMPONENT_MPEG4_ENC_ROLE);
    else
        NAM_OSAL_Strcpy((char *)cRole, NAM_OMX_COMPONENT_H263_ENC_ROLE);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_DMAI_EncodeThread(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    NAM_OMX_BASECOMPONENT *pNAMComponent = NULL;
    NAM_MPEG4ENC_HANDLE   *pMpeg4Enc = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pMpeg4Enc = (NAM_MPEG4ENC_HANDLE *)pNAMComponent->hCodecHandle;

    while (pMpeg4Enc->NBEncThread.bExitEncodeThread == OMX_FALSE) {
        NAM_OSAL_SemaphoreWait(pMpeg4Enc->NBEncThread.hEncFrameStart);

        if (pMpeg4Enc->NBEncThread.bExitEncodeThread == OMX_FALSE) {
            pMpeg4Enc->hDMAIMpeg4Handle.returnCodec = SsbSipDmaiEncExe(pMpeg4Enc->hDMAIMpeg4Handle.hDMAIHandle);
            NAM_OSAL_SemaphorePost(pMpeg4Enc->NBEncThread.hEncFrameEnd);
        }
    }

EXIT:
    FunctionOut();
    NAM_OSAL_ThreadExit(NULL);

    return ret;
}

/* DMAI Init */
OMX_ERRORTYPE NAM_DMAI_Mpeg4Enc_Init(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE              ret = OMX_ErrorNone;
    NAM_OMX_BASECOMPONENT     *pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    NAM_OMX_BASEPORT          *pNAMInputPort = &pNAMComponent->pNAMPort[INPUT_PORT_INDEX];
    NAM_OMX_BASEPORT          *pNAMOutputPort = &pNAMComponent->pNAMPort[OUTPUT_PORT_INDEX];
    NAM_OMX_BASEPORT          *pNAMPort = NULL;
    NAM_MPEG4ENC_HANDLE       *pMpeg4Enc = NULL;
    OMX_HANDLETYPE             hDMAIHandle = NULL;
    OMX_S32                    returnCodec = 0;

    FunctionIn();

    pMpeg4Enc = (NAM_MPEG4ENC_HANDLE *)pNAMComponent->hCodecHandle;
    pMpeg4Enc->hDMAIMpeg4Handle.bConfiguredDMAI = OMX_FALSE;
    pNAMComponent->bUseFlagEOF = OMX_FALSE;
    pNAMComponent->bSaveFlagEOS = OMX_FALSE;

    /* DMAI(Multi Format Codec) encoder and CMM(Codec Memory Management) driver open */
    SSBIP_DMAI_BUFFER_TYPE buf_type = CACHE;
    hDMAIHandle = (OMX_PTR)SsbSipDmaiEncOpen(&buf_type);
    if (hDMAIHandle == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    pMpeg4Enc->hDMAIMpeg4Handle.hDMAIHandle = hDMAIHandle;

    /* set DMAI ENC VIDEO PARAM and initialize DMAI encoder instance */
    if (pMpeg4Enc->hDMAIMpeg4Handle.codecType == CODEC_TYPE_MPEG4) {
        SsbSipDmaiEncSetSize(hDMAIHandle, MPEG4_ENC,
                            pNAMOutputPort->portDefinition.format.video.nFrameWidth,
                            pNAMOutputPort->portDefinition.format.video.nFrameHeight);
    } else {
        SsbSipDmaiEncSetSize(hDMAIHandle, H263_ENC,
                            pNAMOutputPort->portDefinition.format.video.nFrameWidth,
                            pNAMOutputPort->portDefinition.format.video.nFrameHeight);
    }

    /* allocate encoder's input buffer */
    returnCodec = SsbSipDmaiEncGetInBuf(hDMAIHandle, &(pMpeg4Enc->hDMAIMpeg4Handle.inputInfo));
    if (returnCodec != DMAI_RET_OK) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    pMpeg4Enc->DMAIEncInputBuffer[0].YPhyAddr = pMpeg4Enc->hDMAIMpeg4Handle.inputInfo.YPhyAddr;
    pMpeg4Enc->DMAIEncInputBuffer[0].CPhyAddr = pMpeg4Enc->hDMAIMpeg4Handle.inputInfo.CPhyAddr;
    pMpeg4Enc->DMAIEncInputBuffer[0].YVirAddr = pMpeg4Enc->hDMAIMpeg4Handle.inputInfo.YVirAddr;
    pMpeg4Enc->DMAIEncInputBuffer[0].CVirAddr = pMpeg4Enc->hDMAIMpeg4Handle.inputInfo.CVirAddr;
    pMpeg4Enc->DMAIEncInputBuffer[0].YBufferSize = pMpeg4Enc->hDMAIMpeg4Handle.inputInfo.YSize;
    pMpeg4Enc->DMAIEncInputBuffer[0].CBufferSize = pMpeg4Enc->hDMAIMpeg4Handle.inputInfo.CSize;
    pMpeg4Enc->DMAIEncInputBuffer[0].YDataSize = 0;
    pMpeg4Enc->DMAIEncInputBuffer[0].CDataSize = 0;
    NAM_OSAL_Log(NAM_LOG_TRACE, "pMpeg4Enc->hDMAIMpeg4Handle.inputInfo.YVirAddr : 0x%x", pMpeg4Enc->hDMAIMpeg4Handle.inputInfo.YVirAddr);
    NAM_OSAL_Log(NAM_LOG_TRACE, "pMpeg4Enc->hDMAIMpeg4Handle.inputInfo.CVirAddr : 0x%x", pMpeg4Enc->hDMAIMpeg4Handle.inputInfo.CVirAddr);

    /* allocate encoder's input buffer */
    returnCodec = SsbSipDmaiEncGetInBuf(hDMAIHandle, &(pMpeg4Enc->hDMAIMpeg4Handle.inputInfo));
    if (returnCodec != DMAI_RET_OK) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    pMpeg4Enc->DMAIEncInputBuffer[1].YPhyAddr = pMpeg4Enc->hDMAIMpeg4Handle.inputInfo.YPhyAddr;
    pMpeg4Enc->DMAIEncInputBuffer[1].CPhyAddr = pMpeg4Enc->hDMAIMpeg4Handle.inputInfo.CPhyAddr;
    pMpeg4Enc->DMAIEncInputBuffer[1].YVirAddr = pMpeg4Enc->hDMAIMpeg4Handle.inputInfo.YVirAddr;
    pMpeg4Enc->DMAIEncInputBuffer[1].CVirAddr = pMpeg4Enc->hDMAIMpeg4Handle.inputInfo.CVirAddr;
    pMpeg4Enc->DMAIEncInputBuffer[1].YBufferSize = pMpeg4Enc->hDMAIMpeg4Handle.inputInfo.YSize;
    pMpeg4Enc->DMAIEncInputBuffer[1].CBufferSize = pMpeg4Enc->hDMAIMpeg4Handle.inputInfo.CSize;
    pMpeg4Enc->DMAIEncInputBuffer[1].YDataSize = 0;
    pMpeg4Enc->DMAIEncInputBuffer[1].CDataSize = 0;
    NAM_OSAL_Log(NAM_LOG_TRACE, "pMpeg4Enc->hDMAIMpeg4Handle.inputInfo.YVirAddr : 0x%x", pMpeg4Enc->hDMAIMpeg4Handle.inputInfo.YVirAddr);
    NAM_OSAL_Log(NAM_LOG_TRACE, "pMpeg4Enc->hDMAIMpeg4Handle.inputInfo.CVirAddr : 0x%x", pMpeg4Enc->hDMAIMpeg4Handle.inputInfo.CVirAddr);

    pNAMComponent->processData[INPUT_PORT_INDEX].specificBufferHeader.YPhyAddr = pMpeg4Enc->DMAIEncInputBuffer[0].YPhyAddr;
    pNAMComponent->processData[INPUT_PORT_INDEX].specificBufferHeader.CPhyAddr = pMpeg4Enc->DMAIEncInputBuffer[0].CPhyAddr;
    pNAMComponent->processData[INPUT_PORT_INDEX].specificBufferHeader.YVirAddr = pMpeg4Enc->DMAIEncInputBuffer[0].YVirAddr;
    pNAMComponent->processData[INPUT_PORT_INDEX].specificBufferHeader.CVirAddr = pMpeg4Enc->DMAIEncInputBuffer[0].CVirAddr;
    pNAMComponent->processData[INPUT_PORT_INDEX].specificBufferHeader.YSize = pMpeg4Enc->DMAIEncInputBuffer[0].YBufferSize;
    pNAMComponent->processData[INPUT_PORT_INDEX].specificBufferHeader.CSize = pMpeg4Enc->DMAIEncInputBuffer[0].CBufferSize;

    pMpeg4Enc->indexInputBuffer = 0;
    pMpeg4Enc->bFirstFrame = OMX_TRUE;

    pMpeg4Enc->NBEncThread.bExitEncodeThread = OMX_FALSE;
    pMpeg4Enc->NBEncThread.bEncoderRun = OMX_FALSE;
    NAM_OSAL_SemaphoreCreate(&(pMpeg4Enc->NBEncThread.hEncFrameStart));
    NAM_OSAL_SemaphoreCreate(&(pMpeg4Enc->NBEncThread.hEncFrameEnd));
    if (OMX_ErrorNone == NAM_OSAL_ThreadCreate(&pMpeg4Enc->NBEncThread.hNBEncodeThread,
                                                NAM_DMAI_EncodeThread,
                                                pOMXComponent)) {
        pMpeg4Enc->hDMAIMpeg4Handle.returnCodec = DMAI_RET_OK;
    }

    NAM_OSAL_Memset(pNAMComponent->timeStamp, -19771003, sizeof(OMX_TICKS) * MAX_TIMESTAMP);
    NAM_OSAL_Memset(pNAMComponent->nFlags, 0, sizeof(OMX_U32) * MAX_FLAGS);
    pMpeg4Enc->hDMAIMpeg4Handle.indexTimestamp = 0;

EXIT:
    FunctionOut();

    return ret;
}

/* DMAI Terminate */
OMX_ERRORTYPE NAM_DMAI_Mpeg4Enc_Terminate(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    NAM_OMX_BASECOMPONENT *pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    NAM_MPEG4ENC_HANDLE   *pMpeg4Enc = NULL;
    OMX_HANDLETYPE         hDMAIHandle = NULL;

    FunctionIn();

    pMpeg4Enc = (NAM_MPEG4ENC_HANDLE *)pNAMComponent->hCodecHandle;

    if (pMpeg4Enc->NBEncThread.hNBEncodeThread != NULL) {
        pMpeg4Enc->NBEncThread.bExitEncodeThread = OMX_TRUE;
        NAM_OSAL_SemaphorePost(pMpeg4Enc->NBEncThread.hEncFrameStart);
        NAM_OSAL_ThreadTerminate(pMpeg4Enc->NBEncThread.hNBEncodeThread);
        pMpeg4Enc->NBEncThread.hNBEncodeThread = NULL;
    }

    if(pMpeg4Enc->NBEncThread.hEncFrameEnd != NULL) {
        NAM_OSAL_SemaphoreTerminate(pMpeg4Enc->NBEncThread.hEncFrameEnd);
        pMpeg4Enc->NBEncThread.hEncFrameEnd = NULL;
    }

    if(pMpeg4Enc->NBEncThread.hEncFrameStart != NULL) {
        NAM_OSAL_SemaphoreTerminate(pMpeg4Enc->NBEncThread.hEncFrameStart);
        pMpeg4Enc->NBEncThread.hEncFrameStart = NULL;
    }

    hDMAIHandle = pMpeg4Enc->hDMAIMpeg4Handle.hDMAIHandle;
    if (hDMAIHandle != NULL) {
        SsbSipDmaiEncClose(hDMAIHandle);
        pMpeg4Enc->hDMAIMpeg4Handle.hDMAIHandle = NULL;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_DMAI_Mpeg4_Encode(OMX_COMPONENTTYPE *pOMXComponent, NAM_OMX_DATA *pInputData, NAM_OMX_DATA *pOutputData)
{
    OMX_ERRORTYPE              ret = OMX_ErrorNone;
    NAM_OMX_BASECOMPONENT     *pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    NAM_MPEG4ENC_HANDLE       *pMpeg4Enc = (NAM_MPEG4ENC_HANDLE *)pNAMComponent->hCodecHandle;
    OMX_HANDLETYPE             hDMAIHandle = pMpeg4Enc->hDMAIMpeg4Handle.hDMAIHandle;
    SSBSIP_DMAI_ENC_INPUT_INFO *pInputInfo = &(pMpeg4Enc->hDMAIMpeg4Handle.inputInfo);
    SSBSIP_DMAI_ENC_OUTPUT_INFO outputInfo;
    NAM_OMX_BASEPORT          *pNAMPort = NULL;
    DMAI_ENC_ADDR_INFO          addrInfo;
    OMX_U32                    oneFrameSize = pInputData->dataLen;

    FunctionIn();

    if (pMpeg4Enc->hDMAIMpeg4Handle.bConfiguredDMAI == OMX_FALSE) {
        /* set DMAI ENC VIDEO PARAM and initialize DMAI encoder instance */
        if (pMpeg4Enc->hDMAIMpeg4Handle.codecType == CODEC_TYPE_MPEG4) {
            Set_Mpeg4Enc_Param(&(pMpeg4Enc->hDMAIMpeg4Handle.mpeg4DMAIParam), pNAMComponent);
            pMpeg4Enc->hDMAIMpeg4Handle.returnCodec = SsbSipDmaiEncInit(hDMAIHandle, &(pMpeg4Enc->hDMAIMpeg4Handle.mpeg4DMAIParam));
        } else {
            Set_H263Enc_Param(&(pMpeg4Enc->hDMAIMpeg4Handle.h263DMAIParam), pNAMComponent);
            pMpeg4Enc->hDMAIMpeg4Handle.returnCodec = SsbSipDmaiEncInit(hDMAIHandle, &(pMpeg4Enc->hDMAIMpeg4Handle.h263DMAIParam));
        }
        if (pMpeg4Enc->hDMAIMpeg4Handle.returnCodec != DMAI_RET_OK) {
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }

        pMpeg4Enc->hDMAIMpeg4Handle.returnCodec = SsbSipDmaiEncGetOutBuf(hDMAIHandle, &outputInfo);
        if (pMpeg4Enc->hDMAIMpeg4Handle.returnCodec != DMAI_RET_OK)
        {
            NAM_OSAL_Log(NAM_LOG_ERROR, "%s: SsbSipDmaiEncGetOutBuf failed, ret:%d", __FUNCTION__, pMpeg4Enc->hDMAIMpeg4Handle.returnCodec);
            ret = OMX_ErrorUndefined;
            goto EXIT;
        }

        pOutputData->dataBuffer = outputInfo.StrmVirAddr;
        pOutputData->allocSize = outputInfo.headerSize;
        pOutputData->dataLen = outputInfo.headerSize;
        pOutputData->timeStamp = 0;
        pOutputData->nFlags |= OMX_BUFFERFLAG_CODECCONFIG;
        pOutputData->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;

        pMpeg4Enc->hDMAIMpeg4Handle.bConfiguredDMAI = OMX_TRUE;

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
        pInputInfo->YPhyAddr = pMpeg4Enc->DMAIEncInputBuffer[pMpeg4Enc->indexInputBuffer].YPhyAddr;
        pInputInfo->CPhyAddr = pMpeg4Enc->DMAIEncInputBuffer[pMpeg4Enc->indexInputBuffer].CPhyAddr;
        pInputInfo->YVirAddr = pMpeg4Enc->DMAIEncInputBuffer[pMpeg4Enc->indexInputBuffer].YVirAddr;
        pInputInfo->CVirAddr = pMpeg4Enc->DMAIEncInputBuffer[pMpeg4Enc->indexInputBuffer].CVirAddr;
    } else if (pNAMPort->portDefinition.format.video.eColorFormat == OMX_NAM_COLOR_FormatNV12TPhysicalAddress) {
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

    pNAMComponent->timeStamp[pMpeg4Enc->hDMAIMpeg4Handle.indexTimestamp] = pInputData->timeStamp;
    pNAMComponent->nFlags[pMpeg4Enc->hDMAIMpeg4Handle.indexTimestamp] = pInputData->nFlags;

    if ((pMpeg4Enc->hDMAIMpeg4Handle.returnCodec == DMAI_RET_OK) &&
        (pMpeg4Enc->bFirstFrame == OMX_FALSE)) {
        OMX_S32 indexTimestamp = 0;

        /* wait for dmai encode done */
        if (pMpeg4Enc->NBEncThread.bEncoderRun != OMX_FALSE) {
            NAM_OSAL_SemaphoreWait(pMpeg4Enc->NBEncThread.hEncFrameEnd);
            pMpeg4Enc->NBEncThread.bEncoderRun = OMX_FALSE;
        }

        pMpeg4Enc->hDMAIMpeg4Handle.returnCodec = SsbSipDmaiEncGetOutBuf(hDMAIHandle, &outputInfo);
        if ((SsbSipDmaiEncGetConfig(hDMAIHandle, DMAI_ENC_GETCONF_FRAME_TAG, &indexTimestamp) != DMAI_RET_OK) ||
            (((indexTimestamp < 0) || (indexTimestamp >= MAX_TIMESTAMP)))) {
            pOutputData->timeStamp = pInputData->timeStamp;
            pOutputData->nFlags = pInputData->nFlags;
        } else {
            pOutputData->timeStamp = pNAMComponent->timeStamp[indexTimestamp];
            pOutputData->nFlags = pNAMComponent->nFlags[indexTimestamp];
        }

        if (pMpeg4Enc->hDMAIMpeg4Handle.returnCodec == DMAI_RET_OK) {
            /** Fill Output Buffer **/
            pOutputData->dataBuffer = outputInfo.StrmVirAddr;
            pOutputData->allocSize = outputInfo.dataSize;
            pOutputData->dataLen = outputInfo.dataSize;
            pOutputData->usedDataLen = 0;

            pOutputData->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
            if (outputInfo.frameType == DMAI_FRAME_TYPE_I_FRAME)
                    pOutputData->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;

            ret = OMX_ErrorNone;
        } else {
            NAM_OSAL_Log(NAM_LOG_ERROR, "%s: SsbSipDmaiEncGetOutBuf failed, ret:%d", __FUNCTION__, pMpeg4Enc->hDMAIMpeg4Handle.returnCodec);
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
    if (pMpeg4Enc->hDMAIMpeg4Handle.returnCodec != DMAI_RET_OK) {
        NAM_OSAL_Log(NAM_LOG_ERROR, "%s: SsbSipDmaiEncExe failed, ret:%d", __FUNCTION__, pMpeg4Enc->hDMAIMpeg4Handle.returnCodec);
        ret = OMX_ErrorUndefined;
    }

    pMpeg4Enc->hDMAIMpeg4Handle.returnCodec = SsbSipDmaiEncSetInBuf(hDMAIHandle, pInputInfo);
    if (pMpeg4Enc->hDMAIMpeg4Handle.returnCodec != DMAI_RET_OK) {
        NAM_OSAL_Log(NAM_LOG_ERROR, "%s: SsbSipDmaiEncSetInBuf failed, ret:%d", __FUNCTION__, pMpeg4Enc->hDMAIMpeg4Handle.returnCodec);
        ret = OMX_ErrorUndefined;
        goto EXIT;
    } else {
        pMpeg4Enc->indexInputBuffer++;
        pMpeg4Enc->indexInputBuffer %= DMAI_INPUT_BUFFER_NUM_MAX;
        pNAMComponent->processData[INPUT_PORT_INDEX].specificBufferHeader.YPhyAddr = pMpeg4Enc->DMAIEncInputBuffer[pMpeg4Enc->indexInputBuffer].YPhyAddr;
        pNAMComponent->processData[INPUT_PORT_INDEX].specificBufferHeader.CPhyAddr = pMpeg4Enc->DMAIEncInputBuffer[pMpeg4Enc->indexInputBuffer].CPhyAddr;
        pNAMComponent->processData[INPUT_PORT_INDEX].specificBufferHeader.YVirAddr = pMpeg4Enc->DMAIEncInputBuffer[pMpeg4Enc->indexInputBuffer].YVirAddr;
        pNAMComponent->processData[INPUT_PORT_INDEX].specificBufferHeader.CVirAddr = pMpeg4Enc->DMAIEncInputBuffer[pMpeg4Enc->indexInputBuffer].CVirAddr;
        pNAMComponent->processData[INPUT_PORT_INDEX].specificBufferHeader.YSize = pMpeg4Enc->DMAIEncInputBuffer[pMpeg4Enc->indexInputBuffer].YBufferSize;
        pNAMComponent->processData[INPUT_PORT_INDEX].specificBufferHeader.CSize = pMpeg4Enc->DMAIEncInputBuffer[pMpeg4Enc->indexInputBuffer].CBufferSize;
    }

    SsbSipDmaiEncSetConfig(hDMAIHandle, DMAI_ENC_SETCONF_FRAME_TAG, &(pMpeg4Enc->hDMAIMpeg4Handle.indexTimestamp));

    /* dmai encode start */
    NAM_OSAL_SemaphorePost(pMpeg4Enc->NBEncThread.hEncFrameStart);
    pMpeg4Enc->NBEncThread.bEncoderRun = OMX_TRUE;
    pMpeg4Enc->hDMAIMpeg4Handle.indexTimestamp++;
    pMpeg4Enc->hDMAIMpeg4Handle.indexTimestamp %= MAX_TIMESTAMP;
    pMpeg4Enc->bFirstFrame = OMX_FALSE;

EXIT:
    FunctionOut();

    return ret;
}

/* DMAI Encode */
OMX_ERRORTYPE NAM_DMAI_Mpeg4Enc_bufferProcess(OMX_COMPONENTTYPE *pOMXComponent, NAM_OMX_DATA *pInputData, NAM_OMX_DATA *pOutputData)
{
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    NAM_OMX_BASECOMPONENT   *pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    NAM_OMX_BASEPORT        *pInputPort = &pNAMComponent->pNAMPort[INPUT_PORT_INDEX];
    NAM_OMX_BASEPORT        *pOutputPort = &pNAMComponent->pNAMPort[OUTPUT_PORT_INDEX];

    FunctionIn();

    if ((!CHECK_PORT_ENABLED(pInputPort)) || (!CHECK_PORT_ENABLED(pOutputPort)) ||
            (!CHECK_PORT_POPULATED(pInputPort)) || (!CHECK_PORT_POPULATED(pOutputPort))) {
        goto EXIT;
    }
    if (OMX_FALSE == NAM_Check_BufferProcess_State(pNAMComponent)) {
        goto EXIT;
    }

    ret = NAM_DMAI_Mpeg4_Encode(pOMXComponent, pInputData, pOutputData);
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
    NAM_MPEG4ENC_HANDLE     *pMpeg4Enc = NULL;
    OMX_S32                  codecType = -1;
    int i = 0;

    FunctionIn();

    if ((hComponent == NULL) || (componentName == NULL)) {
        ret = OMX_ErrorBadParameter;
        NAM_OSAL_Log(NAM_LOG_ERROR, "%s: parameters are null, ret: %X", __FUNCTION__, ret);
        goto EXIT;
    }
    if (NAM_OSAL_Strcmp(NAM_OMX_COMPONENT_MPEG4_ENC, componentName) == 0) {
        codecType = CODEC_TYPE_MPEG4;
    } else if (NAM_OSAL_Strcmp(NAM_OMX_COMPONENT_H263_ENC, componentName) == 0) {
        codecType = CODEC_TYPE_H263;
    } else {
        ret = OMX_ErrorBadParameter;
        NAM_OSAL_Log(NAM_LOG_ERROR, "%s: componentName(%s) error, ret: %X", __FUNCTION__, componentName, ret);
        goto EXIT;
    }

    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = NAM_OMX_VideoEncodeComponentInit(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        NAM_OSAL_Log(NAM_LOG_ERROR, "%s: NAM_OMX_VideoDecodeComponentInit error, ret: %X", __FUNCTION__, ret);
        goto EXIT;
    }
    pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pNAMComponent->codecType = HW_VIDEO_CODEC;

    pNAMComponent->componentName = (OMX_STRING)NAM_OSAL_Malloc(MAX_OMX_COMPONENT_NAME_SIZE);
    if (pNAMComponent->componentName == NULL) {
        NAM_OMX_VideoEncodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        NAM_OSAL_Log(NAM_LOG_ERROR, "%s: componentName alloc error, ret: %X", __FUNCTION__, ret);
        goto EXIT;
    }
    NAM_OSAL_Memset(pNAMComponent->componentName, 0, MAX_OMX_COMPONENT_NAME_SIZE);

    pMpeg4Enc = NAM_OSAL_Malloc(sizeof(NAM_MPEG4ENC_HANDLE));
    if (pMpeg4Enc == NULL) {
        NAM_OMX_VideoEncodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        NAM_OSAL_Log(NAM_LOG_ERROR, "%s: NAM_MPEG4ENC_HANDLE alloc error, ret: %X", __FUNCTION__, ret);
        goto EXIT;
    }
    NAM_OSAL_Memset(pMpeg4Enc, 0, sizeof(NAM_MPEG4ENC_HANDLE));
    pNAMComponent->hCodecHandle = (OMX_HANDLETYPE)pMpeg4Enc;
    pMpeg4Enc->hDMAIMpeg4Handle.codecType = codecType;

    if (codecType == CODEC_TYPE_MPEG4)
        NAM_OSAL_Strcpy(pNAMComponent->componentName, NAM_OMX_COMPONENT_MPEG4_ENC);
    else
        NAM_OSAL_Strcpy(pNAMComponent->componentName, NAM_OMX_COMPONENT_H263_ENC);

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
    pNAMPort->portDefinition.format.video.nBitrate = 64000;
    pNAMPort->portDefinition.format.video.xFramerate= (15 << 16);
    pNAMPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    pNAMPort->portDefinition.format.video.pNativeRender = 0;
    pNAMPort->portDefinition.format.video.bFlagErrorConcealment = OMX_FALSE;
    NAM_OSAL_Memset(pNAMPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
    NAM_OSAL_Strcpy(pNAMPort->portDefinition.format.video.cMIMEType, "raw/video");
    pNAMPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
    pNAMPort->portDefinition.nBufferSize = DEFAULT_VIDEO_INPUT_BUFFER_SIZE;
    pNAMPort->portDefinition.bEnabled = OMX_TRUE;

    /* Output port */
    pNAMPort = &pNAMComponent->pNAMPort[OUTPUT_PORT_INDEX];
    pNAMPort->portDefinition.format.video.nFrameWidth = DEFAULT_FRAME_WIDTH;
    pNAMPort->portDefinition.format.video.nFrameHeight= DEFAULT_FRAME_HEIGHT;
    pNAMPort->portDefinition.format.video.nBitrate = 64000;
    pNAMPort->portDefinition.format.video.xFramerate= (15 << 16);
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
    pNAMPort->portDefinition.nBufferSize = DEFAULT_VIDEO_OUTPUT_BUFFER_SIZE;
    pNAMPort->portDefinition.bEnabled = OMX_TRUE;

    if (codecType == CODEC_TYPE_MPEG4) {
        for(i = 0; i < ALL_PORT_NUM; i++) {
            INIT_SET_SIZE_VERSION(&pMpeg4Enc->mpeg4Component[i], OMX_VIDEO_PARAM_MPEG4TYPE);
            pMpeg4Enc->mpeg4Component[i].nPortIndex = i;
            pMpeg4Enc->mpeg4Component[i].eProfile   = OMX_VIDEO_MPEG4ProfileSimple;
            pMpeg4Enc->mpeg4Component[i].eLevel     = OMX_VIDEO_MPEG4Level4;

            pMpeg4Enc->mpeg4Component[i].nPFrames = 10;
            pMpeg4Enc->mpeg4Component[i].nBFrames = 0;          /* No support for B frames */
            pMpeg4Enc->mpeg4Component[i].nMaxPacketSize = 256;  /* Default value */
            pMpeg4Enc->mpeg4Component[i].nAllowedPictureTypes =  OMX_VIDEO_PictureTypeI | OMX_VIDEO_PictureTypeP;
            pMpeg4Enc->mpeg4Component[i].bGov = OMX_FALSE;

        }
    } else {
        for(i = 0; i < ALL_PORT_NUM; i++) {
            INIT_SET_SIZE_VERSION(&pMpeg4Enc->h263Component[i], OMX_VIDEO_PARAM_H263TYPE);
            pMpeg4Enc->h263Component[i].nPortIndex = i;
            pMpeg4Enc->h263Component[i].eProfile   = OMX_VIDEO_H263ProfileBaseline;
            pMpeg4Enc->h263Component[i].eLevel     = OMX_VIDEO_H263Level45;

            pMpeg4Enc->h263Component[i].nPFrames = 20;
            pMpeg4Enc->h263Component[i].nBFrames = 0;          /* No support for B frames */
            pMpeg4Enc->h263Component[i].bPLUSPTYPEAllowed = OMX_FALSE;
            pMpeg4Enc->h263Component[i].nAllowedPictureTypes = OMX_VIDEO_PictureTypeI | OMX_VIDEO_PictureTypeP;
            pMpeg4Enc->h263Component[i].bForceRoundingTypeToZero = OMX_TRUE;
            pMpeg4Enc->h263Component[i].nPictureHeaderRepetition = 0;
            pMpeg4Enc->h263Component[i].nGOBHeaderInterval = 0;
        }
    }

    pOMXComponent->GetParameter      = &NAM_DMAI_Mpeg4Enc_GetParameter;
    pOMXComponent->SetParameter      = &NAM_DMAI_Mpeg4Enc_SetParameter;
    pOMXComponent->GetConfig         = &NAM_DMAI_Mpeg4Enc_GetConfig;
    pOMXComponent->SetConfig         = &NAM_DMAI_Mpeg4Enc_SetConfig;
    pOMXComponent->GetExtensionIndex = &NAM_DMAI_Mpeg4Enc_GetExtensionIndex;
    pOMXComponent->ComponentRoleEnum = &NAM_DMAI_Mpeg4Enc_ComponentRoleEnum;
    pOMXComponent->ComponentDeInit   = &NAM_OMX_ComponentDeinit;

    pNAMComponent->nam_dmai_componentInit      = &NAM_DMAI_Mpeg4Enc_Init;
    pNAMComponent->nam_dmai_componentTerminate = &NAM_DMAI_Mpeg4Enc_Terminate;
    pNAMComponent->nam_dmai_bufferProcess      = &NAM_DMAI_Mpeg4Enc_bufferProcess;
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
    NAM_MPEG4ENC_HANDLE     *pMpeg4Enc = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    pNAMComponent = (NAM_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    NAM_OSAL_Free(pNAMComponent->componentName);
    pNAMComponent->componentName = NULL;

    pMpeg4Enc = (NAM_MPEG4ENC_HANDLE *)pNAMComponent->hCodecHandle;
    if (pMpeg4Enc != NULL) {
        NAM_OSAL_Free(pMpeg4Enc);
        pMpeg4Enc = pNAMComponent->hCodecHandle = NULL;
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

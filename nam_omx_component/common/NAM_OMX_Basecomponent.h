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
 * @file       NAM_OMX_Basecomponent.h
 * @brief
 * @author     SeungBeom Kim (sbcrux.kim@samsung.com)
 *             Yunji Kim (yunji.kim@samsung.com)
 * @version    1.0
 * @history
 *    2010.7.15 : Create
 */

#ifndef NAM_OMX_BASECOMP
#define NAM_OMX_BASECOMP

#include "NAM_OMX_Def.h"
#include "OMX_Component.h"
#include "NAM_OSAL_Queue.h"
#include "NAM_OMX_Baseport.h"


typedef struct _NAM_OMX_MESSAGE
{
    OMX_U32 messageType;
    OMX_U32 messageParam;
    OMX_PTR pCmdData;
} NAM_OMX_MESSAGE;

typedef struct _NAM_OMX_DATABUFFER
{
    OMX_HANDLETYPE        bufferMutex;
    OMX_BUFFERHEADERTYPE* bufferHeader;
    OMX_BOOL              dataValid;
    OMX_U32               allocSize;
    OMX_U32               dataLen;
    OMX_U32               usedDataLen;
    OMX_U32               remainDataLen;
    OMX_U32               nFlags;
    OMX_TICKS             timeStamp;
} NAM_OMX_DATABUFFER;

typedef struct _NAM_BUFFER_HEADER{
    void *YPhyAddr; // [IN/OUT] physical address of Y
    void *CPhyAddr; // [IN/OUT] physical address of CbCr
    void *YVirAddr; // [IN/OUT] virtual address of Y
    void *CVirAddr; // [IN/OUT] virtual address of CbCr
    int YSize;      // [IN/OUT] input size of Y data
    int CSize;      // [IN/OUT] input size of CbCr data
} NAM_BUFFER_HEADER;

typedef struct _NAM_OMX_DATA
{
    OMX_BYTE  dataBuffer;
    OMX_U32   allocSize;
    OMX_U32   dataLen;
    OMX_U32   usedDataLen;
    OMX_U32   remainDataLen;
    OMX_U32   previousDataLen;
    OMX_U32   nFlags;
    OMX_TICKS timeStamp;
    NAM_BUFFER_HEADER specificBufferHeader;
} NAM_OMX_DATA;

/* for Check TimeStamp after Seek */
typedef struct _NAM_OMX_TIMESTAPM
{
    OMX_BOOL  needSetStartTimeStamp;
    OMX_BOOL  needCheckStartTimeStamp;
    OMX_TICKS startTimeStamp;
    OMX_U32   nStartFlags;
} NAM_OMX_TIMESTAMP;

typedef struct _NAM_OMX_BASECOMPONENT
{
    OMX_STRING               componentName;
    OMX_VERSIONTYPE          componentVersion;
    OMX_VERSIONTYPE          specVersion;

    OMX_STATETYPE            currentState;
    NAM_OMX_TRANS_STATETYPE  transientState;

    NAM_CODEC_TYPE           codecType;
    NAM_OMX_PRIORITYMGMTTYPE compPriority;
    OMX_MARKTYPE             propagateMarkType;
    OMX_HANDLETYPE           compMutex;

    OMX_HANDLETYPE           hCodecHandle;

    /* Message Handler */
    OMX_BOOL                 bExitMessageHandlerThread;
    OMX_HANDLETYPE           hMessageHandler;
    OMX_HANDLETYPE           msgSemaphoreHandle;
    NAM_QUEUE                messageQ;

    /* Buffer Process */
    OMX_BOOL                 bExitBufferProcessThread;
    OMX_HANDLETYPE           hBufferProcess;

    /* Buffer */
    NAM_OMX_DATABUFFER       namDataBuffer[2];

    /* Data */
    NAM_OMX_DATA             processData[2];

    /* Port */
    OMX_PORT_PARAM_TYPE      portParam;
    NAM_OMX_BASEPORT        *pNAMPort;

    OMX_HANDLETYPE           pauseEvent;

    /* Callback function */
    OMX_CALLBACKTYPE        *pCallbacks;
    OMX_PTR                  callbackData;

    /* Save Timestamp */
    OMX_TICKS                timeStamp[MAX_TIMESTAMP];
    NAM_OMX_TIMESTAMP        checkTimeStamp;

    /* Save Flags */
    OMX_U32                  nFlags[MAX_FLAGS];

    OMX_BOOL                 getAllDelayBuffer;
    OMX_BOOL                 remainOutputData;
    OMX_BOOL                 reInputData;

    /* Android CapabilityFlags */
    OMXComponentCapabilityFlagsType capabilityFlags;

    OMX_BOOL bUseFlagEOF;
    OMX_BOOL bSaveFlagEOS;

    OMX_ERRORTYPE (*nam_dmai_componentInit)(OMX_COMPONENTTYPE *pOMXComponent);
    OMX_ERRORTYPE (*nam_dmai_componentTerminate)(OMX_COMPONENTTYPE *pOMXComponent);
    OMX_ERRORTYPE (*nam_dmai_bufferProcess) (OMX_COMPONENTTYPE *pOMXComponent, NAM_OMX_DATA *pInputData, NAM_OMX_DATA *pOutputData);

    OMX_ERRORTYPE (*nam_AllocateTunnelBuffer)(NAM_OMX_BASEPORT *pOMXBasePort, OMX_U32 nPortIndex);
    OMX_ERRORTYPE (*nam_FreeTunnelBuffer)(NAM_OMX_BASEPORT *pOMXBasePort, OMX_U32 nPortIndex);
    OMX_ERRORTYPE (*nam_BufferProcess)(OMX_HANDLETYPE hComponent);
    OMX_ERRORTYPE (*nam_BufferReset)(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex);
    OMX_ERRORTYPE (*nam_InputBufferReturn)(OMX_COMPONENTTYPE *pOMXComponent);
    OMX_ERRORTYPE (*nam_OutputBufferReturn)(OMX_COMPONENTTYPE *pOMXComponent);

    int (*nam_checkInputFrame)(unsigned char *pInputStream, int buffSize, OMX_U32 flag, OMX_BOOL bPreviousFrameEOF, OMX_BOOL *pbEndOfFrame);

} NAM_OMX_BASECOMPONENT;


#ifdef __cplusplus
extern "C" {
#endif

    OMX_ERRORTYPE NAM_OMX_Check_SizeVersion(OMX_PTR header, OMX_U32 size);


#ifdef __cplusplus
};
#endif

#endif

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
 * @file        NAM_OMX_Vdec.h
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 *              HyeYeon Chung (hyeon.chung@samsung.com)
 *              Yunji Kim (yunji.kim@samsung.com)
 * @version     1.0
 * @history
 *   2010.7.15 : Create
 */

#ifndef NAM_OMX_VIDEO_DECODE
#define NAM_OMX_VIDEO_DECODE

#include "OMX_Component.h"
#include "NAM_OMX_Def.h"
#include "NAM_OSAL_Queue.h"
#include "NAM_OMX_Baseport.h"


///////////////////////////////////////////////////////////////////////////////
// constants definitions 

/* ...desired input buffer size */
#define OMX_DSP_INPUT_BUFFER_SIZE       65536

/* ...desired amount of input buffers */
#define OMX_DSP_INPUT_BUFFER_NUMBER     4

/* ...required amount of output port buffers - display holds 3 buffers */
#define OMX_DSP_OUTPUT_BUFFER_NUMBER    6

/* num of output port buffers for 720P codecs */
#define OMX_DSP_OUTPUT_BUFFER_NUMBER_HP 5

/* num of DSP decoder internal output buffers (BufTab)
 * it might be more than the number of output port buffers,
 * if the codec holds more than 1 buffer */
#define OMX_DSP_OUTPUT_BUFTAB_NUMBER    5

// DSP engine support

/* ...buffer used by codec */
#define OMX_DSP_CODEC_MASK              (1 << 0)

/* ...buffer used by display */
#define OMX_DSP_DISPLAY_MASK            (1 << 1)

///////////////////////////////////////////////////////////////////////////////

#define MAX_VIDEO_INPUTBUFFER_NUM    5
// MAX_VIDEO_OUTPUTBUFFER_NUM = OMX_DSP_OUTPUT_BUFFER_NUMBER_HP
// #define MAX_VIDEO_OUTPUTBUFFER_NUM   2
#define MAX_VIDEO_OUTPUTBUFFER_NUM   5

#define DEFAULT_FRAME_WIDTH          176
#define DEFAULT_FRAME_HEIGHT         144

#define DEFAULT_VIDEO_INPUT_BUFFER_SIZE    ((DEFAULT_FRAME_WIDTH * DEFAULT_FRAME_HEIGHT) * 2)

// For TI OMX_COLOR_FormatCbYCrY
//#define DEFAULT_VIDEO_OUTPUT_BUFFER_SIZE   ((DEFAULT_FRAME_WIDTH * DEFAULT_FRAME_HEIGHT * 3) / 2)
#define DEFAULT_VIDEO_OUTPUT_BUFFER_SIZE   ((DEFAULT_FRAME_WIDTH * DEFAULT_FRAME_HEIGHT) * 2)

#define DMAI_INPUT_BUFFER_NUM_MAX         2

// For TI OMX_COLOR_FormatCbYCrY
#define DEFAULT_DMAI_INPUT_BUFFER_SIZE    (1280 * 720 * 2)

#define INPUT_PORT_SUPPORTFORMAT_NUM_MAX    1

//#define OUTPUT_PORT_SUPPORTFORMAT_NUM_MAX   3
// only OMX_COLOR_FormatCbYCrY
#define OUTPUT_PORT_SUPPORTFORMAT_NUM_MAX   1

#ifdef USE_ANDROID_EXTENSION
#define ANDROID_MAX_VIDEO_OUTPUTBUFFER_NUM   1
#endif

typedef struct
{
    void *pAddrY;
    void *pAddrC;
} DMAI_DEC_ADDR_INFO;

typedef struct _NAM_DMAI_NBDEC_THREAD
{
    OMX_HANDLETYPE  hNBDecodeThread;
    OMX_HANDLETYPE  hDecFrameStart;
    OMX_HANDLETYPE  hDecFrameEnd;
    OMX_BOOL        bExitDecodeThread;
    OMX_BOOL        bDecoderRun;

    OMX_U32         oneFrameSize;
} NAM_DMAI_NBDEC_THREAD;

typedef struct _DMAI_DEC_INPUT_BUFFER
{
    void *PhyAddr;      // physical address
    void *VirAddr;      // virtual address
    int   bufferSize;   // input buffer alloc size
    int   dataSize;     // Data length
} DMAI_DEC_INPUT_BUFFER;

#ifdef __cplusplus
extern "C" {
#endif

OMX_ERRORTYPE NAM_OMX_UseBuffer(
    OMX_IN OMX_HANDLETYPE            hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE **ppBufferHdr,
    OMX_IN OMX_U32                   nPortIndex,
    OMX_IN OMX_PTR                   pAppPrivate,
    OMX_IN OMX_U32                   nSizeBytes,
    OMX_IN OMX_U8                   *pBuffer);
OMX_ERRORTYPE NAM_OMX_AllocateBuffer(
    OMX_IN OMX_HANDLETYPE            hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE **ppBuffer,
    OMX_IN OMX_U32                   nPortIndex,
    OMX_IN OMX_PTR                   pAppPrivate,
    OMX_IN OMX_U32                   nSizeBytes);
OMX_ERRORTYPE NAM_OMX_FreeBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_U32        nPortIndex,
    OMX_IN OMX_BUFFERHEADERTYPE *pBufferHdr);
OMX_ERRORTYPE NAM_OMX_AllocateTunnelBuffer(
    NAM_OMX_BASEPORT *pOMXBasePort,
    OMX_U32           nPortIndex);
OMX_ERRORTYPE NAM_OMX_FreeTunnelBuffer(
    NAM_OMX_BASEPORT *pOMXBasePort,
    OMX_U32           nPortIndex);
OMX_ERRORTYPE NAM_OMX_ComponentTunnelRequest(
    OMX_IN  OMX_HANDLETYPE hComp,
    OMX_IN OMX_U32         nPort,
    OMX_IN OMX_HANDLETYPE  hTunneledComp,
    OMX_IN OMX_U32         nTunneledPort,
    OMX_INOUT OMX_TUNNELSETUPTYPE *pTunnelSetup);
OMX_ERRORTYPE NAM_OMX_BufferProcess(OMX_HANDLETYPE hComponent);
OMX_ERRORTYPE NAM_OMX_VideoDecodeGetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nParamIndex,
    OMX_INOUT OMX_PTR     ComponentParameterStructure);
OMX_ERRORTYPE NAM_OMX_VideoDecodeSetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        ComponentParameterStructure);
OMX_ERRORTYPE NAM_OMX_VideoDecodeComponentDeinit(OMX_IN OMX_HANDLETYPE hComponent);

#ifdef __cplusplus
}
#endif

#endif

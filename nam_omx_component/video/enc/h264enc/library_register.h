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
 * @file        library_register.h
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version     1.0
 * @history
 *   2010.7.15 : Create
 */

#ifndef NAM_OMX_H264_REG
#define NAM_OMX_H264_REG

#include "NAM_OMX_Def.h"
#include "OMX_Component.h"
#include "NAM_OMX_Component_Register.h"


#define OSCL_EXPORT_REF __attribute__((visibility("default")))
#define MAX_COMPONENT_NUM       1
#define MAX_COMPONENT_ROLE_NUM  1

/* H.264 */
#define NAM_OMX_COMPONENT_H264_ENC "OMX.NAM.AVC.Encoder"
#define NAM_OMX_COMPONENT_H264_ENC_ROLE "video_encoder.avc"


#ifdef __cplusplus
extern "C" {
#endif

OSCL_EXPORT_REF int NAM_OMX_COMPONENT_Library_Register(NAMRegisterComponentType **namComponents);

#ifdef __cplusplus
};
#endif

#endif


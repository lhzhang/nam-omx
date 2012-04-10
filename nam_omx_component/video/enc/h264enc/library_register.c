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
 * @file        library_register.c
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version     1.0
 * @history
 *   2010.7.15 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include "NAM_OSAL_Memory.h"
#include "NAM_OSAL_ETC.h"
#include "library_register.h"
#include "NAM_OSAL_Log.h"


OSCL_EXPORT_REF int NAM_OMX_COMPONENT_Library_Register(NAMRegisterComponentType **namComponents)
{
    FunctionIn();

    if (namComponents == NULL)
        goto EXIT;

    /* component 1 - video decoder H.264 */
    NAM_OSAL_Strcpy(namComponents[0]->componentName, NAM_OMX_COMPONENT_H264_ENC);
    NAM_OSAL_Strcpy(namComponents[0]->roles[0], NAM_OMX_COMPONENT_H264_ENC_ROLE);
    namComponents[0]->totalRoleNum = MAX_COMPONENT_ROLE_NUM;

EXIT:
    FunctionOut();

    return MAX_COMPONENT_NUM;
}


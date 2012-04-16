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
 * @file       NAM_OMX_Component_Register.c
 * @brief      NAM OpenMAX IL Component Register
 * @author     SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version    1.0
 * @history
 *    2010.7.15 : Create
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <assert.h>

#include "OMX_Component.h"
#include "NAM_OSAL_Memory.h"
#include "NAM_OSAL_ETC.h"
#include "NAM_OSAL_Library.h"
#include "NAM_OMX_Component_Register.h"
#include "NAM_OMX_Macros.h"

#undef  NAM_LOG_TAG
#define NAM_LOG_TAG    "NAM_COMP_REGS"
//#define NAM_LOG_OFF
#define NAM_TRACE_ON
#include "NAM_OSAL_Log.h"


#define REGISTRY_FILENAME "namomxregistry"


OMX_ERRORTYPE NAM_OMX_Component_Register(NAM_OMX_COMPONENT_REGLIST **compList, OMX_U32 *compNum)
{
    OMX_ERRORTYPE  ret = OMX_ErrorNone;
    int            componentNum = 0, roleNum = 0, totalCompNum = 0;
    int            read;
    char          *omxregistryfile = NULL;
    char          *line = NULL;
    char          *libName;
    FILE          *omxregistryfp;
    size_t         len;
    OMX_HANDLETYPE soHandle;
    const char    *errorMsg;
    int (*NAM_OMX_COMPONENT_Library_Register)(NAMRegisterComponentType **namComponents);
    NAMRegisterComponentType **namComponentsTemp;
    NAM_OMX_COMPONENT_REGLIST *componentList;

    FunctionIn();

    omxregistryfile = NAM_OSAL_Malloc(strlen("/system/etc/") + strlen(REGISTRY_FILENAME) + 2);
    NAM_OSAL_Strcpy(omxregistryfile, "/system/etc/");
    NAM_OSAL_Strcat(omxregistryfile, REGISTRY_FILENAME);

    omxregistryfp = fopen(omxregistryfile, "r");
    if (omxregistryfp == NULL) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }
    NAM_OSAL_Free(omxregistryfile);

    fseek(omxregistryfp, 0, 0);
    componentList = (NAM_OMX_COMPONENT_REGLIST *)NAM_OSAL_Malloc(sizeof(NAM_OMX_COMPONENT_REGLIST) * MAX_OMX_COMPONENT_NUM);
    NAM_OSAL_Memset(componentList, 0, sizeof(NAM_OMX_COMPONENT_REGLIST) * MAX_OMX_COMPONENT_NUM);
    libName = NAM_OSAL_Malloc(MAX_OMX_COMPONENT_LIBNAME_SIZE);

    while ((read = getline(&line, &len, omxregistryfp)) != -1) {
        if ((*line == 'l') && (*(line + 1) == 'i') && (*(line + 2) == 'b') &&
            (*(line + 3) == 'O') && (*(line + 4) == 'M') && (*(line + 5) == 'X')) {
            NAM_OSAL_Memset(libName, 0, MAX_OMX_COMPONENT_LIBNAME_SIZE);
            NAM_OSAL_Strncpy(libName, line, NAM_OSAL_Strlen(line)-1);
            NAM_OSAL_Log(NAM_LOG_TRACE, "libName : %s", libName);

            if ((soHandle = NAM_OSAL_dlopen(libName, RTLD_NOW)) != NULL) {
                NAM_OSAL_dlerror();    /* clear error*/
                if ((NAM_OMX_COMPONENT_Library_Register = NAM_OSAL_dlsym(soHandle, "NAM_OMX_COMPONENT_Library_Register")) != NULL) {
                    int i = 0, j = 0;

                    componentNum = (*NAM_OMX_COMPONENT_Library_Register)(NULL);
                    namComponentsTemp = (NAMRegisterComponentType **)NAM_OSAL_Malloc(sizeof(NAMRegisterComponentType*) * componentNum);
                    for (i = 0; i < componentNum; i++) {
                        namComponentsTemp[i] = NAM_OSAL_Malloc(sizeof(NAMRegisterComponentType));
                        NAM_OSAL_Memset(namComponentsTemp[i], 0, sizeof(NAMRegisterComponentType));
                    }
                    (*NAM_OMX_COMPONENT_Library_Register)(namComponentsTemp);

                    for (i = 0; i < componentNum; i++) {
                        NAM_OSAL_Strcpy(componentList[totalCompNum].component.componentName, namComponentsTemp[i]->componentName);
                        for (j = 0; j < namComponentsTemp[i]->totalRoleNum; j++)
                            NAM_OSAL_Strcpy(componentList[totalCompNum].component.roles[j], namComponentsTemp[i]->roles[j]);
                        componentList[totalCompNum].component.totalRoleNum = namComponentsTemp[i]->totalRoleNum;

                        NAM_OSAL_Strcpy(componentList[totalCompNum].libName, libName);

                        totalCompNum++;
                    }
                    for (i = 0; i < componentNum; i++) {
                        NAM_OSAL_Free(namComponentsTemp[i]);
                    }

                    NAM_OSAL_Free(namComponentsTemp);
                } else {
                    if ((errorMsg = NAM_OSAL_dlerror()) != NULL)
                        NAM_OSAL_Log(NAM_LOG_WARNING, "dlsym failed: %s", errorMsg);
                }
                NAM_OSAL_dlclose(soHandle);
            } else {
                NAM_OSAL_Log(NAM_LOG_WARNING, "dlopen failed: %s", NAM_OSAL_dlerror());
            }
        } else {
            /* not a component name line. skip */
            continue;
        }
    }

    NAM_OSAL_Free(libName);
    fclose(omxregistryfp);

    *compList = componentList;
    *compNum = totalCompNum;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_OMX_Component_Unregister(NAM_OMX_COMPONENT_REGLIST *componentList)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    NAM_OSAL_Memset(componentList, 0, sizeof(NAM_OMX_COMPONENT_REGLIST) * MAX_OMX_COMPONENT_NUM);
    NAM_OSAL_Free(componentList);

EXIT:
    return ret;
}

OMX_ERRORTYPE NAM_OMX_ComponentAPICheck(OMX_COMPONENTTYPE component)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if ((NULL == component.GetComponentVersion)    ||
        (NULL == component.SendCommand)            ||
        (NULL == component.GetParameter)           ||
        (NULL == component.SetParameter)           ||
        (NULL == component.GetConfig)              ||
        (NULL == component.SetConfig)              ||
        (NULL == component.GetExtensionIndex)      ||
        (NULL == component.GetState)               ||
        (NULL == component.ComponentTunnelRequest) ||
        (NULL == component.UseBuffer)              ||
        (NULL == component.AllocateBuffer)         ||
        (NULL == component.FreeBuffer)             ||
        (NULL == component.EmptyThisBuffer)        ||
        (NULL == component.FillThisBuffer)         ||
        (NULL == component.SetCallbacks)           ||
        (NULL == component.ComponentDeInit)        ||
        (NULL == component.UseEGLImage)            ||
        (NULL == component.ComponentRoleEnum))
        ret = OMX_ErrorInvalidComponent;
    else
        ret = OMX_ErrorNone;

    return ret;
}

OMX_ERRORTYPE NAM_OMX_ComponentLoad(NAM_OMX_COMPONENT *nam_component)
{
    OMX_ERRORTYPE      ret = OMX_ErrorNone;
    OMX_HANDLETYPE     libHandle;
    OMX_COMPONENTTYPE *pOMXComponent;

    FunctionIn();

    OMX_ERRORTYPE (*NAM_OMX_ComponentInit)(OMX_HANDLETYPE hComponent, OMX_STRING componentName);

    libHandle = NAM_OSAL_dlopen(nam_component->libName, RTLD_NOW);
    if (!libHandle) {
        ret = OMX_ErrorInvalidComponentName;
        NAM_OSAL_Log(NAM_LOG_ERROR, "OMX_ErrorInvalidComponentName, Line:%d", __LINE__);
        goto EXIT;
    }

    NAM_OMX_ComponentInit = NAM_OSAL_dlsym(libHandle, "NAM_OMX_ComponentInit");
    if (!NAM_OMX_ComponentInit) {
        NAM_OSAL_dlclose(libHandle);
        ret = OMX_ErrorInvalidComponent;
        NAM_OSAL_Log(NAM_LOG_ERROR, "OMX_ErrorInvalidComponent, Line:%d", __LINE__);
        goto EXIT;
    }

    pOMXComponent = (OMX_COMPONENTTYPE *)NAM_OSAL_Malloc(sizeof(OMX_COMPONENTTYPE));
    INIT_SET_SIZE_VERSION(pOMXComponent, OMX_COMPONENTTYPE);
    ret = (*NAM_OMX_ComponentInit)((OMX_HANDLETYPE)pOMXComponent, nam_component->componentName);
    if (ret != OMX_ErrorNone) {
        NAM_OSAL_Free(pOMXComponent);
        NAM_OSAL_dlclose(libHandle);
        ret = OMX_ErrorInvalidComponent;
        NAM_OSAL_Log(NAM_LOG_ERROR, "OMX_ErrorInvalidComponent, Line:%d", __LINE__);
        goto EXIT;
    } else {
        if (NAM_OMX_ComponentAPICheck(*pOMXComponent) != OMX_ErrorNone) {
            NAM_OSAL_Free(pOMXComponent);
            NAM_OSAL_dlclose(libHandle);
            if (NULL != pOMXComponent->ComponentDeInit)
                pOMXComponent->ComponentDeInit(pOMXComponent);
            ret = OMX_ErrorInvalidComponent;
            NAM_OSAL_Log(NAM_LOG_ERROR, "OMX_ErrorInvalidComponent, Line:%d", __LINE__);
            goto EXIT;
        }
        nam_component->libHandle = libHandle;
        nam_component->pOMXComponent = pOMXComponent;
        ret = OMX_ErrorNone;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_OMX_ComponentUnload(NAM_OMX_COMPONENT *nam_component)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pOMXComponent = NULL;

    FunctionIn();

    if (!nam_component) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pOMXComponent = nam_component->pOMXComponent;
    if (pOMXComponent != NULL) {
        pOMXComponent->ComponentDeInit(pOMXComponent);
        NAM_OSAL_Free(pOMXComponent);
        nam_component->pOMXComponent = NULL;
    }

    if (nam_component->libHandle != NULL) {
        NAM_OSAL_dlclose(nam_component->libHandle);
        nam_component->libHandle = NULL;
    }

EXIT:
    FunctionOut();

    return ret;
}

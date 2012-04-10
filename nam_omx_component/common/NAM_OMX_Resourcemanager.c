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
 * @file       NAM_OMX_Resourcemanager.c
 * @brief
 * @author     SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version    1.0
 * @history
 *    2010.7.15 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "NAM_OMX_Resourcemanager.h"
#include "NAM_OMX_Banamomponent.h"

#undef  NAM_LOG_TAG
#define NAM_LOG_TAG    "NAM_RM"
#define NAM_LOG_OFF
#include "NAM_OSAL_Log.h"


#define MAX_RESOURCE_VIDEO 4

/* Max allowable video scheduler component instance */
static NAM_OMX_RM_COMPONENT_LIST *gpVideoRMComponentList = NULL;
static NAM_OMX_RM_COMPONENT_LIST *gpVideoRMWaitingList = NULL;
static OMX_HANDLETYPE ghVideoRMComponentListMutex = NULL;


OMX_ERRORTYPE addElementList(NAM_OMX_RM_COMPONENT_LIST **ppList, OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE              ret = OMX_ErrorNone;
    NAM_OMX_RM_COMPONENT_LIST *pTempComp = NULL;
    NAM_OMX_BANAMOMPONENT     *pNAMComponent = NULL;

    pNAMComponent = (NAM_OMX_BANAMOMPONENT *)pOMXComponent->pComponentPrivate;
    if (*ppList != NULL) {
        pTempComp = *ppList;
        while (pTempComp->pNext != NULL) {
            pTempComp = pTempComp->pNext;
        }
        pTempComp->pNext = (NAM_OMX_RM_COMPONENT_LIST *)NAM_OSAL_Malloc(sizeof(NAM_OMX_RM_COMPONENT_LIST));
        if (pTempComp->pNext == NULL) {
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
        ((NAM_OMX_RM_COMPONENT_LIST *)(pTempComp->pNext))->pNext = NULL;
        ((NAM_OMX_RM_COMPONENT_LIST *)(pTempComp->pNext))->pOMXStandComp = pOMXComponent;
        ((NAM_OMX_RM_COMPONENT_LIST *)(pTempComp->pNext))->groupPriority = pNAMComponent->compPriority.nGroupPriority;
        goto EXIT;
    } else {
        *ppList = (NAM_OMX_RM_COMPONENT_LIST *)NAM_OSAL_Malloc(sizeof(NAM_OMX_RM_COMPONENT_LIST));
        if (*ppList == NULL) {
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
        pTempComp = *ppList;
        pTempComp->pNext = NULL;
        pTempComp->pOMXStandComp = pOMXComponent;
        pTempComp->groupPriority = pNAMComponent->compPriority.nGroupPriority;
    }

EXIT:
    return ret;
}

OMX_ERRORTYPE removeElementList(NAM_OMX_RM_COMPONENT_LIST **ppList, OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE              ret = OMX_ErrorNone;
    NAM_OMX_RM_COMPONENT_LIST *pCurrComp = NULL;
    NAM_OMX_RM_COMPONENT_LIST *pPrevComp = NULL;
    OMX_BOOL                   bDetectComp = OMX_FALSE;

    if (*ppList == NULL) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

    pCurrComp = *ppList;
    while (pCurrComp != NULL) {
        if (pCurrComp->pOMXStandComp == pOMXComponent) {
            if (*ppList == pCurrComp) {
                *ppList = pCurrComp->pNext;
                NAM_OSAL_Free(pCurrComp);
            } else {
                pPrevComp->pNext = pCurrComp->pNext;
                NAM_OSAL_Free(pCurrComp);
            }
            bDetectComp = OMX_TRUE;
            break;
        } else {
            pPrevComp = pCurrComp;
            pCurrComp = pCurrComp->pNext;
        }
    }

    if (bDetectComp == OMX_FALSE)
        ret = OMX_ErrorComponentNotFound;
    else
        ret = OMX_ErrorNone;

EXIT:
    return ret;
}

int searchLowPriority(NAM_OMX_RM_COMPONENT_LIST *RMComp_list, int inComp_priority, NAM_OMX_RM_COMPONENT_LIST **outLowComp)
{
    int ret = 0;
    NAM_OMX_RM_COMPONENT_LIST *pTempComp = NULL;
    NAM_OMX_RM_COMPONENT_LIST *pCandidateComp = NULL;

    if (RMComp_list == NULL)
        ret = -1;

    pTempComp = RMComp_list;
    *outLowComp = 0;

    while (pTempComp != NULL) {
        if (pTempComp->groupPriority > inComp_priority) {
            if (pCandidateComp != NULL) {
                if (pCandidateComp->groupPriority < pTempComp->groupPriority)
                    pCandidateComp = pTempComp;
            } else {
                pCandidateComp = pTempComp;
            }
        }

        pTempComp = pTempComp->pNext;
    }

    *outLowComp = pCandidateComp;
    if (pCandidateComp == NULL)
        ret = 0;
    else
        ret = 1;

EXIT:
    return ret;
}

OMX_ERRORTYPE removeComponent(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    NAM_OMX_BANAMOMPONENT *pNAMComponent = NULL;

    pNAMComponent = (NAM_OMX_BANAMOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pNAMComponent->currentState == OMX_StateIdle) {
        (*(pNAMComponent->pCallbacks->EventHandler))
            (pOMXComponent, pNAMComponent->callbackData,
            OMX_EventError, OMX_ErrorResourcesLost, 0, NULL);
        ret = OMX_SendCommand(pOMXComponent, OMX_CommandStateSet, OMX_StateLoaded, NULL);
        if (ret != OMX_ErrorNone) {
            ret = OMX_ErrorUndefined;
            goto EXIT;
        }
    } else if ((pNAMComponent->currentState == OMX_StateExecuting) || (pNAMComponent->currentState == OMX_StatePause)) {
        /* Todo */
    }

    ret = OMX_ErrorNone;

EXIT:
    return ret;
}


OMX_ERRORTYPE NAM_OMX_ResourceManager_Init()
{
    FunctionIn();
    NAM_OSAL_MutexCreate(&ghVideoRMComponentListMutex);
    FunctionOut();
    return OMX_ErrorNone;
}

OMX_ERRORTYPE NAM_OMX_ResourceManager_Deinit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    NAM_OMX_RM_COMPONENT_LIST *pCurrComponent;
    NAM_OMX_RM_COMPONENT_LIST *pNextComponent;

    FunctionIn();

    NAM_OSAL_MutexLock(ghVideoRMComponentListMutex);

    if (gpVideoRMComponentList) {
        pCurrComponent = gpVideoRMComponentList;
        while (pCurrComponent != NULL) {
            pNextComponent = pCurrComponent->pNext;
            NAM_OSAL_Free(pCurrComponent);
            pCurrComponent = pNextComponent;
        }
        gpVideoRMComponentList = NULL;
    }

    if (gpVideoRMWaitingList) {
        pCurrComponent = gpVideoRMWaitingList;
        while (pCurrComponent != NULL) {
            pNextComponent = pCurrComponent->pNext;
            NAM_OSAL_Free(pCurrComponent);
            pCurrComponent = pNextComponent;
        }
        gpVideoRMWaitingList = NULL;
    }
    NAM_OSAL_MutexUnlock(ghVideoRMComponentListMutex);

    NAM_OSAL_MutexTerminate(ghVideoRMComponentListMutex);
    ghVideoRMComponentListMutex = NULL;

    ret = OMX_ErrorNone;
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_OMX_Get_Resource(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE              ret = OMX_ErrorNone;
    NAM_OMX_BANAMOMPONENT     *pNAMComponent = NULL;
    NAM_OMX_RM_COMPONENT_LIST *pComponentTemp = NULL;
    NAM_OMX_RM_COMPONENT_LIST *pComponentCandidate = NULL;
    int numElem = 0;
    int lowCompDetect = 0;

    FunctionIn();

    NAM_OSAL_MutexLock(ghVideoRMComponentListMutex);

    pNAMComponent = (NAM_OMX_BANAMOMPONENT *)pOMXComponent->pComponentPrivate;
    pComponentTemp = gpVideoRMComponentList;
    if (pNAMComponent->codecType == HW_VIDEO_CODEC) {
        if (pComponentTemp != NULL) {
            while (pComponentTemp) {
                numElem++;
                pComponentTemp = pComponentTemp->pNext;
            }
        } else {
            numElem = 0;
        }
        if (numElem >= MAX_RESOURCE_VIDEO) {
            lowCompDetect = searchLowPriority(gpVideoRMComponentList, pNAMComponent->compPriority.nGroupPriority, &pComponentCandidate);
            if (lowCompDetect <= 0) {
                ret = OMX_ErrorInsufficientResources;
                goto EXIT;
            } else {
                ret = removeComponent(pComponentCandidate->pOMXStandComp);
                if (ret != OMX_ErrorNone) {
                    ret = OMX_ErrorInsufficientResources;
                    goto EXIT;
                } else {
                    ret = removeElementList(&gpVideoRMComponentList, pComponentCandidate->pOMXStandComp);
                    ret = addElementList(&gpVideoRMComponentList, pOMXComponent);
                    if (ret != OMX_ErrorNone) {
                        ret = OMX_ErrorInsufficientResources;
                        goto EXIT;
                    }
                }
            }
        } else {
            ret = addElementList(&gpVideoRMComponentList, pOMXComponent);
            if (ret != OMX_ErrorNone) {
                ret = OMX_ErrorInsufficientResources;
                goto EXIT;
            }
        }
    }
    ret = OMX_ErrorNone;

EXIT:

    NAM_OSAL_MutexUnlock(ghVideoRMComponentListMutex);

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_OMX_Release_Resource(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE              ret = OMX_ErrorNone;
    NAM_OMX_BANAMOMPONENT     *pNAMComponent = NULL;
    NAM_OMX_RM_COMPONENT_LIST *pComponentTemp = NULL;
    OMX_COMPONENTTYPE         *pOMXWaitComponent = NULL;
    int numElem = 0;

    FunctionIn();

    NAM_OSAL_MutexLock(ghVideoRMComponentListMutex);

    pNAMComponent = (NAM_OMX_BANAMOMPONENT *)pOMXComponent->pComponentPrivate;
    pComponentTemp = gpVideoRMWaitingList;
    if (pNAMComponent->codecType == HW_VIDEO_CODEC) {
        if (gpVideoRMComponentList == NULL) {
            ret = OMX_ErrorUndefined;
            goto EXIT;
        }

        ret = removeElementList(&gpVideoRMComponentList, pOMXComponent);
        if (ret != OMX_ErrorNone) {
            ret = OMX_ErrorUndefined;
            goto EXIT;
        }
        while (pComponentTemp) {
            numElem++;
            pComponentTemp = pComponentTemp->pNext;
        }
        if (numElem > 0) {
            pOMXWaitComponent = gpVideoRMWaitingList->pOMXStandComp;
            removeElementList(&gpVideoRMWaitingList, pOMXWaitComponent);
            ret = OMX_SendCommand(pOMXWaitComponent, OMX_CommandStateSet, OMX_StateIdle, NULL);
            if (ret != OMX_ErrorNone) {
                goto EXIT;
            }
        }
    }

EXIT:

    NAM_OSAL_MutexUnlock(ghVideoRMComponentListMutex);

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_OMX_In_WaitForResource(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    NAM_OMX_BANAMOMPONENT *pNAMComponent = NULL;

    FunctionIn();

    NAM_OSAL_MutexLock(ghVideoRMComponentListMutex);

    pNAMComponent = (NAM_OMX_BANAMOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pNAMComponent->codecType == HW_VIDEO_CODEC)
        ret = addElementList(&gpVideoRMWaitingList, pOMXComponent);

    NAM_OSAL_MutexUnlock(ghVideoRMComponentListMutex);

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE NAM_OMX_Out_WaitForResource(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    NAM_OMX_BANAMOMPONENT *pNAMComponent = NULL;

    FunctionIn();

    NAM_OSAL_MutexLock(ghVideoRMComponentListMutex);

    pNAMComponent = (NAM_OMX_BANAMOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pNAMComponent->codecType == HW_VIDEO_CODEC)
        ret = removeElementList(&gpVideoRMWaitingList, pOMXComponent);

    NAM_OSAL_MutexUnlock(ghVideoRMComponentListMutex);

    FunctionOut();

    return ret;
}


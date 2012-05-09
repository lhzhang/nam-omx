/*******************************************************************************
 * iface.cpp
 *
 * Simple interface for Android mediaserver to open Codec Engine.  
 *
 * Copyright (C) 2010 Alexander Smirnov <asmirnov.bluesman@gmail.com>
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
 ******************************************************************************/

#include "ti_dmai_iface.h"
#include <ti/sdo/dmai/ce/Adec1.h>

#include <ti/sdo/ce/osal/Memory.h>
#include <ti/sdo/ce/osal/Lock.h>
#include <ti/sdo/ce/ipc/Comm.h>
#include <ti/sdo/ce/ipc/Processor.h>
#include <ti/sdo/ce/osal/Queue.h>
#include <ti/sdo/ce/osal/Global.h>

#include "NAM_OSAL_Mutex.h"

#undef  NAM_LOG_TAG
#define NAM_LOG_TAG    "NAM_BASE_PORT"
//#define NAM_LOG_OFF
#define NAM_TRACE_ON
#include "NAM_OSAL_Log.h"

#define HAVE_ENGINE_MUTEX 0

#if HAVE_ENGINE_MUTEX
static OMX_HANDLETYPE gEngineHandleMutex = NULL;
#endif

static Engine_Handle global_engine_handle = NULL;
static int refcount = 0;

#if HAVE_ENGINE_MUTEX
void TIDmaiHandleInit()
{
    FunctionIn();

    NAM_OSAL_MutexCreate(&gEngineHandleMutex);
    global_engine_handle = NULL;
    refcount = 0;

    FunctionOut();
}

void TIDmaiHandleDeinit()
{
    FunctionIn();

    NAM_OSAL_MutexTerminate(gEngineHandleMutex);
    gEngineHandleMutex = NULL;

    FunctionOut();
}

#else

void TIDmaiHandleInit() {}
void TIDmaiHandleDeinit() {}

#endif

Engine_Handle TIDmaiDetHandle()
{
    Engine_Error        ec;

    FunctionIn();

#if HAVE_ENGINE_MUTEX
    NAM_OSAL_Mutexlock(gEngineHandleMutex);
#endif

    if (!refcount) {
        /* ...initialize codec engine runtime */
        CERuntime_init ();

        /* ...initialize DMAI framework */
        Dmai_init ();

        if ((global_engine_handle = Engine_open("codecServer", NULL, &ec)) == NULL) {
            NAM_OSAL_Log(NAM_LOG_ERROR, "Failed to open engine 'codecServer': %X", (unsigned) ec);
            CERuntime_exit ();
        }
    }
    refcount++;

#if HAVE_ENGINE_MUTEX
    NAM_OSAL_MutexUnlock(gEngineHandleMutex);
#endif

    FunctionOut();

    return global_engine_handle;

}

/*
 * Currently there is no method to call this function. No
 * exit signals or callbacks from the mediaserver weren't found. But the call is important 
 * becasuse if mediaserver crashes, codec server cannot be re-opened without
 * closing.
 */
void TIDMmaiFreeHandle(Engine_Handle h)
{
    FunctionIn();

#if HAVE_ENGINE_MUTEX
    NAM_OSAL_Mutexlock(gEngineHandleMutex);
#endif

    NAM_OSAL_Log(NAM_LOG_TRACE, "TIDMmaiFreeHandle refcount: %d", refcount);

    if (refcount > 0 && h == global_engine_handle) {
        refcount--;
        if (!refcount && global_engine_handle) {
            Engine_close(global_engine_handle);
            global_engine_handle = NULL;
            CERuntime_exit ();
        }
    }

#if HAVE_ENGINE_MUTEX
    NAM_OSAL_MutexUnlock(gEngineHandleMutex);
#endif

    FunctionOut();
}


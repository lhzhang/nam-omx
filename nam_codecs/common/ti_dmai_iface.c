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

#include <utils/Log.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#define LOGGER_LOG_MAIN         "log/main"
#define LOGGER_LOG_RADIO        "log/radio"
#define LOGGER_LOG_EVENTS       "log/events"
#define LOGGER_LOG_SYSTEM       "log/system"
#define LOG_BUF_SIZE             1024

#undef  NAM_LOG_TAG
#define NAM_LOG_TAG    "NAM_IFACE"
//#define NAM_LOG_OFF
#define NAM_TRACE_ON
#include "NAM_OSAL_Log.h"

#define HAVE_ENGINE_MUTEX 0

#define TI_TRACE_FD_UNINIT  -2
#define TI_TRACE_FD_INVALID -1

static int g_ti_trace_fd = TI_TRACE_FD_UNINIT;
static char g_msg[LOG_BUF_SIZE];
static int g_msg_len = 0;

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

#endif // HAVE_ENGINE_MUTEX

static int nam_ti_log_init()
{
    if (g_ti_trace_fd == TI_TRACE_FD_UNINIT) {
        g_ti_trace_fd = open("/dev/"LOGGER_LOG_MAIN, O_WRONLY);
        if (g_ti_trace_fd < 0) {
            NAM_OSAL_Log(NAM_LOG_ERROR, "Failed to open g_ti_trace_fd");
            close(g_ti_trace_fd);
            g_ti_trace_fd = TI_TRACE_FD_INVALID;
            return  -1;
        } else {
            NAM_OSAL_Log(NAM_LOG_TRACE, "Successfully open g_ti_trace_fd: %d", g_ti_trace_fd);
        }
    } else {
        NAM_OSAL_Log(NAM_LOG_TRACE, "Open g_ti_trace_fd again, g_ti_trace_fd: %d", g_ti_trace_fd);
    }

    return 0;
}

static void nam_ti_log_deinit()
{
    if (g_ti_trace_fd > 0)
        close(g_ti_trace_fd);
}

static inline int nam_ti_log(const char *fmt, ...)
{
    int ret = 0;
    int saw_lf;
    int check_len;
    va_list ap;
    char buf[LOG_BUF_SIZE];
    struct iovec vec[3];

    if(g_ti_trace_fd > 0) {
        va_start(ap, fmt);
        vsnprintf(buf, LOG_BUF_SIZE, fmt, ap);
        va_end(ap);

        do {
            check_len = g_msg_len + strlen(buf) + 1;
            if (check_len <= LOG_BUF_SIZE) {
                /* lf: Line feed ('\n') */
                saw_lf = (strchr(buf, '\n') != NULL) ? 1 : 0;
                strncpy(g_msg + g_msg_len, buf, strlen(buf));
                g_msg_len += strlen(buf);
                if (!saw_lf) {
                   /* skip */
                   return 0;
                } else {
                   /* add line feed */
                   g_msg_len += 1;
                   g_msg[g_msg_len] = '\n';
                }
            } else {
                /* trace is fragmented */
                g_msg_len += 1;
                g_msg[g_msg_len] = '\n';
            }
#if 0
            return __android_log_write(ANDROID_LOG_INFO, LOG_TAG, g_msg);
#else
            int prio = ANDROID_LOG_INFO;
            const char *tag = "NAM_DSP";

            vec[0].iov_base   = (unsigned char *) &prio;
            vec[0].iov_len    = 1;
            vec[1].iov_base   = (void *) tag;
            vec[1].iov_len    = strlen(tag) + 1;
            vec[2].iov_base   = (void *) g_msg;
            vec[2].iov_len    = g_msg_len;

            do {
                ret = writev(g_ti_trace_fd, vec, 3);
            } while (ret < 0 && errno == EINTR);
#endif
            // reset g_msg_len
            g_msg_len = 0;
         } while (check_len > LOG_BUF_SIZE);
    }

    return ret;
}

Engine_Handle TIDmaiDetHandle()
{
    Engine_Handle	hEngine = NULL;
    Engine_Error        ec;

    FunctionIn();

#if HAVE_ENGINE_MUTEX
    NAM_OSAL_Mutexlock(gEngineHandleMutex);
#endif

    if (!refcount) {
        nam_ti_log_init();

        /* initialize codec engine runtime */
        CERuntime_init();

        /* init trace */
        //GT_init();

        /* initialize DMAI framework */
        Dmai_init();

        //Dmai_setLogLevel(Dmai_LogLevel_All);

        /* Set printf function for GT */
        GT_setprintf( (GT_PrintFxn)nam_ti_log);

	/* Enable tracing in all modules */
	GT_set("*=01234567");

        if ((global_engine_handle = Engine_open("codecServer", NULL, &ec)) == NULL) {
            NAM_OSAL_Log(NAM_LOG_ERROR, "Failed to open engine 'codecServer': %X", (unsigned) ec);
            CERuntime_exit ();
            hEngine = NULL;
            goto EXIT;
        }
        hEngine = global_engine_handle;
        
    }
    refcount++;

#if HAVE_ENGINE_MUTEX
    NAM_OSAL_MutexUnlock(gEngineHandleMutex);
#endif

EXIT:
    FunctionOut();

    return hEngine;

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
            nam_ti_log_deinit();
        }
    }

#if HAVE_ENGINE_MUTEX
    NAM_OSAL_MutexUnlock(gEngineHandleMutex);
#endif

    FunctionOut();
}


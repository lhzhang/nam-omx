/*******************************************************************************
 * ticodec.cpp
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

#include "ticodec.h"

void ticodec()
{
    Dmai_init();
    Cpu_getDevice(NULL, NULL);
    Vdec2_create(NULL, NULL, NULL, NULL);
    Vdec2_getInBufSize(NULL);
    Buffer_create(0, NULL);
    BufTab_create(0, 0, NULL);
    Buffer_getUserPtr(NULL);
    Vdec2_getOutBufSize(NULL);
    BufferGfx_calcLineLength(0, (ColorSpace_Type)0);
    BufferGfx_getBufferAttrs(NULL);
    Vdec2_setBufTab(NULL, NULL);
    Vdec2_delete(NULL);
    Buffer_delete(NULL);
    BufTab_delete(NULL);
    BufTab_freeAll(NULL);
    BufTab_collapse(NULL);
    Vdec2_process(NULL, NULL, NULL);
}

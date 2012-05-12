/*******************************************************************************
 * ticodec.h
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

#ifndef __TICODEC_H
#define __TICODEC_H

/* DMAI headers */
#include <xdc/std.h>
#include <ti/sdo/ce/Engine.h>
#include <ti/sdo/ce/CERuntime.h>
#include <ti/sdo/dmai/Dmai.h>
#include <ti/sdo/dmai/ce/Vdec2.h>
#include <ti/sdo/dmai/VideoStd.h>
#include <ti/sdo/dmai/Buffer.h>
#include <ti/sdo/dmai/BufTab.h>
#include <ti/sdo/dmai/BufferGfx.h>
#include <ti/sdo/dmai/Display.h>
#include <ti/sdo/dmai/Cpu.h>

/* For compiling/linking only */
void ticodec();


#endif /* __TICODEC_H */

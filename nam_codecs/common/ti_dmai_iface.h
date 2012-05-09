/*******************************************************************************
 * iface.h
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

#ifndef __TI_DMAI_IFACE_H
#define __TI_DMAI_IFACE_H

#include <xdc/std.h>
#include <ti/sdo/ce/Engine.h>
#include <ti/sdo/ce/CERuntime.h>
#include <ti/sdo/dmai/Dmai.h>

void TIDmaiHandleInit();
void TIDmaiHandleDeinit();
Engine_Handle TIDmaiDetHandle(void);
void TIDMmaiFreeHandle(Engine_Handle h);

#endif /* __TI_DMAI_IFACE_H */

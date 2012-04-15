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

#ifndef __IFACE_H
#define __IFACE_H

#include <xdc/std.h>
#include <ti/sdo/ce/Engine.h>
#include <ti/sdo/ce/CERuntime.h>
#include <ti/sdo/dmai/Dmai.h>

class InterfaceClass
{
public:
    InterfaceClass();
    ~InterfaceClass();

    Engine_Handle GetDSPHandle(void);
    void          FreeDSPHandle(Engine_Handle h);

    /* For testing/debugging only, to-be-removed */
    void           Adec(void);
};

#endif /* __IFACE_H */

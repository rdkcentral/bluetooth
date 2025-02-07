/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/
/* 
 * @file agentHelper.h
 * Includes information for gatt server agent setup over BT
*/

#ifndef __AGENTHELPER_H__
#define __AGENTHELPER_H__

#include <gio/gio.h>
#include "gdbusBluez.h"

struct agentMethodCb
{
    int (*reqConfirmation)(const gchar *, guint ui32PassCode, void *);
    int (*reqAuth)(const gchar *, void *);
    int (*reqPasskey)(const gchar *, void *);
};

/**
 * Register a BlueZ agent by acquiring the bus name.
 * @note this is called when bleconfd app created and bus acquired.
 * @param adapterPath
 * @param agentPath
 * @param capability The capability parameter can have the values
 *                   "DisplayOnly", "DisplayYesNo", "KeyboardOnly",
 *                   "NoInputNoOutput" and "KeyboardDisplay" which
 *                   reflects the input and output capabilities of the agent.
 * @param            callback for agent methods
 * @param
 */
void bluezAgentHelperRegister(const char *adapterPath, const char *agentPath, const char *capability, struct agentMethodCb *, void *);

void bluezAgentHelperUnregister(const char *adapterPath, const char *agentPath);

#endif // end of __AGENTHELPER_H__

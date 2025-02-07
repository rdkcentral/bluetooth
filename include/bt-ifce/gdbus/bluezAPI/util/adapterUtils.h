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
 * @file adapterUtils.h
 * Includes information for adapter over BT
*/

#include <gdbusBluez.h>

#ifndef __ADAPTERUTILS_H__
#define __ADAPTERUTILS_H__

/**
 * Get a proxy for the default Adapter1 interface. A Non-NULL response guarantees
 * the BlueZ stack and D-Bus are operational, and that BlueZ accepts this application's messages.
 * @return [nullable] an adapter proxy to be used with bluez_adapter1... APIs.
 * When finished, use g_object_unref() or declare as a g_autoptr(BluezAdapter1)
 */
BluezAdapter1 *bluezAdapterUtilsGetDefault(void);

/**
 * Get the controller index for an adapter
 * @param adapter
 * @return A non-negative integer on success, or -1 on failure
 */
int8_t bluezAdapterUtilsGetIndex(BluezAdapter1 *adapter);

/**
 * Get the object path for an adapter
 * @param adapter
 * @return The object path. This is only valid while the adapter object is alive.
 */
inline const char *bluezAdapterUtilsGetObjectPath(BluezAdapter1 *adapter)
{
    if (adapter == NULL)
    {
        return NULL;
    }

    return g_dbus_proxy_get_object_path(G_DBUS_PROXY(adapter));
}

#endif //end of __ADAPTERUTILS_H__

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
 * adapterUtiles.c
 * Implementation of Bluetooth Adapter functions
*/


#include "bluezAPICommon.h"
#define G_LOG_DOMAIN _LOG_DOMAIN "-adapterUtils"

#include <dbusHelper/dbusUtil.h>
#include <gio/gio.h>
#include <glib.h>
#include <pthread.h>
#include <bluez-dbus.h>
#include <inttypes.h>
#include "util/adapterUtils.h"
#include "gdbusBluez.h"

/**
 * Create a proxy for a named adapter.
 * @param name A valid adapter name (e.g., hci0)
 * @see bluez_object_manager* API for BlueZ managed objects //TODO: provide getAdapters API
 * @return [nullable] A BluezAdapter1
 */
static BluezAdapter1 *createAdapter1(const char *name)
{
    g_autoptr(GError) error = NULL;
    g_autofree char *adapterPath = g_strdup_printf(BLUEZ_BUS_PATH "/%s", name);

    BluezAdapter1 *proxy = bluez_adapter1_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                                                 G_DBUS_PROXY_FLAGS_NONE,
                                                                 BLUEZ_BUS_NAME, /* bus name */
                                                                 adapterPath,    /* object path */
                                                                 NULL,           /* cancellable */
                                                                 &error);

    if (proxy == NULL)
    {
        g_warning("can not get BlueZ Adapter1: %s", error->message);
    }

    return proxy;
}

BluezAdapter1 *bluezAdapterUtilsGetDefault(void)
{
    BluezAdapter1 *proxy = NULL;

    GSList *adapters = dbusUtilFindSubpaths(BLUEZ_BUS_NAME, BLUEZ_BUS_PATH);

    if (adapters == NULL)
    {
        return NULL;
    }

    const char *adapterName = adapters->data;

    if (adapterName != NULL)
    {
        proxy = createAdapter1(adapterName);
    }

    g_slist_free_full(adapters, free);

    return proxy;
}

int8_t bluezAdapterUtilsGetIndex(BluezAdapter1 *adapter)
{
    int8_t index = -1;
    const gchar *objectPath = g_dbus_proxy_get_object_path(G_DBUS_PROXY(adapter));

    if (objectPath != NULL)
    {
        size_t prefixLen = strlen("/org/bluez/hci");

        if (g_str_has_prefix(objectPath, "/org/bluez/hci") && strlen(objectPath) > prefixLen)
        {
            guint64 tmp;
            g_autoptr(GError) error = NULL;

            if (g_ascii_string_to_unsigned(objectPath + prefixLen, 10, 0, INT8_MAX, &tmp, &error))
            {
                index = (int8_t) tmp;
            }
            else
            {
                g_warning("Can not convert '%s' to index: %s", objectPath, error->message);
            }
        }
    }

    return index;
}

extern inline const char *bluezAdapterUtilsGetObjectPath(BluezAdapter1 *adapter);

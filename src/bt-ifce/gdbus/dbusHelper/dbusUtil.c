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
 * deviceinfo.c
 * Implementation of Gatt server over Bluetooth
*/

#include "dbusCommon.h"

#define G_LOG_DOMAIN _LOG_DOMAIN "-util"

#include <glib.h>
#include <gio/gio.h>
#include <gdbusFreedesktop.h>

GDBusNodeInfo *dbusUtilIntrospect(const char *busName, const char *objectPath)
{
    GError *error = NULL;
    g_autoptr(DBusIntrospectable) introspector = dbus_introspectable_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                                                                            G_DBUS_PROXY_FLAGS_NONE,
                                                                                            busName,        /* bus name */
                                                                                            objectPath,     /* object path */
                                                                                            NULL,           /* cancellable */
                                                                                            &error);
    g_autofree gchar *xml = NULL;

    gboolean ok = dbus_introspectable_call_introspect_sync(introspector,
                                                           &xml,
                                                           NULL,
                                                           &error);

    GDBusNodeInfo *nodeInfo = NULL;

    if (ok == TRUE)
    {
        g_clear_error(&error);

        nodeInfo = g_dbus_node_info_new_for_xml(xml, &error);
        if (nodeInfo == NULL)
        {
            g_critical("Unable to parse introspection XML: %s", error->message);
        }
    }

    g_clear_error(&error);

    return nodeInfo;
}

GSList *dbusUtilFindSubpaths(const char *busName, const char *objectPath)
{
    g_autoptr(GDBusNodeInfo) nodeInfo = dbusUtilIntrospect(busName, objectPath);
    GSList *paths = NULL;

    if (nodeInfo != NULL)
    {
        for (size_t i = 0;
             nodeInfo->nodes != NULL && nodeInfo->nodes[i] != NULL && nodeInfo->nodes[i]->path != NULL;
             i++)
        {
            paths = g_slist_append(paths, g_strdup(nodeInfo->nodes[i]->path));
        }
    }

    return paths;
}

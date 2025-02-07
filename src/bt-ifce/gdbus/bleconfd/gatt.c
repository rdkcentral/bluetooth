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
 * gatt.c
 * Implementation of Gatt server & char setup over Bluetooth
*/

#include "gatt.h"

#include <glib.h>
#include <glib/gprintf.h>

static GattCharSerObj *CharIfce;
BluezGattCharacteristic1 *gatt_characteristic_new(const gchar *service_path, gatt_characteristic_desc_t *desc)
{
    BluezGattCharacteristic1 *iface = bluez_gatt_characteristic1_skeleton_new();
    bluez_gatt_characteristic1_set_uuid(iface, desc->uuid);
    bluez_gatt_characteristic1_set_flags(iface, desc->flags);
    bluez_gatt_characteristic1_set_handle(iface, 0x0000);
    bluez_gatt_characteristic1_set_notifying(iface, desc->notifying);
    bluez_gatt_characteristic1_set_service(iface, service_path);

    if (desc->on_write)
        g_signal_connect(iface, "handle-write-value", G_CALLBACK(desc->on_write), NULL);

    if (desc->on_read)
        g_signal_connect(iface, "handle-read-value", G_CALLBACK(desc->on_read), NULL);

    if (desc->on_notify_start)
        g_signal_connect(iface, "handle-start-notify", G_CALLBACK(desc->on_notify_start), NULL);

    if (desc->on_notify_stop)
        g_signal_connect(iface, "handle-stop-notify", G_CALLBACK(desc->on_notify_stop), NULL);

    return iface;
}

BluezObjectSkeleton *gatt_characteristic_export(GDBusObjectManagerServer *object_manager,
                                                const gchar *service_path, gatt_characteristic_desc_t *desc)
{
    BluezGattCharacteristic1 *charObj = gatt_characteristic_new(service_path, desc);

    gchar char_object_path[BT_MAX_GATT_CHAR_STR_LEN] = {};
    if(desc->is_bleAdvTest1 == TRUE) { 
       g_sprintf(char_object_path, "%s/chars/%s", service_path, desc->name);
    }
    else {
       g_sprintf(char_object_path, "%s", desc->name);
    }

    BluezObjectSkeleton *skeleton = bluez_object_skeleton_new(char_object_path);
    bluez_object_skeleton_set_gatt_characteristic1(skeleton, charObj);
    g_dbus_object_manager_server_export(object_manager, G_DBUS_OBJECT_SKELETON(skeleton));

    const gchar *path = g_dbus_object_get_object_path(G_DBUS_OBJECT(skeleton));
    g_info("Export characteristic:%s", path);

    if (desc->value)
    {
        g_info("Export characteristic value:%s", desc->value);
        bluez_gatt_characteristic1_set_value(charObj, g_variant_new_bytestring(desc->value));
    }

    if (CharIfce->CharObjCount < MAX_GATT_CHAR_SERVER_OBJ) {
        CharIfce->CharObj[CharIfce->CharObjCount] = charObj;
        strncpy(CharIfce->CharObjPath[CharIfce->CharObjCount],char_object_path, BT_MAX_GATT_CHAR_STR_LEN - 1);
        CharIfce->CharObjCount++;
    } else {
        g_info ("Maximum limit of server characteristic info reached ...\n");
    }

    // TODO: does caller need to unref this?
    return skeleton;
}

BluezObjectSkeleton *gatt_service_export(GDBusObjectManagerServer *object_manager,
                                         const gchar *object_path_root, gatt_service_desc_t *desc)
{
    gchar service_object_path[256];
    if(desc->is_bleAdvTest == TRUE) {
       g_sprintf(service_object_path, "%s/services/%s", object_path_root, desc->name);
    }
    else {
       g_sprintf(service_object_path, "%s", desc->name);
    }
    BluezGattService1 *service_interface = bluez_gatt_service1_skeleton_new();
    bluez_gatt_service1_set_uuid(service_interface, desc->uuid);
    bluez_gatt_service1_set_handle(service_interface, 0x0000);
    bluez_gatt_service1_set_primary(service_interface, desc->is_primary);

    BluezObjectSkeleton *skeleton = bluez_object_skeleton_new(service_object_path);
    bluez_object_skeleton_set_gatt_service1(skeleton, service_interface);
    g_dbus_object_manager_server_export(object_manager, G_DBUS_OBJECT_SKELETON(skeleton));

    const gchar *path = g_dbus_object_get_object_path(G_DBUS_OBJECT(skeleton));
    g_info("Export service:%s", path);

    // TODO: does caller need to unref this?
    return skeleton;
}

BluezObjectSkeleton *gatt_service_build_and_export(
    GDBusObjectManagerServer *object_manager,
    const char *root_path,
    gatt_service_desc_t *service_desc,
    gatt_characteristic_desc_t *listof_chars)
{
    BluezObjectSkeleton *service = gatt_service_export(object_manager, root_path, service_desc);

    gchar const *service_object_path = g_dbus_object_get_object_path(G_DBUS_OBJECT(service));

    int i = 0;
    while (listof_chars[i].uuid != NULL)
    {
        gatt_characteristic_desc_t *char_desc = &listof_chars[i];
	if(service_desc->is_bleAdvTest == TRUE) {
           char_desc->is_bleAdvTest1 = TRUE;
        }
        gatt_characteristic_export(object_manager, service_object_path, char_desc);
        i++;
    }

    return service;
}

BluezGattCharacteristic1 *get_gatt_ser_char_obj(const char *path)
{
    int i;
    for (i=0; i < CharIfce->CharObjCount ;i++) {
        if (strcmp(CharIfce->CharObjPath[i],path) == 0) {
            return CharIfce->CharObj[i];
        }
    }
    return NULL;
}

void gatt_char_obj_init()
{
    CharIfce = (GattCharSerObj*) malloc (sizeof(GattCharSerObj));

    int i;
    for (i=0; i < MAX_GATT_CHAR_SERVER_OBJ ;i++) {
        CharIfce->CharObj[i] = NULL;
	memset(CharIfce->CharObjPath[i],'\0',sizeof(CharIfce->CharObjPath[i]));
    }
    CharIfce->CharObjCount = 0;
}

void releaseObj(gpointer obj)
{
    if (NULL != obj)
    {
        int cnt = GATT_OBJECT_REFCOUNT_VALUE(obj);
        while (cnt-- != 0)
        {
            g_object_unref(obj);
        }
    }
}

void gatt_char_obj_release()
{
    int i;
    for (i=0; i < CharIfce->CharObjCount ;i++) {
	 releaseObj(CharIfce->CharObj[i]);
	 memset(CharIfce->CharObjPath[i],'\0',sizeof(CharIfce->CharObjPath[i]));
    }
    CharIfce->CharObjCount = 0;
}

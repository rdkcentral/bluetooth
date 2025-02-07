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
 * @file gatt.h
 * Includes information for gatt server setup over BT
*/

#ifndef __GATT_H__
#define __GATT_H__

#include <glib.h>
#include "gdbusBluez.h"

#define __unused(A) UNUSED_ ## A __attribute__((__unused__))
#define MAX_GATT_CHAR_SERVER_OBJ 15
#define BT_MAX_GATT_CHAR_STR_LEN      256

#define GATT_OBJECT_G_OBJECT_REFCOUNT(obj) (((GObject*)(obj))->ref_count)
#define GATT_OBJECT_REFCOUNT_VALUE(obj) g_atomic_int_get ((gint *) &GATT_OBJECT_G_OBJECT_REFCOUNT(obj))

struct ad_data
{
    guint8 data[25];
    guint8 len;
};

/*
            AD2 Len         0x11    Length of manufacturer data
        1   AD2 Type        0xff    Manufacturer specific proprietary data
10-11   2   Company ID      0x07a3  Comcast Cable Corporation
12-13   2   Product ID      0x0301
14-25   12  Serial Number           Device Serial Number (12 bytes)
*/

struct manufacturer_data
{
    guint16 id;
    struct ad_data data;
};

typedef struct _ble_adv_info
{
    const gchar *localName; /* Name of advertising packet : "XLESETUP-8713", etc. */
    const gchar *advType;   /* Type of advertising packet : "Broadcast"/"Peripheral" */
    gboolean discoverable;
    guint16 appearance;
    guint16 timeoutSecs;
    gint16 bTxPower; /* Includes Tx power in advertisement */
    guint16 productId;
    const gchar *const *serviceUuids; /* List of the UUIDs of the services supported by the device */
    struct manufacturer_data manufacturer;
} ble_adv_info_t;

struct leadvertisement1Cb
{
    void (*advRelease)(const char *apBtAdapter, void *);
};

void bleAdvInit(ble_adv_info_t *bleAdvInfo);
int bleStartAdv(const char *apBtAdapter, struct leadvertisement1Cb *, void *);
int bleStopAdv(const char *apBtAdapter, void *apstCbData);
void bleAdvRelease(void);

typedef struct _gatt_char_description {
  gchar *name;
  gchar *uuid;
  gboolean notifying;
  gboolean (*on_write)(BluezGattCharacteristic1 *, GDBusMethodInvocation *, GVariant *, GVariant *);
  gboolean (*on_read)(BluezGattCharacteristic1 *, GDBusMethodInvocation *, GVariant *options);
  gboolean (*on_notify_start)(BluezGattCharacteristic1 *, GDBusMethodInvocation *);
  gboolean (*on_notify_stop)(BluezGattCharacteristic1 *, GDBusMethodInvocation *);
  const gchar *flags[8];
  gchar *value;
  gboolean is_bleAdvTest1;
} gatt_characteristic_desc_t;

#define GATT_DECL_ONREAD(NAME) gboolean NAME (BluezGattCharacteristic1 *, GDBusMethodInvocation *, GVariant *options)

#define GATT_EASY_READCHAR(NAME, UUID, ONREAD) \
  { .name = #NAME, \
    .uuid = #UUID, \
    .notifying = FALSE, \
    .on_write = NULL, \
    .on_read = ONREAD, \
    .on_notify_start = NULL, \
    .on_notify_stop = NULL, \
    .flags = {"secure-read", NULL}, \
    .value = NULL \
  }

#define GATT_NULLCHAR { NULL, NULL, FALSE, NULL, NULL, NULL, NULL, {NULL}, NULL }

typedef struct _gatt_service_description {
  gchar *name;
  gchar *uuid;
  gboolean is_primary;
  gboolean is_bleAdvTest;
} gatt_service_desc_t;

typedef struct _GattCharSer_Obj {
   BluezGattCharacteristic1 * CharObj[MAX_GATT_CHAR_SERVER_OBJ];
   char                       CharObjPath[MAX_GATT_CHAR_SERVER_OBJ][BT_MAX_GATT_CHAR_STR_LEN];
   guint16                    CharObjCount;
} GattCharSerObj;

BluezObjectSkeleton *gatt_characteristic_export(
  GDBusObjectManagerServer *object_manager,
  const gchar *service_path,
  gatt_characteristic_desc_t *desc);

BluezObjectSkeleton *gatt_service_export(
  GDBusObjectManagerServer *object_manager,
  const char *root_path,
  gatt_service_desc_t *desc);

BluezObjectSkeleton *gatt_service_build_and_export(
  GDBusObjectManagerServer *object_manager,
  const char *root_path,
  gatt_service_desc_t *sevrice_description,
  gatt_characteristic_desc_t *listof_chars);

BluezObjectSkeleton *device_information_service_export(
  GDBusObjectManagerServer *object_manager,
  const gchar *root_path);

BluezObjectSkeleton *rdk_setup_service_export(
  GDBusObjectManagerServer *object_manager,
  const gchar *root_path);

BluezGattCharacteristic1 *get_gatt_ser_char_obj(const char *path);
void releaseObj(gpointer obj);
void gatt_char_obj_init();
void gatt_char_obj_release();

#endif
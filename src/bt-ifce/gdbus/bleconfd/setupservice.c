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
 * setupservice.c
 * Implementation of Gatt server over Bluetooth for testing
*/

#include "gatt.h"
#include <btrCore_platform_spec.h>

static gboolean get_qrcode(BluezGattCharacteristic1 *, GDBusMethodInvocation *, GVariant *);
static gboolean get_provision_status(BluezGattCharacteristic1 *, GDBusMethodInvocation *, GVariant *);
static gboolean get_sim_iccid(BluezGattCharacteristic1 *, GDBusMethodInvocation *, GVariant *);
static gboolean get_modem_imei(BluezGattCharacteristic1 *, GDBusMethodInvocation *, GVariant *);
static gboolean get_cellular_signal_strength(BluezGattCharacteristic1 *, GDBusMethodInvocation *, GVariant *);
static gboolean get_mesh_backhaul_type(BluezGattCharacteristic1 *, GDBusMethodInvocation *, GVariant *);
static gboolean get_wifi_backhaul_status(BluezGattCharacteristic1 *, GDBusMethodInvocation *, GVariant *);
static gboolean on_notify_start(BluezGattCharacteristic1 *, GDBusMethodInvocation *);
static gboolean on_notify_stop(BluezGattCharacteristic1 *, GDBusMethodInvocation *);


static gatt_service_desc_t service_def = {
  .name = SERVICE_SETUP,
  .uuid = SERVICE_SETUP_UUID,
  .is_primary = TRUE,
  .is_bleAdvTest = TRUE
};

static gatt_characteristic_desc_t listof_chars[] = {
  { "QRCode", UUID_QR_CODE, FALSE, NULL,
    &get_qrcode, NULL, NULL, {"read", NULL}},

  { "ProvisionStatus", UUID_PROVISION_STATUS, TRUE, NULL,
    &get_provision_status, &on_notify_start, &on_notify_stop, {"read", "notify"}},

  { "SimICCID", UUID_SIM_ICCID, FALSE, NULL,
    &get_sim_iccid, NULL, NULL, {"read", NULL}},

  { "ModemIMEI", UUID_MODEM_IMEI, FALSE, NULL,
    &get_modem_imei, NULL, NULL, {"read", NULL}},

  { "CellularSignalStrength", UUID_CELLULAR_SIGNAL_STRENGTH, FALSE, NULL,
    &get_cellular_signal_strength, NULL, NULL, {"read", NULL}},

  { "MeshBackhaulType", UUID_MESH_BACKHAUL_TYPE, FALSE, NULL,
    &get_mesh_backhaul_type, NULL, NULL, {"read", NULL}},

  { "WiFiBackhaulStatus", UUID_WIFI_BACKHAUL_STATS, FALSE, NULL,
    &get_wifi_backhaul_status, NULL, NULL, {"read", NULL}},

  GATT_NULLCHAR
};


BluezObjectSkeleton *rdk_setup_service_export(GDBusObjectManagerServer *object_manager,
  const gchar *object_path_root)
{
  return gatt_service_build_and_export(
    object_manager,
    object_path_root,
    &service_def,
    listof_chars);
}

gboolean get_qrcode(BluezGattCharacteristic1 *object, GDBusMethodInvocation *invocation,
  GVariant * __unused(options))
{
  g_info("Getting qrcode\n");

  gchar *qrCode ="QRCode_test";
  bluez_gatt_characteristic1_complete_read_value(
    object,
    invocation,
    g_variant_new_bytestring(qrCode));
  return TRUE;
}

static BluezGattCharacteristic1 *provision_status_char = NULL;
static guint16 status = 0x201;

gboolean update_status(gpointer __unused(user_data))
{
  GVariantBuilder builder;
  g_variant_builder_init(&builder, G_VARIANT_TYPE("ay"));

  guint8 *p = (guint8 *) &status;
  g_variant_builder_add(&builder, "y", *p++);
  g_variant_builder_add(&builder, "y", *p);
  bluez_gatt_characteristic1_set_value(provision_status_char, g_variant_new("ay", &builder));

  status++;

  return G_SOURCE_CONTINUE;
}

gboolean on_notify_start(BluezGattCharacteristic1 *object,
  GDBusMethodInvocation *invocation)
{
  static gboolean timer_started = FALSE;

  const gchar *uuid = bluez_gatt_characteristic1_get_uuid(object);
  g_info("notify start:%s", uuid);

  //
  // XXX: This is just a sample of how to do notifications. You have to call
  // bluez_gatt_characteristic1_set_value so you need a pointer to the 
  // BluezGattCharacteristic1. You can do this on creation or capture here
  //
  if (!provision_status_char && g_str_equal(UUID_PROVISION_STATUS, uuid)) {
    provision_status_char = g_object_ref(object);
    if (!timer_started) {
      g_timeout_add(1000, update_status, NULL);
      timer_started = TRUE;
    }
  }
  return TRUE;
}

gboolean on_notify_stop(BluezGattCharacteristic1 *object, GDBusMethodInvocation *invocation)
{
  const gchar *uuid = bluez_gatt_characteristic1_get_uuid(object);
  g_info("notify stop:%s", uuid);

  return TRUE;
}

gboolean get_provision_status(BluezGattCharacteristic1 *object, GDBusMethodInvocation *invocation,
  GVariant * __unused(options))
{
  g_info("Getting provision status\n");

  GVariantBuilder builder;
  g_variant_builder_init(&builder, G_VARIANT_TYPE("ay"));

  guint8 *p = (guint8 *) &status;
  g_variant_builder_add(&builder, "y", *p++);
  g_variant_builder_add(&builder, "y", *p);

  bluez_gatt_characteristic1_complete_read_value(
    object,
    invocation,
    g_variant_new("ay", &builder));

  return TRUE;
}

gboolean get_sim_iccid(BluezGattCharacteristic1 *object, GDBusMethodInvocation *invocation,
  GVariant * __unused(options))
{
  g_info("Getting ssimIccid value\n");

  gchar *simICCID = "simICCID_test";
  bluez_gatt_characteristic1_complete_read_value(
    object,
    invocation,
    g_variant_new_bytestring(simICCID));
  return TRUE;
}

gboolean get_modem_imei(BluezGattCharacteristic1 *object, GDBusMethodInvocation *invocation,
  GVariant * __unused(options))
{
  g_info("Getting ModemIMEI value\n");

  gchar *modemIMEI = "modemIMEI_test";
  bluez_gatt_characteristic1_complete_read_value(
    object,
    invocation,
    g_variant_new_bytestring(modemIMEI));
  return TRUE;
}

gboolean get_cellular_signal_strength(BluezGattCharacteristic1 *object, GDBusMethodInvocation *invocation,
  GVariant * __unused(options))
{
  g_info("Getting cellularSignalStrength value\n");

  gint16 cellSignalStrength = -100;
  GVariantBuilder builder;
  g_variant_builder_init(&builder, G_VARIANT_TYPE("ay"));

  gint8 *p = (gint8 *) &cellSignalStrength;
  g_variant_builder_add(&builder, "y", *p++);
  g_variant_builder_add(&builder, "y", *p);

  bluez_gatt_characteristic1_complete_read_value(
    object,
    invocation,
    g_variant_new("ay", &builder));

  return TRUE;
}

gboolean get_mesh_backhaul_type(BluezGattCharacteristic1 *object, GDBusMethodInvocation *invocation,
  GVariant * __unused(options))
{
  g_info("Getting meshBackhaulType value\n");

  guint16 backHaulType = RDK_SETUP_MESH_BACKHAUL_WIFI;
  GVariantBuilder builder;
  g_variant_builder_init(&builder, G_VARIANT_TYPE("ay"));

  guint8 *p = (guint8 *) &backHaulType;
  g_variant_builder_add(&builder, "y", *p++);
  g_variant_builder_add(&builder, "y", *p);

  bluez_gatt_characteristic1_complete_read_value(
    object,
    invocation,
    g_variant_new("ay", &builder));

  return TRUE;
}

gboolean get_wifi_backhaul_status(BluezGattCharacteristic1 *object, GDBusMethodInvocation *invocation,
  GVariant * __unused(options))
{
  g_info("Getting WifiBackhaulStatus value\n");

  gchar *wifiBackhaulStatus = "wifiStatus_test";
  bluez_gatt_characteristic1_complete_read_value(
    object,
    invocation,
    g_variant_new_bytestring(wifiBackhaulStatus));
  return TRUE;
}

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
 * btrCore_platform_dis_cb.c
 * Implementation of Gatt server
*/

#include "gatt.h"
#include <btrCore_platform_spec.h>

//=============================================================================
// platform specific callbacks here
gboolean
device_information_get_system_id(BluezGattCharacteristic1 *object, GDBusMethodInvocation *invocation,
  GVariant * __unused(options))
{
  g_info("getting deviceInformationSystemId \n");

  gchar *systemID = "systemID_test";
  bluez_gatt_characteristic1_complete_read_value(
    object,
    invocation,
    g_variant_new_bytestring(systemID));

  return TRUE;
}

gboolean
device_information_get_model_number(BluezGattCharacteristic1 *object, GDBusMethodInvocation *invocation,
  GVariant * __unused(options))
{
  g_info("getting deviceInformationmodelNumber \n");

  gchar *modelNumber = "modelNumber_test";
  bluez_gatt_characteristic1_complete_read_value(
    object,
    invocation,
    g_variant_new_bytestring(modelNumber));
  return TRUE;
}

gboolean
device_information_get_serial_number(BluezGattCharacteristic1 *object, GDBusMethodInvocation *invocation,
  GVariant * __unused(options))
{
  g_info("getting deviceInformationSerialNumber\n");

  gchar *SerialNumber= "SerialNumber_test";
  bluez_gatt_characteristic1_complete_read_value(
    object,
    invocation,
    g_variant_new_bytestring(SerialNumber));

  return TRUE;
}
gboolean
device_information_get_firmware_version(BluezGattCharacteristic1 *object, GDBusMethodInvocation *invocation,
  GVariant * __unused(options))
{
  g_info("getting deviceInformatioFirmwareVersion\n");

  gchar *firmwareVersion= "firmwareVersion_test";
  bluez_gatt_characteristic1_complete_read_value(
    object,
    invocation,
    g_variant_new_bytestring(firmwareVersion));

  return TRUE;
}

gboolean
device_information_get_hardware_version(BluezGattCharacteristic1 *object, GDBusMethodInvocation *invocation,
  GVariant * __unused(options))
{
  g_info("getting deviceInformatioHardwareVersion\n");

  gchar *hardwareVersion= "hardwareVersion_test";
  bluez_gatt_characteristic1_complete_read_value(
    object,
    invocation,
    g_variant_new_bytestring(hardwareVersion));

  return TRUE;
}

gboolean
device_information_get_software_version(BluezGattCharacteristic1 *object, GDBusMethodInvocation *invocation,
  GVariant * __unused(options))
{
  g_info("getting deviceInformatioSoftwareVersion\n");

  gchar *softwareVersion= "softwareVersion_test";
  bluez_gatt_characteristic1_complete_read_value(
    object,
    invocation,
    g_variant_new_bytestring(softwareVersion));

  return TRUE;
}

gboolean
device_information_get_manufacturer_name(BluezGattCharacteristic1 *object, GDBusMethodInvocation *invocation,
  GVariant * __unused(options))
{
  g_info("getting deviceInformationManufacturerName\n");

  gchar *manufacturerName= "manufacturerName_test";
  bluez_gatt_characteristic1_complete_read_value(
    object,
    invocation,
    g_variant_new_bytestring(manufacturerName));

  return TRUE;
}

static GVariant* get_manufacturer_data()
{
  int i, n;
  gchar *serial_number = "serialNumber_test";

  GVariantBuilder builder;
  g_variant_builder_init(&builder, G_VARIANT_TYPE("ay"));

  // add in product id
  g_variant_builder_add(&builder, "y", 0x03);
  g_variant_builder_add(&builder, "y", 0x01);

  for (i = 0, n = strlen(serial_number); i < n; ++i)
    g_variant_builder_add(&builder, "y", (guint8) serial_number[i]);

  return g_variant_builder_end(&builder);
}

void
app_advertisement_setup(BluezLEAdvertisement1 *advertisement)
{
  bluez_leadvertisement1_set_discoverable(advertisement, TRUE);

  // last 2 bytes of mac address
  bluez_leadvertisement1_set_local_name(advertisement, "PLATFORMSETUP-1234");

  GVariantBuilder builder;
  g_variant_builder_init(&builder, G_VARIANT_TYPE("a{qv}"));
  g_variant_builder_add(&builder, "{qv}", COMPANY_ID, get_manufacturer_data());

  bluez_leadvertisement1_set_manufacturer_data(advertisement,
    g_variant_builder_end(&builder));

}

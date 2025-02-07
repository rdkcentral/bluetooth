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

#include "gatt.h"
#include <deviceinfo.h>
#include <btrCore_platform_spec.h>

GATT_DECL_ONREAD(device_information_get_system_id);
GATT_DECL_ONREAD(device_information_get_model_number);
GATT_DECL_ONREAD(device_information_get_serial_number);
GATT_DECL_ONREAD(device_information_get_firmware_version);
GATT_DECL_ONREAD(device_information_get_hardware_version);
GATT_DECL_ONREAD(device_information_get_software_version);
GATT_DECL_ONREAD(device_information_get_manufacturer_name);

static gatt_service_desc_t service_def = {
    .name = SERVICE_DIS,
    .uuid = SERVICE_DIS_UUID,
    .is_primary = TRUE,
    .is_bleAdvTest = TRUE
};

static gatt_characteristic_desc_t listof_chars[] = {
    GATT_EASY_READCHAR(SystemId, 0x2a23, &device_information_get_system_id),
    GATT_EASY_READCHAR(ModelNumber, 0x2a24, &device_information_get_model_number),
    GATT_EASY_READCHAR(SerialNumber, 0x2a25, &device_information_get_serial_number),
    GATT_EASY_READCHAR(FirmwareVersion, 0x2a26, &device_information_get_firmware_version),
    GATT_EASY_READCHAR(HardwareVersion, 0x2a27, &device_information_get_hardware_version),
    GATT_EASY_READCHAR(SoftwareVersion, 0x2a28, &device_information_get_software_version),
    GATT_EASY_READCHAR(ManufacturerName, 0x2a29, &device_information_get_manufacturer_name),
    GATT_NULLCHAR};

BluezObjectSkeleton *device_information_service_export(GDBusObjectManagerServer *object_manager,
                                                       const gchar *object_path_root)
{

    return gatt_service_build_and_export(
        object_manager,
        object_path_root,
        &service_def,
        listof_chars);
}

// The following is for auto export gatt info from btrcore
bleGattInfo *gGattInfo;

void serviceInit()
{
    // delete later
}

void bleGattServiceInit(bleGattInfo *gattInfo)
{
    gGattInfo = gattInfo;
}

int exportServices(GDBusObjectManagerServer *object_manager, const gchar *object_path_root)
{
    gatt_service_desc_t *pService_def;
    gatt_characteristic_desc_t *pListof_chars;
    for (int i = 0; i < gGattInfo->nNumGattServices; i++)
    {
        pService_def = &gGattInfo->gattService[i].service_def;
        pListof_chars = gGattInfo->gattService[i].listof_chars;
        g_info("Export service : %s", pService_def->name);
        g_info("Export service char: %s", pListof_chars[i].name);
        if (!gatt_service_build_and_export(
                object_manager,
                object_path_root,
                pService_def,
                pListof_chars))
        {
            g_info("Failed to export service : %s", pService_def->name);
            return -1;
        }
    }
    return 0;
}

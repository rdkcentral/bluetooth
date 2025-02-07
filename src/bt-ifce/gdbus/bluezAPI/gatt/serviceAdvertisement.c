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
 * serviceAdvertisement.c
 * Implementation of Gatt advertisement
*/

#include "bluezAPICommon.h"
#define G_LOG_DOMAIN _LOG_DOMAIN "-serviceAdvertisement"

#include <glib.h>
#include <stdint.h>
#include "gatt/serviceAdvertisement.h"

#define BLE_ADV_TXPOWER_NO_PREFERENCE 0x7f

BluezLEAdvertisement1 *serviceAdvertisementCreate(const char *const *serviceUuids, const char *localName, bool discoverable, uint16_t timeoutSecs)
{
    g_autofree char *uuidList = g_strjoinv(",", (gchar **) serviceUuids);
    g_debug("Creating advertisement with service uuids [%s]", uuidList);

    BluezLEAdvertisement1 *advInterface = bluez_leadvertisement1_skeleton_new();

    bluez_leadvertisement1_set_appearance(advInterface, BLE_APPEARANCE_TABLET);
    bluez_leadvertisement1_set_type_(advInterface, "peripheral");

    if (discoverable)
    {
        bluez_leadvertisement1_set_discoverable(advInterface, TRUE);
    }

    if (timeoutSecs)
    {
        bluez_leadvertisement1_set_discoverable_timeout(advInterface, timeoutSecs);
        bluez_leadvertisement1_set_timeout(advInterface, timeoutSecs);
    }

    bluez_leadvertisement1_set_tx_power(advInterface, BLE_ADV_TXPOWER_NO_PREFERENCE);

    if (localName != NULL)
    {
        bluez_leadvertisement1_set_local_name(advInterface, localName);
    }

    bluez_leadvertisement1_set_service_uuids(advInterface, serviceUuids);

    return advInterface;
}

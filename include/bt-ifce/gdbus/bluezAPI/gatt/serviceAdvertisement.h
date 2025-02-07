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
 * @file serviceAdvertisement.h
 * Includes information for advertisement setup
*/

#ifndef __SERVICE_ADV_H__
#define __SERVICE_ADV_H__

#include <stdbool.h>
#include <gdbusBluez.h>

#define BLE_APPEARANCE_TABLET 0x0087

/**
 * Create basic advertisement data for a set of services
 * @param serviceUuids the 128 bit service uuids to advertise, terminated with NULL
 * @param localName the localName to set on the advertisement (optional, recommended.)
 *        Note: this shall be the same as the device LocalName (Bluetooth Core Supplement, 1.2).
 *        @see bluez_adapter1_get_name
 * @param discoverable This will override the adapter's Discoverable property. Set this to true to make the
 *                     advertisement scannable.
 * @param timeoutSecs Set this to a nonzero to set the advertisement's Limited Discoverable flag.
 *                    After the timeout, the advertisement will no longer be scannable.
 * @return An advertisement, or NULL on failure. Use g_object_ref/unref (or g_autoptr) when acquiring/discarding
 *         references.
 */
BluezLEAdvertisement1 *serviceAdvertisementCreate(const char *const *serviceUuids, const char *localName, bool discoverable, uint16_t timeoutSecs);

#endif //__SERVICE_ADV_H__

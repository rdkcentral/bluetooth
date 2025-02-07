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
 * @file deviceinfo.h
 * Includes information for gatt server setup over BT
*/

#ifndef __DEVICEINFO_H__
#define __DEVICEINFO_H__

#include "gatt.h"

typedef struct BleGattService
{
  gatt_service_desc_t service_def;
  gatt_characteristic_desc_t listof_chars[10];
  unsigned short ui16NumberOfGattChar;
} bleGattService;

typedef struct BleGattInfo{
  bleGattService gattService[10];
  int nNumGattServices;
} bleGattInfo;

void bleGattServiceInit(bleGattInfo *gattInfo);
int exportServices(GDBusObjectManagerServer *object_manager, const gchar *object_path_root);

#endif // end of __DEVICEINFO_H__
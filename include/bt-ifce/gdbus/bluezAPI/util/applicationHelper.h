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
 * @file applicationHelper.h
 * Includes information for gatt server application setup over BT
*/

#ifndef __APPLICATIONHELPER_H__
#define __APPLICATIONHELPER_H__

#include <gio/gio.h>
#include "gdbusBluez.h"

typedef struct _BluezApplication BluezApplication;
/**
 * Callback to notify a server that its application has been completely registered.
 */
typedef void (*BluezApplicationRegisteredFunc)(BluezApplication *app, void *userData);

/**
 * Callback to notify a server that its application has been completely unregistered.
 */

typedef void (*BluezApplicationUnregisteredFunc)(BluezApplication *app, void *userData);

struct _BluezApplication
{
    GDBusObjectManagerServer *objectManager;
    GMainContext *expectedContext;
    guint bluezWatchId;
    char *adapterPath;
    char *leAdvPath;
    int components;
    int componentMask;

    BluezGattManager1 *gattManager;
    BluezLEAdvertisingManager1 *leAdvertisingManager;

    BluezApplicationRegisteredFunc onRegistered;
    BluezApplicationUnregisteredFunc onUnregistered;
    void *callbackData;

    grefcount busy;
};



/**
 * Create a BlueZ application
 * @param adapterPath adapter to attach to
 * @param objectManager [nullable][transfer-full] GATT services object manager. All managed objects are unexported when the
                        application is unregistered.
 * @param leAdvertisementPath [nullable] LE advertisement to register
 * @param onRegistered [nullable] invoked when the application has been successfully registered with BlueZ.
                                  Any failures are guaranteed to produce an onUnregistered call.
 * @param onUnregistered [nullable] invoked when the application has been removed form BlueZ
 * @param callbackData [nullable] data to be provided in onRegistered/onUnregistered.
 * @return
 */
BluezApplication *bluezApplicationHelperCreate(const char *adapterPath,
                                               GDBusObjectManagerServer *objectManager,
                                               const char *leAdvertisementPath,
                                               BluezApplicationRegisteredFunc onRegistered,
                                               BluezApplicationUnregisteredFunc onUnregistered,
                                               void *callbackData);

/**
 * Register a BlueZ application. Call this after acquiring the bus name.
 * @note this must be called on the same thread as bluezApplicationHelperCreate.
 * @param app
 */
void bluezApplicationHelperRegister(BluezApplication *app);

/**
 * Unregister a BlueZ application. Call this when losing the bus name, or
 * before stopping your D-Bus server.
 * @note this must be called on the same thread as bluezApplicationHelperCreate.
 * @param app
 */
void bluezApplicationHelperUnregister(BluezApplication *app);

/**
 * Acquire a reference to a BlueZ application.
 * Use bluezApplicationHelperRelease when finished.
 * @param app
 * @return the same application with a reference taken.
 */
inline BluezApplication *bluezApplicationHelperAcquire(BluezApplication *app)
{
    return g_atomic_rc_box_acquire(app);
}

/**
 * Release a reference to a BlueZ application.
 * After the last reference is released, the object will be destroyed
 * automatically.
 * @param app
 */
void bluezApplicationHelperRelease(BluezApplication *app);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BluezApplication, bluezApplicationHelperRelease);

/**
 * Get a proxy to the BlueZ LE Advertising manager
 * @param adapterPath The adapter path to host LE advertisement data (e.g., /org/bluez/hci0)
 * @note adapters have a finite space for LE Advertising Data.
 * @see bluez_leadvertising_manager1_get_supported_instances() and bluez_leadvertising_manager1_get_active_instances() for available space
 * @see bluezAdapterUtilsGetDefault for the default adapter
 * @return [nullable] the org.bluez.LEAdvertisingManager1 interface proxy.
 */
BluezLEAdvertisingManager1 *bluezApplicationHelperGetLEAdvertisingManager(const char *adapterPath);

/**
 * Get a proxy to the BlueZ GATT manager
 * @param adapterPath The adapter path to host GATT service(s).
 * @return [nullable] the org.bluez.GATTManager1 interface proxy.
 */
BluezGattManager1 *bluezApplicationHelperGetGATTManager(const char *adapterPath);


#endif //__APPLICATIONHELPER_H__

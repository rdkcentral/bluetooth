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
 * btr_leAdvGDBus.c
 * Implementation of Gatt server over Bluetooth
*/

#include <util/applicationHelper.h>
#include <util/agentHelper.h>
#include "gatt.h"
#include "deviceinfo.h"

#include <getopt.h>
#include <glib/gprintf.h>
#include <util/adapterUtils.h>

#include <stdint.h>
#include <gatt/serviceAdvertisement.h>

#include <stdio.h>
#include <gio/gio.h>
#include <gdbusBluez.h>
#include <bluez-dbus.h>

typedef struct _bleconfd_t
{
    GDBusConnection *bus;
    guint owner_id;
    gchar *bus_name;
    GBusType bus_type;
    BluezAdapter1 *adapter;
    GDBusObjectManagerServer *object_manager;
    gchar *bus_root_path;
    BluezApplication *app;
} bleconfd_t;

G_DEFINE_QUARK(bleconfd - quark, bleconfd_error)

static void bleconfd_init(GDBusConnection *bus, gpointer user_data);
static void bleconfd_dump_context(bleconfd_t *ctx);
static void bleconfd_on_app_registered(BluezApplication *app, gpointer user_data);
static void bleconfd_on_app_unregistered(BluezApplication *app, gpointer user_data);

static GDBusConnection *gConn = NULL;
static BluezLEAdvertisement1 *gAdvIface = NULL;
static bleconfd_t *gCtx = NULL;
static char gAdvObjPath[256];
static void *gBtLeAdvApstIfceHdl = NULL;
static struct leadvertisement1Cb *gBtLeAdvCb  = NULL;
static gulong timeout_handler_id = 0;

static gboolean onAdvRelease(
    BluezLEAdvertisement1 *object,
    GDBusMethodInvocation *invocation) {
    g_message("Advertisement timeout");
    bluez_leadvertisement1_complete_release(object, invocation);

    // informing up layer adv is gone
    gBtLeAdvCb->advRelease(0, gBtLeAdvApstIfceHdl);
    return TRUE;
}

static BluezLEAdvertisement1 *bleAdvertisementCreate(ble_adv_info_t *advInfo)
{
    BluezLEAdvertisement1 *advObj = bluez_leadvertisement1_skeleton_new();

    if (advObj == NULL) {
        g_message("Unable to create Advertisement");
        return NULL;
    }

    bluez_leadvertisement1_set_appearance(advObj, advInfo->appearance);
    bluez_leadvertisement1_set_type_(advObj, advInfo->advType);

    if (advInfo->timeoutSecs)
    {
        bluez_leadvertisement1_set_discoverable_timeout(advObj, advInfo->timeoutSecs);
        bluez_leadvertisement1_set_timeout(advObj, advInfo->timeoutSecs);
    }

    bluez_leadvertisement1_set_tx_power(advObj, advInfo->bTxPower);

    if (advInfo->localName != NULL)
    {
        g_message("Set Beacon name: %s", advInfo->localName);
        bluez_leadvertisement1_set_local_name(advObj, advInfo->localName);
    }

    if (advInfo->discoverable)
    {
        bluez_leadvertisement1_set_discoverable(advObj, TRUE);
    }

    bluez_leadvertisement1_set_service_uuids(advObj, advInfo->serviceUuids);

    GVariantBuilder manfDataBuilder;
    g_variant_builder_init(&manfDataBuilder, G_VARIANT_TYPE("ay"));

    // add product id if there's product ID value
    if (advInfo->productId)
    {
        g_variant_builder_add(&manfDataBuilder, "y", advInfo->productId & 0xff);
        g_variant_builder_add(&manfDataBuilder, "y", (advInfo->productId >> 8) & 0xff);
    }

    for (int i = 0; i < advInfo->manufacturer.data.len; i++)
        g_variant_builder_add(&manfDataBuilder, "y", advInfo->manufacturer.data.data[i]);

    GVariantBuilder manfBuilder;
    g_variant_builder_init(&manfBuilder, G_VARIANT_TYPE("a{qv}"));
    g_variant_builder_add(&manfBuilder, "{qv}", advInfo->manufacturer.id, g_variant_builder_end(&manfDataBuilder));

    bluez_leadvertisement1_set_manufacturer_data(advObj, g_variant_builder_end(&manfBuilder));

    /*TODO:Work around, Need to identify exact root cause for the Flag setting as 0x01(Limited Discoverable)
      instead of 0x02 (General Discoverable) when setting discoverable_timeout from btMgrBus*/
    bluez_adapter1_set_discoverable_timeout((BluezAdapter1 *)advObj, 0);

    // set callback for the timeout 
    timeout_handler_id = g_signal_connect(advObj, "handle-release", G_CALLBACK(onAdvRelease), NULL);
    if(timeout_handler_id == 0){
        g_message("Failed to register the release signal");
    } else{
        g_message("Success to register the release signal");
    }

    return advObj;
}

static void bleconfd_init(GDBusConnection *bus, gpointer user_data)
{
    // g_info("on_bus_acquired:%s", name);

    bleconfd_t *ctx = (bleconfd_t *)user_data;
    ctx->bus = bus;
    ctx->adapter = bluezAdapterUtilsGetDefault();
    
    g_sprintf(gAdvObjPath, "%s/Advertisement", ctx->bus_root_path);

    ctx->object_manager = g_dbus_object_manager_server_new(ctx->bus_root_path);

    ctx->app = bluezApplicationHelperCreate(
        bluezAdapterUtilsGetObjectPath(ctx->adapter),
        g_object_ref(ctx->object_manager),
        gAdvObjPath,
        (BluezApplicationRegisteredFunc)bleconfd_on_app_registered,
        (BluezApplicationUnregisteredFunc)bleconfd_on_app_unregistered,
        ctx);

    exportServices(ctx->object_manager, ctx->bus_root_path);
    // export all objects
    g_autoptr(BluezObjectSkeleton) advertisement_skeleton =
        bluez_object_skeleton_new(gAdvObjPath);
    bluez_object_skeleton_set_leadvertisement1(advertisement_skeleton, gAdvIface);
    g_dbus_object_manager_server_export(ctx->object_manager, G_DBUS_OBJECT_SKELETON(advertisement_skeleton));
    g_dbus_object_manager_server_set_connection(ctx->object_manager, ctx->bus);
}

static void bleconfd_dump_context(bleconfd_t *ctx)
{
    g_message("bus_name    : %s", ctx->bus_name);
    g_message("bus_type    : %s", (ctx->bus_type == G_BUS_TYPE_SYSTEM ? "system" : "session"));
    g_message("root_path   : %s", ctx->bus_root_path);
}

static void bleconfd_on_app_registered(BluezApplication *__unused(app),
                                gpointer __unused(user_data))
{
    g_message("bleconfd_on_app_registered");
}

void bleAdvInit(ble_adv_info_t *advInfo)
{
    gAdvIface = bleAdvertisementCreate(advInfo);
}

static void bleAdvDeInit(void)
{
    // release obj manager to avoid existed service obj error
    if (gCtx && gCtx->object_manager) {
        releaseObj(G_OBJECT(gCtx->object_manager));
        gCtx->object_manager = NULL;
    }
    // release all char obj to avoid char error
    gatt_char_obj_release();

    if (gAdvIface) {
        releaseObj(G_OBJECT(gAdvIface));
        gAdvIface = NULL;
    }

    if (gConn) {
        g_object_unref(gConn);
        gConn = NULL;
    }

    if (gCtx) {
        if (gCtx->adapter) {
            releaseObj(G_OBJECT(gCtx->adapter));
            gCtx->adapter = NULL;
        }

        if (gCtx->bus_name) {
            free(gCtx->bus_name);
            gCtx->bus_name = NULL;
        }

        if (gCtx->bus_root_path) {
            free(gCtx->bus_root_path);
            gCtx->bus_root_path = NULL;
        }

        gCtx->app = NULL;
        gCtx->bus = NULL;

        g_message("bleconfd_ FREEING gCtx");
        g_free(gCtx);
        gCtx = NULL;
    }

    gBtLeAdvApstIfceHdl = NULL;
    gBtLeAdvCb = NULL;
}

static void bleconfd_on_app_unregistered(BluezApplication *__unused(app),
                                         gpointer __unused(user_data))
{
    g_message("bleconfd_on_app_unregistered");
}

int bleStartAdv(const char *apBtAdapter,
                struct leadvertisement1Cb *apstAdvCb,
                void *apstBtLeAdvIfceHdl)
{
    g_message("Registering Advertisement.");
    bleconfd_t *ctx = g_new0(bleconfd_t, 1);
    ctx->bus_type = G_BUS_TYPE_SYSTEM;
    if (ctx->bus_name == NULL)
        ctx->bus_name = g_strdup("org.rdk.bluetooth");
    if (ctx->bus_root_path == NULL)
        ctx->bus_root_path = g_strdup("/org/bluez/hci0");

    ctx->bus_type = G_BUS_TYPE_SYSTEM;
    gConn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, NULL);
    bleconfd_dump_context(ctx);
    bleconfd_init(gConn, ctx);
    bluezApplicationHelperRegister(ctx->app);
    gCtx = ctx;
    gBtLeAdvApstIfceHdl = apstBtLeAdvIfceHdl;
    gBtLeAdvCb = apstAdvCb;
    return 0;
}

// this function is only called from upper layer when there's advertising.
int bleStopAdv(const char *apBtAdapter, void *apstCbData)
{
    g_message("Stop Brodcasting/Advertisement.");
    int ret = -1;
    if (gCtx == NULL || gConn == NULL)
    {
        g_message("No adv registered.");
        return ret;
    }

    g_message("Unregister advertisement at path \"%s\".", gCtx->bus_root_path);

    //(process:37174): GLib-GIO-WARNING **: 17:01:00.350: ../../../gio/gdbusobjectmanagerserver.c:1118: Error registering object at /org/bluez/hci0/Advertisement
    // with interface org.bluez.LEAdvertisement1: An object is already exported for the interface org.bluez.LEAdvertisement1 at /org/bluez/hci0/Advertisement
    // for this error
    if (g_dbus_object_manager_server_unexport(gCtx->object_manager, gAdvObjPath))
    {
        g_message("Unexport the obj mgr");

        bluezApplicationHelperUnregister(gCtx->app);
        bleAdvDeInit();
        ret = 0;
    }
    else
    {
        g_warning("Failed to remove adv object from object manager.");
        ret = -2;
    }

    return ret;
}

// clean up when adv timeout event
void bleAdvRelease(void) {
    g_message("Adv Timeout, Clean up.");
    // disconnect the adv obj mgr to the adv path
    g_dbus_object_manager_server_unexport(gCtx->object_manager, gAdvObjPath);
    // disconnect the application from the obj mgr
    bluez_gatt_manager1_call_unregister_application_sync(gCtx->app->gattManager,
                                                         g_dbus_object_manager_get_object_path(G_DBUS_OBJECT_MANAGER(gCtx->app->objectManager)),
                                                         NULL, /* cancellable */
                                                         NULL);
    // disconnect the timeout/release event
    g_signal_handler_disconnect(gAdvIface, timeout_handler_id);
    timeout_handler_id = 0;
    bleAdvDeInit();
}
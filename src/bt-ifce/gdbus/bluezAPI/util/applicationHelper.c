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
 * applicationHelper.c
 * Implementation of Gatt server setup over Bluetooth
*/


#include "bluezAPICommon.h"
#define G_LOG_DOMAIN _LOG_DOMAIN "-appHelper"

#include <gio/gio.h>
#include <gdbusBluez.h>
#include <util/applicationHelper.h>
#include <bluez-dbus.h>
#include <stdbool.h>

BluezGattManager1 *bluezApplicationHelperGetGATTManager(const char *adapterPath)
{
    g_autoptr(GError) error = NULL;
    g_return_val_if_fail(adapterPath != NULL, NULL);

    g_debug("Getting GattManager1 at path '%s'", adapterPath);

    BluezGattManager1 *proxy = bluez_gatt_manager1_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                                                          G_DBUS_PROXY_FLAGS_NONE,
                                                                          BLUEZ_BUS_NAME,   /* bus name */
                                                                          adapterPath,      /* object path */
                                                                          NULL,             /* cancellable */
                                                                          &error);

    if (error != NULL)
    {
        g_critical("could not get GATT manager: %s", error->message);
    }

    return proxy;
}

BluezLEAdvertisingManager1 *bluezApplicationHelperGetLEAdvertisingManager(const char *adapterPath)
{
    g_autoptr(GError) error = NULL;
    g_return_val_if_fail(adapterPath != NULL, NULL);

    g_debug("Getting AdvertisingManager1 at path '%s'", adapterPath);

    BluezLEAdvertisingManager1 *proxy = bluez_leadvertising_manager1_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                                                                            G_DBUS_PROXY_FLAGS_NONE,
                                                                                            BLUEZ_BUS_NAME, /* bus name */
                                                                                            adapterPath,    /* object path */
                                                                                            NULL,           /* cancellable */
                                                                                            &error);

    if (error != NULL)
    {
        g_critical("Could not get advertising manager: %s", error->message);
    }

    return proxy;
}

enum BluezComponent
{
    BLUEZ_COMPONENT_NONE =      0,
    BLUEZ_COMPONENT_GATT =      1 << 0,
    BLUEZ_COMPONENT_LE_ADV =    1 << 1,
};

static bool setRegistered(BluezApplication *bluez, enum BluezComponent which);
static bool setUnregistered(BluezApplication *bluez, enum BluezComponent which);
static void onApplicationUnregistered(BluezApplication *app);

static inline const char *getRootPath(BluezApplication *app)
{
    if (app->objectManager)
        return g_dbus_object_manager_get_object_path(G_DBUS_OBJECT_MANAGER(app->objectManager));
    else
    {
        g_message("Invalid bluez app.");
        return NULL;
    }
}

static void onGATTRegistered(GDBusMethodInvocation *invocation,
                             GAsyncResult *res,
                             BluezApplication *ctx)
{
    g_return_if_fail(ctx);
    g_return_if_fail(ctx->gattManager);

    g_autoptr(GError) error = NULL;

    gboolean registered = bluez_gatt_manager1_call_register_application_finish(ctx->gattManager, res, &error);

    if (registered)
    {
        g_message("GATT registered at '%s'", getRootPath(ctx));

        if(setRegistered(ctx, BLUEZ_COMPONENT_GATT) && ctx->onRegistered)
        {
            ctx->onRegistered(ctx, ctx->callbackData);
        }
    }
    else
    {
        g_warning("GATT registration failed: '%s'", error->message);
        bluezApplicationHelperUnregister(ctx);
    }

}

void onLEAdvertisementRegistered(GDBusMethodInvocation *invocation,
                                 GAsyncResult *res,
                                 BluezApplication *ctx)
{
    g_return_if_fail(ctx);

    g_return_if_fail(BLUEZ_IS_LEADVERTISING_MANAGER1(ctx->leAdvertisingManager));

    g_autoptr(GError) error = NULL;

    gboolean registered = bluez_leadvertising_manager1_call_register_advertisement_finish(ctx->leAdvertisingManager,
                                                                                          res,
                                                                                          &error);

    if (registered)
    {
        g_message("Registered LE advertisement at '%s'", ctx->leAdvPath);
        if (setRegistered(ctx, BLUEZ_COMPONENT_LE_ADV) && ctx->onRegistered)
        {
            ctx->onRegistered(ctx, ctx->callbackData);
        }
    }
    else
    {
        g_warning("Failed to register LE advertisement at '%s': %s", ctx->leAdvPath, error->message);
        bluezApplicationHelperUnregister(ctx);
    }
}

#if 0
static void onGATTUnregistered(GDBusMethodInvocation *invocation,
                               GAsyncResult *res,
                               BluezApplication *ctx)
{
    g_return_if_fail(ctx);
    g_return_if_fail(ctx->gattManager);

    g_autoptr(GError) error = NULL;

    gboolean unregistered = bluez_gatt_manager1_call_unregister_application_finish(ctx->gattManager,
                                                                                   res,
                                                                                   &error);
    const char *rootPath = getRootPath(ctx);

    if (unregistered)
    {
        g_message("GATT at '%s' unregistered", rootPath);
        setUnregistered(ctx, BLUEZ_COMPONENT_GATT);
    }
    else
    {
        g_warning("GATT unregistration at '%s' failed: %s", rootPath, error->message);
    }

    g_object_unref(ctx->gattManager);
    ctx->gattManager = NULL;

    onApplicationUnregistered(ctx);

    bluezApplicationHelperRelease(ctx);
}

void onLEAdvertisementUnregistered(GDBusMethodInvocation *invocation,
                                   GAsyncResult *res,
                                   BluezApplication *ctx)
{

    g_return_if_fail(ctx);
    g_return_if_fail(ctx->leAdvertisingManager);

    g_autoptr(GError) error = NULL;

    gboolean unregistered = bluez_leadvertising_manager1_call_unregister_advertisement_finish(ctx->leAdvertisingManager,
                                                                                              res,
                                                                                              &error);

    if (unregistered)
    {
        g_message("Unregistered LE advertisement at '%s'", ctx->leAdvPath);
    }
    else
    {
        g_message("Notice: Failed to unregister LE advertisement at '%s': %s", ctx->leAdvPath, error->message);
    }

    g_object_unref(ctx->leAdvertisingManager);
    ctx->leAdvertisingManager = NULL;

    onApplicationUnregistered(ctx);

    bluezApplicationHelperRelease(ctx);
}
#endif

static void onBluezGone(GDBusConnection* connection, const gchar* name, gpointer user_data)
{
    g_return_if_fail(user_data != NULL);

    BluezApplication *app = user_data;

    g_warning("BlueZ stack is gone, app is no longer registered");

    app->components = 0;
    app->onUnregistered(app, app->callbackData);
}

BluezApplication *bluezApplicationHelperCreate(const char *adapterPath,
                                               GDBusObjectManagerServer *objectManager,
                                               const char *leAdvertisementPath,
                                               BluezApplicationRegisteredFunc onRegistered,
                                               BluezApplicationUnregisteredFunc onUnregistered,
                                               void *callbackData)
{
    g_return_val_if_fail(adapterPath, NULL);

    int componentMask = BLUEZ_COMPONENT_NONE;

    if (objectManager != NULL)
    {
        componentMask |= BLUEZ_COMPONENT_GATT;
    }

    char *leAdvPath = NULL;

    if (leAdvertisementPath != NULL)
    {
        componentMask |= BLUEZ_COMPONENT_LE_ADV;
        leAdvPath = strdup(leAdvertisementPath);

        g_return_val_if_fail(leAdvPath != NULL, NULL);
    }

    g_return_val_if_fail(componentMask != BLUEZ_COMPONENT_NONE, NULL);

    BluezApplication *app = g_atomic_rc_box_new0(BluezApplication);

    app->adapterPath = strdup(adapterPath);
    app->objectManager = objectManager;
    app->componentMask = componentMask;
    app->components = BLUEZ_COMPONENT_NONE;
    app->leAdvPath = leAdvPath;
    app->onRegistered = onRegistered;
    app->onUnregistered = onUnregistered;
    app->callbackData = callbackData;
    app->expectedContext = g_main_context_ref_thread_default();

    app->bluezWatchId = g_bus_watch_name(G_BUS_TYPE_SYSTEM,
                                         BLUEZ_BUS_NAME,                        /* name */
                                         G_BUS_NAME_WATCHER_FLAGS_AUTO_START,
                                         NULL,                                  /* name_appeared_handler */
                                         onBluezGone,                           /* name_vanished_handler */
                                         app,                                   /* user_data */
                                         NULL                                   /* user_data_free_func */);

    return app;
}

void bluezApplicationHelperRegister(BluezApplication *bluezCtx)
{
    g_return_if_fail(bluezCtx);
   // g_assert(g_main_context_is_owner(bluezCtx->expectedContext));
#if 1
    if (bluezCtx->componentMask & BLUEZ_COMPONENT_GATT)
    {
        g_message("Registering BlueZ GATT application at '%s'", getRootPath(bluezCtx));

        bluezCtx->gattManager = bluezApplicationHelperGetGATTManager(bluezCtx->adapterPath);

        bluez_gatt_manager1_call_register_application(bluezCtx->gattManager,
                                                      getRootPath(bluezCtx),
                                                      g_variant_new_parsed("@a{sv} {}"), /* options */
                                                      NULL,                              /* cancellable */
                                                      (GAsyncReadyCallback)onGATTRegistered,
                                                      bluezApplicationHelperAcquire(bluezCtx) /* user_data */);
    }
    if (bluezCtx->componentMask & BLUEZ_COMPONENT_LE_ADV)
    {
        g_message("Registering BlueZ LE Advertisement at '%s'", bluezCtx->leAdvPath);

        bluezCtx->leAdvertisingManager = bluezApplicationHelperGetLEAdvertisingManager(bluezCtx->adapterPath);

        bluez_leadvertising_manager1_call_register_advertisement(bluezCtx->leAdvertisingManager,
                                                                 bluezCtx->leAdvPath,
                                                                 g_variant_new_parsed("@a{sv} {}"), /* options */
                                                                 NULL,                              /* cancellable */
                                                                 (GAsyncReadyCallback)onLEAdvertisementRegistered,
                                                                 bluezApplicationHelperAcquire(bluezCtx) /* user_data */);
    }
#endif

// sync way, todo. if sync way, it will encounter bluez timeout error
#if 0
    int ret = -1;
    g_autoptr(GError) error = NULL;
    if (bluezCtx->componentMask & BLUEZ_COMPONENT_GATT)
    {
        g_message("Registering BlueZ GATT application at '%s'", getRootPath(bluezCtx));

        bluezCtx->gattManager = bluezApplicationHelperGetGATTManager(bluezCtx->adapterPath);
        if (bluez_gatt_manager1_call_register_application_sync(bluezCtx->gattManager,
                                                               getRootPath(bluezCtx),
                                                               g_variant_new_parsed("@a{sv} {}"), /* options */
                                                               NULL,                              /* cancellable */
                                                               &error))
        {
            g_message("GATT registered at '%s'", getRootPath(bluezCtx));

            if (setRegistered(bluezCtx, BLUEZ_COMPONENT_GATT) && bluezCtx->onRegistered)
            {
                bluezCtx->onRegistered(bluezCtx, bluezCtx->callbackData);
                ret = 0;
            }
        }
        else
        {
            g_warning("GATT registration failed: '%s'", error->message);
            bluezApplicationHelperUnregister(bluezCtx);
            ret = -1;
        }
    }

    if (bluezCtx->componentMask & BLUEZ_COMPONENT_LE_ADV)
    {
        g_message("Registering BlueZ LE Advertisement at '%s'", bluezCtx->leAdvPath);

        bluezCtx->leAdvertisingManager = bluezApplicationHelperGetLEAdvertisingManager(bluezCtx->adapterPath);

        if (bluez_leadvertising_manager1_call_register_advertisement_sync(bluezCtx->leAdvertisingManager,
                                                                          bluezCtx->leAdvPath,
                                                                          g_variant_new_parsed("@a{sv} {}"), /* options */
                                                                          NULL,                              /* cancellable */
                                                                          &error))
        {
            g_message("Registered LE advertisement at '%s'", bluezCtx->leAdvPath);
            if (setRegistered(bluezCtx, BLUEZ_COMPONENT_LE_ADV) && bluezCtx->onRegistered)
            {
                bluezCtx->onRegistered(bluezCtx, bluezCtx->callbackData);
                ret = 0;
            }
        }
        else
        {
            g_warning("Failed to register LE advertisement at '%s': %s", bluezCtx->leAdvPath, error->message);

            bluezApplicationHelperUnregister(bluezCtx);
            ret = -2;
        }
    }
    g_message("Registered with res : %d", ret);
    // return ret;
#endif
}

static bool setRegistered(BluezApplication *bluez, enum BluezComponent which)
{
    bluez->components |= which;

    return (bluez->components & bluez->componentMask) == bluez->componentMask;
}

static bool setUnregistered(BluezApplication *bluez, enum BluezComponent which)
{
    bluez->components &= ~(which);

    return (bluez->components & bluez->componentMask) == 0;
}

static void bluezApplicationHelperDestroy(BluezApplication *app)
{
    if (app->leAdvPath == NULL || app->adapterPath == NULL)
    {
        g_message("leAdvPath or adapterPath is not valid");
        return;
    }

    free(app->leAdvPath);
    app->leAdvPath = NULL;

    free(app->adapterPath);
    app->adapterPath = NULL;

    if (app->leAdvertisingManager)
    {
        g_object_unref(app->leAdvertisingManager);
        app->leAdvertisingManager = NULL;
    }

    if (app->gattManager)
    {
        g_object_unref(app->gattManager);
        app->gattManager = NULL;
    }

    if (app->objectManager)
    {
        g_object_unref(app->objectManager);
        app->objectManager = NULL;
    }

    if (app->bluezWatchId)
    {
        g_bus_unwatch_name(app->bluezWatchId);
        app->bluezWatchId = 0;
    }
}

extern inline BluezApplication *bluezApplicationHelperAcquire(BluezApplication *app);

void bluezApplicationHelperRelease(BluezApplication *app)
{
    if (app != NULL)
    {
        g_atomic_rc_box_release_full(app, (GDestroyNotify) bluezApplicationHelperDestroy);
    }
}

#if 1
// sync way
void bluezApplicationHelperUnregister(BluezApplication *ctx)
{
    g_return_if_fail(ctx);
    g_return_if_fail(ctx->adapterPath);
    // g_assert(g_main_context_is_owner(ctx->expectedContext));

    /*
     * Do these first before dispatching any async unregistration,
     * since operations will be sequential in this main loop thread
     */
    g_ref_count_init(&ctx->busy);
    g_ref_count_inc(&ctx->busy);
    g_autoptr(GError) error = NULL;

    if (ctx->components & BLUEZ_COMPONENT_GATT)
    {
        ctx->gattManager = bluezApplicationHelperGetGATTManager(ctx->adapterPath);
        const char *rootPath = getRootPath(ctx);
        if (bluez_gatt_manager1_call_unregister_application_sync(ctx->gattManager,
                                                                 rootPath,
                                                                 NULL, /* cancellable */
                                                                 &error))
        {
            g_message("GATT at '%s' unregistered", rootPath);
            setUnregistered(ctx, BLUEZ_COMPONENT_GATT);
        }
        else
        {
            g_warning("GATT unregistration at '%s' failed: %s", rootPath, error->message);
        }
    }
    else
    {
        onApplicationUnregistered(ctx);
    }

    if (ctx->components & BLUEZ_COMPONENT_LE_ADV)
    {
        ctx->leAdvertisingManager = bluezApplicationHelperGetLEAdvertisingManager(ctx->adapterPath);

        if (bluez_leadvertising_manager1_call_unregister_advertisement_sync(ctx->leAdvertisingManager,
                                                                            ctx->leAdvPath, /* advertisementPath */
                                                                            NULL,           /* cancellable */
                                                                            &error))
        {
            g_message("Unregistered LE advertisement at '%s'", ctx->leAdvPath);

            g_object_unref(ctx->leAdvertisingManager);
            ctx->leAdvertisingManager = NULL;

            onApplicationUnregistered(ctx);

            bluezApplicationHelperRelease(ctx);
        }
        else
        {
            g_message("Notice: Failed to unregister LE advertisement at '%s': %s", ctx->leAdvPath, error->message);
        }
    }
    else
    {
        onApplicationUnregistered(ctx);
        bluezApplicationHelperRelease(ctx);
    }
    g_message("Exit bluezApplicationHelperUnregister .. \n");
}
#endif

#if 0
void bluezApplicationHelperUnregister(BluezApplication *ctx)
{
    g_return_if_fail(ctx);
    g_return_if_fail(ctx->adapterPath);
   // g_assert(g_main_context_is_owner(ctx->expectedContext));

    /*
     * Do these first before dispatching any async unregistration,
     * since operations will be sequential in this main loop thread
     */
    g_ref_count_init(&ctx->busy);
    g_ref_count_inc(&ctx->busy);

    if (ctx->components & BLUEZ_COMPONENT_GATT)
    {
        ctx->gattManager = bluezApplicationHelperGetGATTManager(ctx->adapterPath);

        bluez_gatt_manager1_call_unregister_application(ctx->gattManager,
                                                        getRootPath(ctx),
                                                        NULL,                                       /* cancellable */
                                                        (GAsyncReadyCallback) onGATTUnregistered,
                                                        bluezApplicationHelperAcquire(ctx)          /* user_data */);
    }
    else
    {
        onApplicationUnregistered(ctx);
    }

    if (ctx->components & BLUEZ_COMPONENT_LE_ADV)
    {
        ctx->leAdvertisingManager =  bluezApplicationHelperGetLEAdvertisingManager(ctx->adapterPath);

        bluez_leadvertising_manager1_call_unregister_advertisement(ctx->leAdvertisingManager,
                                                                   ctx->leAdvPath,                    /* advertisementPath */
                                                                   NULL,                              /* cancellable */
                                                                   (GAsyncReadyCallback) onLEAdvertisementUnregistered,
                                                                   bluezApplicationHelperAcquire(ctx) /* user_data */);
    }
    else
    {
        onApplicationUnregistered(ctx);
    }
}
#endif

static void onApplicationUnregistered(BluezApplication *app)
{
    g_return_if_fail(app);

    if (g_ref_count_dec(&app->busy) && app->onUnregistered)
    {
        app->onUnregistered(app, app->callbackData);
    }
}

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
 * agentHelper.c
 * Implementation of agent over Bluetooth
*/


#include "bluezAPICommon.h"
#define G_LOG_DOMAIN _LOG_DOMAIN "-agentHelper"

#include <stdio.h>
#include <gio/gio.h>
#include <gdbusBluez.h>
#include <bluez-dbus.h>
#include <util/agentHelper.h>

#define BLUEZ_PATH "/org/bluez"
#define AGENTHELPER_G_OBJECT_REFCOUNT(obj) (((GObject*)(obj))->ref_count)
#define AGENTHELPER_REFCOUNT_VALUE(obj) g_atomic_int_get ((gint *) &AGENTHELPER_G_OBJECT_REFCOUNT(obj))

static const char *gAgentPath = NULL;
static BluezAgentManager1 *gAgentMgr1Proxy = NULL;
static GDBusConnection *gConn = NULL;
static GDBusObjectManagerServer *gAgentSrv = NULL;
static BluezObjectSkeleton *gAgentObjSkeleton = NULL;
static BluezAgent1 *gAgentIface = NULL;
static gboolean agentInit = 0;
static struct agentMethodCb *gAgtMethodCb = NULL;
static void *gAgtApstBtIfceHdl = NULL;
static int gCbRes = 0;

static gboolean onRequestConfirmation(BluezAgent1 *object,
                                      GDBusMethodInvocation *invocation,
                                      const gchar *arg_device,
                                      guint arg_passkey)
{
    g_message("RequestConfirmation pairing.");
    g_message("Path: %s Pass: %d", arg_device, arg_passkey);
    gCbRes = gAgtMethodCb->reqConfirmation(arg_device, arg_passkey, gAgtApstBtIfceHdl);
    g_message("Confirmation accept: %d", gCbRes);

    if (gCbRes == 1)
    {
        g_message("Request confirmed.");
        bluez_agent1_complete_request_confirmation(object, invocation);
    }
    else
    {
        g_message("Request denied.");
        g_dbus_method_invocation_return_error(invocation,
                                              G_DBUS_ERROR,
                                              G_DBUS_ERROR_ACCESS_DENIED,
                                              "Confirmation denied!");
    }
    gCbRes = 0;

    return TRUE;
}

static gboolean onAuthService(BluezAgent1 *object,
                              GDBusMethodInvocation *invocation,
                              const gchar *arg_device)
{
    g_message("Auth Service at path %s.", arg_device);
	gCbRes = gAgtMethodCb->reqAuth(arg_device, gAgtApstBtIfceHdl);
    g_message("Auth accept: %d", gCbRes);
    if (gCbRes == 1)
    {
        g_message("Auth request confirmed.");
        bluez_agent1_complete_authorize_service(object, invocation);
    }
    else
    {
        g_message("Auth request denied.");
        g_dbus_method_invocation_return_error(invocation,
                                              G_DBUS_ERROR,
                                              G_DBUS_ERROR_ACCESS_DENIED,
                                              "Auth Service denied!");
    }
    gCbRes = 0;
    
    return TRUE;
}

static gboolean onRequestAuthorization(BluezAgent1 *object,
                                       GDBusMethodInvocation *invocation,
                                       const gchar *arg_device)
{
    g_message("RequestAuthorization pairing.");
    g_message("Path: %s", arg_device);
    gCbRes = gAgtMethodCb->reqAuth(arg_device, gAgtApstBtIfceHdl);
    g_message("Auth accept: %d", gCbRes);
    if (gCbRes)
    {
        g_message("Authorization request.");
        bluez_agent1_complete_request_authorization(object, invocation);
    }
    else
    {
        g_message("Authorization denied.");
        g_dbus_method_invocation_return_error(invocation,
                                              G_DBUS_ERROR,
                                              G_DBUS_ERROR_ACCESS_DENIED,
                                              "Authorization denied!");
    }
    gCbRes = 0;

    return TRUE;
}

static gboolean onDisplayPasskey(BluezAgent1 *object,
                                 GDBusMethodInvocation *invocation,
                                 const gchar *arg_device,
                                 guint arg_passkey,
                                 guint16 arg_entered)
{
    g_message("DisplayPasskey pairing.");
    g_message("Path: %s Pass: %d Entered: %d", arg_device, arg_passkey, arg_entered);
    bluez_agent1_complete_display_passkey(object, invocation);
    return TRUE;
}

static gboolean onRequestPasskey(BluezAgent1 *object,
                                 GDBusMethodInvocation *invocation,
                                 const gchar *arg_device,
                                 guint arg_passkey,
                                 guint16 arg_entered)
{
    g_message("Getting the Pin from user: ");
    arg_passkey = gAgtMethodCb->reqPasskey(arg_device, gAgtApstBtIfceHdl);
    g_message("Passkey is: %d\n", arg_passkey);
    bluez_agent1_complete_request_passkey(object, invocation, arg_passkey);

    return TRUE;
}

static gboolean onCancel(BluezAgent1 *object,
                         GDBusMethodInvocation *invocation)
{
    g_message("Pairing canceled.");
    bluez_agent1_complete_cancel(object, invocation);
    return TRUE;
}

static gboolean onRelease(BluezAgent1 *object,
                          GDBusMethodInvocation *invocation)
{
    bluez_agent1_complete_release(object, invocation);
    return TRUE;
}

static void agent_method_register(void)
{
    if (NULL == gConn)
    {
        gConn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, NULL);
    }

    if (gAgentSrv == NULL)
    {
        gAgentSrv = g_dbus_object_manager_server_new(BLUEZ_PATH);
    }
    g_message("Create agent object at path '%s'", gAgentPath);
    gAgentObjSkeleton = bluez_object_skeleton_new(gAgentPath);
    gAgentIface = bluez_agent1_skeleton_new();
    bluez_object_skeleton_set_agent1(gAgentObjSkeleton, gAgentIface);
    g_dbus_object_manager_server_export(gAgentSrv, G_DBUS_OBJECT_SKELETON(gAgentObjSkeleton));
    g_dbus_object_manager_server_set_connection(gAgentSrv, gConn);

    g_signal_connect(gAgentIface, "handle-request-confirmation", G_CALLBACK(onRequestConfirmation), gAgentIface);
    g_signal_connect(gAgentIface, "handle-request-authorization", G_CALLBACK(onRequestAuthorization), gAgentIface);
    g_signal_connect(gAgentIface, "handle-display-passkey", G_CALLBACK(onDisplayPasskey), gAgentIface);
    g_signal_connect(gAgentIface, "handle-request-passkey", G_CALLBACK(onRequestPasskey), gAgentIface);
    g_signal_connect(gAgentIface, "handle-authorize-service", G_CALLBACK(onAuthService), gAgentIface);
    g_signal_connect(gAgentIface, "handle-cancel", G_CALLBACK(onCancel), gAgentIface);
    g_signal_connect(gAgentIface, "handle-release", G_CALLBACK(onRelease), gAgentIface);
}

static BluezAgentManager1 *bluezAgentHelperGetAgentManager(GDBusConnection *con, const char *path)
{
    g_autoptr(GError) error = NULL;
    g_return_val_if_fail(path != NULL, NULL);
    BluezAgentManager1 *proxy = NULL;

    g_message("Getting AgentManager1 at path '%s'", path);
    proxy = bluez_agent_manager1_proxy_new_sync(
        gConn,
        G_DBUS_PROXY_FLAGS_NONE,
        BLUEZ_BUS_NAME, /* bus name */
        path,
        NULL, /* cancellable */
        &error);

    if (error != NULL)
    {
        g_critical("Could not get agent manager: %s", error->message);
    }

    return proxy;
}

static void onAgentRegistered(GDBusMethodInvocation *invocation, GAsyncResult *res)
{
    g_autoptr(GError) error = NULL;

    gboolean registered = bluez_agent_manager1_call_register_agent_finish(gAgentMgr1Proxy, res, &error);

    if (registered)
    {
        g_message("Registered agent.");
    }
    else
    {
        g_warning("Agent registration failed: '%s'", error->message);

        bluezAgentHelperUnregister(BLUEZ_PATH, gAgentPath);
    }
}

static void onDefaultAgentRegistered(GDBusMethodInvocation *invocation, GAsyncResult *res)
{
    g_autoptr(GError) error = NULL;

    gboolean registered = bluez_agent_manager1_call_request_default_agent_finish(gAgentMgr1Proxy, res, &error);

    if (registered)
    {
        g_message("Registered default agent.");
        agentInit = registered;
    }
    else
    {
        g_warning("Default agent registration failed: '%s'", error->message);

        bluezAgentHelperUnregister(BLUEZ_PATH, gAgentPath);
    }
}

static void agentManagerRegister(const char *cap)
{
    gAgentMgr1Proxy = bluezAgentHelperGetAgentManager(gConn, BLUEZ_PATH);
    if (NULL != gAgentMgr1Proxy)
    {
        bluez_agent_manager1_call_register_agent(
            gAgentMgr1Proxy,
            gAgentPath,
            (gchar *)cap, /* options */
            NULL,         /* cancellable */
            (GAsyncReadyCallback)onAgentRegistered,
            NULL /* user_data */);

        bluez_agent_manager1_call_request_default_agent(
            gAgentMgr1Proxy,
            gAgentPath,
            NULL, /* cancellable */
            (GAsyncReadyCallback)onDefaultAgentRegistered,
            NULL /* user_data */);
    }
}

void bluezAgentHelperRegister(const char *adapterPath,
                              const char *agentPath,
                              const char *capability,
                              struct agentMethodCb *methodCb,
                              void *apstBtIfceHdl)
{
    g_message("Registering BlueZ Agent at '%s' with Capability \"%s\"",
              BLUEZ_PATH, capability);
    if (NULL == agentPath)
    {
        gAgentPath = "/org/bluez/agent";
    }
    else
    {
        gAgentPath = agentPath;
    }

    if (!agentInit)
    {
        agent_method_register();
        agentManagerRegister(capability);

        gAgtApstBtIfceHdl = apstBtIfceHdl;
        gAgtMethodCb = methodCb;
    }else
    {
        g_message("Agent has been inited");
    }
}

static void releaseObj(gpointer obj)
{
    if (NULL != obj)
    {
        int cnt = AGENTHELPER_REFCOUNT_VALUE(obj);
        while (cnt-- != 0)
        {
            g_object_unref(obj);
        }
    }
}

static void agentDeInit(void)
{
    releaseObj(G_OBJECT(gAgentObjSkeleton));
    releaseObj(G_OBJECT(gAgentIface));
    releaseObj(G_OBJECT(gAgentSrv));
    releaseObj(G_OBJECT(gAgentMgr1Proxy));

    gAgentIface = NULL;
    gAgentSrv = NULL;
    gAgentObjSkeleton = NULL;
    gAgentMgr1Proxy = NULL;

    gAgentPath = NULL;
    gConn = NULL;
    agentInit = FALSE;
    gAgtApstBtIfceHdl = NULL;
}

void bluezAgentHelperUnregister(const char *adapterPath,
                                const char *agentPath)
{
    if (NULL == agentPath)
    {
        g_warning("Empty agent path!");
        return;
    }

    if (gAgentMgr1Proxy == NULL || gAgentSrv == NULL)
    {
        g_message("No agent registered.");
    }
    else
    {
        g_message("Unregister agent at path \"%s\".", agentPath);
        if (!g_dbus_object_manager_server_unexport(gAgentSrv, agentPath))
        {
            g_warning("Failed to remove object from object manager.");
        }

        if (bluez_agent_manager1_call_unregister_agent_sync(
                gAgentMgr1Proxy,
                agentPath,
                NULL, /* cancellable */
                NULL /* user_data */))
        {
            agentDeInit();
        }
    }
}

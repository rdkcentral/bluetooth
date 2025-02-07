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
 * btr_leAdvGDBusTest.c
 * Test for Advertisement
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gio/gio.h>
#include <glib.h>
#include <glib-unix.h>
#include <util/applicationHelper.h>
#include <util/agentHelper.h>

#include "gatt.h"

#include <getopt.h>
#include <glib/gprintf.h>
#include <util/adapterUtils.h>

#include <stdint.h>
#include <gatt/serviceAdvertisement.h>


typedef struct _bleconfd_t {
  GDBusConnection *bus;
  GMainLoop *main_loop;
  guint owner_id;
  gchar* bus_name;
  GBusType bus_type;
  BluezAdapter1 *adapter;
  GDBusObjectManagerServer *object_manager;
  gchar *bus_root_path;
  BluezApplication *app;
} bleconfd_t;


G_DEFINE_QUARK(bleconfd-quark, bleconfd_error)

static void bleconfd_on_bus_acquired(GDBusConnection *bus, const gchar *name, gpointer user_data);
static void bleconfd_on_name_acquired(GDBusConnection *bus, const gchar *name, gpointer user_data);
static void bleconfd_on_name_lost(GDBusConnection *bus, const char *name, gpointer user_data);
static void bleconfd_dump_context(bleconfd_t *ctx);
static void bleconfd_on_app_registered(BluezApplication *app, gpointer user_data);
static void bleconfd_on_app_unregistered(BluezApplication *app, gpointer user_data);
static int  bleconfd_on_BTReqPasskeyCB (const gchar *lpcPath, void *apvUserData);
static int  bleconfd_on_BTReqAutherizationCB (const gchar *lpcPath, void *apvUserData);
static int  bleconfd_on_BTReqConfirmationCB (const gchar *lpcPath, guint ui32PassCode, void *apvUserData);

void app_advertisement_setup(BluezLEAdvertisement1 *advertisement);
int agent_init(const char *capability);
static const char *agent_capability = NULL;
GMainLoop *loop;


static struct agentMethodCb agtIfaceCb = {
    .reqConfirmation = bleconfd_on_BTReqConfirmationCB,
    .reqAuth         = bleconfd_on_BTReqAutherizationCB,
    .reqPasskey      = bleconfd_on_BTReqPasskeyCB,
};

  
static void cleanup_handler(int signo)
{
	if (signo == SIGINT) {
		g_print("received SIGINT\n");
		g_main_loop_quit(loop);
	}
}

int main(int argc, char *argv[])
{
  bleconfd_t *ctx = g_new0(bleconfd_t, 1);
  ctx->bus_type = G_BUS_TYPE_SYSTEM;

  
  if(signal(SIGINT, cleanup_handler) == SIG_ERR)
	g_info("can't catch SIGINT\n");
		
  while (1) {
    static struct option long_options[] = {
      {"bus-name", required_argument, 0, 'b'},
      {"system", no_argument, 0, 'y'},
      {"session", no_argument, 0, 'e'},
      {"root-object-path", required_argument, 0, 'r'},
      {"appname", required_argument, 0, 'a'},
      {"capability", required_argument, 0, 'c'},
      {0, 0, 0, 0}
    };

    int option_index = 0;
    int opt = getopt_long(argc, argv, "a:en:y", long_options, &option_index);
    if (opt == -1)
      break;
    switch (opt) {
      case 'b':
        ctx->bus_name = g_strdup(optarg);
        break;
      case 'y':
        ctx->bus_type = G_BUS_TYPE_SYSTEM;
        break;
      case 'e':
        ctx->bus_type = G_BUS_TYPE_SESSION;
        break;
      case 'r':
        ctx->bus_root_path = g_strdup(optarg);
        break;
      case 'c':
      	agent_capability = g_strdup(optarg);
        break;
      case '?':
        break;
    }
  }

  if (ctx->bus_name == NULL)
    ctx->bus_name = g_strdup("org.rdk.bluetooth");
  if (ctx->bus_root_path == NULL)
    ctx->bus_root_path = g_strdup("/org/rdk/bluetooth/GattServer");

  bleconfd_dump_context(ctx);
  gatt_char_obj_init();

  ctx->main_loop = g_main_loop_new(NULL, FALSE);
  loop = ctx->main_loop;
  ctx->owner_id = g_bus_own_name(
    ctx->bus_type,
    ctx->bus_name,
    G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT | G_BUS_NAME_OWNER_FLAGS_REPLACE,
    bleconfd_on_bus_acquired,
    bleconfd_on_name_acquired,
    bleconfd_on_name_lost,
    ctx,
    NULL);

//  agent_init(agent_capability); //for testing, check the agent.c
  
  g_info("running main loop");
  g_main_loop_run(ctx->main_loop);

  if (ctx->owner_id != 0)
    g_bus_unown_name(ctx->owner_id);
  if (ctx->bus)
    g_object_unref(ctx->bus);
  if (ctx->main_loop)
    g_main_loop_unref(ctx->main_loop);
  g_free(ctx);

  gatt_char_obj_release();

  return 0;
}

void bleconfd_on_bus_acquired(GDBusConnection *bus, const gchar *name, gpointer user_data)
{
  g_info("on_bus_acquired:%s", name);

  bleconfd_t *ctx = (bleconfd_t *) user_data;
  ctx->bus = bus;
  ctx->adapter = bluezAdapterUtilsGetDefault();
  ctx->object_manager = g_dbus_object_manager_server_new(ctx->bus_root_path);

  char advertisement_object_path[256];
  g_sprintf(advertisement_object_path, "%s/Advertisement", ctx->bus_root_path);

  ctx->app = bluezApplicationHelperCreate(
    bluezAdapterUtilsGetObjectPath(ctx->adapter),
    g_object_ref(ctx->object_manager),
    advertisement_object_path,
    (BluezApplicationRegisteredFunc) bleconfd_on_app_registered,
    (BluezApplicationUnregisteredFunc) bleconfd_on_app_unregistered,
    ctx);

  // 1. build services
  BluezObjectSkeleton *device_info_service =
      device_information_service_export(ctx->object_manager, ctx->bus_root_path);
  (void)device_info_service;
  
#ifdef LE_MODE
  BluezObjectSkeleton *rdk_setup_service =
      rdk_setup_service_export(ctx->object_manager, ctx->bus_root_path);
  (void)rdk_setup_service;
#endif

  // g_autofree gchar* path = NULL;
  // g_object_get(G_OBJECT(device_info), "g-object-path", &path, NULL);
  // g_autofree gchar* uuid = NULL;
  //  g_object_get(G_OBJECT(device_info), "UUID", &uuid, NULL);

  // 2. create advertisement (TODO): advertise the RDK setup service as the primary
  // service, not deviceinfo
  const char *const advertised_services[] = {
      "0x180a",
      NULL};

  // TODO: where should localname come from?
  gchar *local_name = "MY_BEACON";
  g_autoptr(BluezLEAdvertisement1) adv = serviceAdvertisementCreate(
      advertised_services,
      local_name,
      true,
      5);
  app_advertisement_setup(adv);

  // 3. export all objects
  g_autoptr(BluezObjectSkeleton) advertisement_skeleton =
      bluez_object_skeleton_new(advertisement_object_path);
  bluez_object_skeleton_set_leadvertisement1(advertisement_skeleton, adv);
  g_dbus_object_manager_server_export(ctx->object_manager,
                                      G_DBUS_OBJECT_SKELETON(advertisement_skeleton));

  g_dbus_object_manager_server_set_connection(ctx->object_manager, ctx->bus);
}

void bleconfd_on_name_acquired(GDBusConnection * __unused(bus), const gchar *name, gpointer user_data)
{
  g_info("on_name_acquired:%s", name);

  bleconfd_t *ctx = (bleconfd_t *) user_data;
  bluezApplicationHelperRegister(ctx->app);
  bluezAgentHelperRegister("/org/bluez", "/org/bluez/agent", agent_capability, &agtIfaceCb, &user_data);
}

void bleconfd_on_name_lost(GDBusConnection * __unused(bus), const char *name, gpointer user_data)
{
  g_info("on_name_lost:%s", name);

  bleconfd_t *ctx = (bleconfd_t *) user_data;
  bluezApplicationHelperUnregister(ctx->app);
  bluezAgentHelperUnregister("/org/bluez", "/org/bluez/agent");
}

void bleconfd_dump_context(bleconfd_t *ctx)
{
  g_info("bus_name    : %s", ctx->bus_name);
  g_info("bus_type    : %s", (ctx->bus_type == G_BUS_TYPE_SYSTEM ? "system" : "session"));
  g_info("root_path   : %s", ctx->bus_root_path);
}

void bleconfd_on_app_registered(BluezApplication * __unused(app),
  gpointer __unused(user_data))
{
  g_info("bleconfd_on_app_registered");
}

void bleconfd_on_app_unregistered(BluezApplication * __unused(app),
  gpointer __unused(user_data))
{
  g_info("bleconfd_on_app_unregistered");
}


static int
bleconfd_on_BTReqPasskeyCB (
    const gchar *lpcPath,
    void *apvUserData
) {
    unsigned int ui32PassCode = 0;
    return ui32PassCode;
}


static int
bleconfd_on_BTReqAutherizationCB (
    const gchar *lpcPath,
    void *apvUserData
) {
    int yesNo = 1;
    return yesNo;
}

static int
bleconfd_on_BTReqConfirmationCB (
    const gchar *lpcPath,
    guint ui32PassCode,
    void *apvUserData
) {
    int yesNo = 1;
    return yesNo;
}

##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2016 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################
SUBDIRS = 

BTRCORE_IFCE_GDBUS_GEN_SRC = gdbusFreedesktop.c
BTRCORE_IFCE_GDBUS_GEN_SRC += gdbusBluez.c

BTRCORE_IFCE_GDBUS_SRC = dbusHelper/dbusUtil.c
BTRCORE_IFCE_GDBUS_SRC += bluezAPI/util/adapterUtils.c
BTRCORE_IFCE_GDBUS_SRC += bluezAPI/util/applicationHelper.c
BTRCORE_IFCE_GDBUS_SRC += bluezAPI/util/agentHelper.c
BTRCORE_IFCE_GDBUS_SRC += bluezAPI/gatt/serviceAdvertisement.c
BTRCORE_IFCE_GDBUS_SRC += bleconfd/btr_leAdvGDBus.c
BTRCORE_IFCE_GDBUS_SRC += bleconfd/deviceinfo.c
BTRCORE_IFCE_GDBUS_SRC += bleconfd/gatt.c
BTRCORE_IFCE_GDBUS_SRC += bleconfd/setupservice.c
BTRCORE_IFCE_GDBUS_SRC += platform/btrCore_platform_dis_cb.c


SPECS_PATH = $(top_srcdir)/specs

DBUS_FD_SP_PATH = $(SPECS_PATH)/dbus
DBUS_FD_SPECS = $(DBUS_FD_SP_PATH)/org.freedesktop.xml

BLUEZ5_SP_PATH = $(SPECS_PATH)/bluez5_54
BLUEZ5_SPECS = $(BLUEZ5_SP_PATH)/org.bluez_Adapter1.xml
BLUEZ5_SPECS += $(BLUEZ5_SP_PATH)/org.bluez_AdvertisingManager1.xml
BLUEZ5_SPECS += $(BLUEZ5_SP_PATH)/org.bluez_Agent1.xml
BLUEZ5_SPECS += $(BLUEZ5_SP_PATH)/org.bluez_AgentManager1.xml
BLUEZ5_SPECS += $(BLUEZ5_SP_PATH)/org.bluez_Device1.xml
BLUEZ5_SPECS += $(BLUEZ5_SP_PATH)/org.bluez_GattCharacteristic1.xml
BLUEZ5_SPECS += $(BLUEZ5_SP_PATH)/org.bluez_GattDescriptor1.xml
BLUEZ5_SPECS += $(BLUEZ5_SP_PATH)/org.bluez_GattManager1.xml
BLUEZ5_SPECS += $(BLUEZ5_SP_PATH)/org.bluez_GattProfile1.xml
BLUEZ5_SPECS += $(BLUEZ5_SP_PATH)/org.bluez_GattService1.xml
BLUEZ5_SPECS += $(BLUEZ5_SP_PATH)/org.bluez_LEAdvertisement1.xml
BLUEZ5_SPECS += $(BLUEZ5_SP_PATH)/org.bluez_Media1.xml
BLUEZ5_SPECS += $(BLUEZ5_SP_PATH)/org.bluez_NetworkManager1.xml
BLUEZ5_SPECS += $(BLUEZ5_SP_PATH)/org.bluez_ProfileManager1.xml


BTRCORE_IFCE_GDBUS_INC = ${top_srcdir}/include/bt-ifce/gdbus
BTRCORE_IFCE_PLATFORM_INC = ${top_srcdir}/soc/include

AM_CPPFLAGS = -Wall -I$(BTRCORE_IFCE_PLATFORM_INC) -I$(BTRCORE_IFCE_GDBUS_INC) -I$(BTRCORE_IFCE_GDBUS_INC)/dbusHelper -I$(BTRCORE_IFCE_GDBUS_INC)/bluezAPI -I$(BTRCORE_IFCE_GDBUS_INC)/bleconfd

lib_LTLIBRARIES = libbtrCoreIfceGdbus.la
libbtrCoreIfceGdbus_la_SOURCES = $(BTRCORE_IFCE_GDBUS_GEN_SRC) $(BTRCORE_IFCE_GDBUS_SRC)
libbtrCoreIfceGdbus_la_CFLAGS = $(FFI_CFLAGS) $(GLIB_CFLAGS) $(GIO_CFLAGS) $(GIO_UNIX_CFLAGS) -I$(BTRCORE_IFCE_PLATFORM_INC)
libbtrCoreIfceGdbus_la_LIBADD = $(FFI_LIBS) $(GLIB_LIBS) $(GIO_LIBS) $(GIO_UNIX_LIBS)


gdbusFreedesktop.c: $(DBUS_FD_SPECS)
	$(GDBUS_CODEGEN) --header --interface-prefix org.freedesktop --c-namespace "" --c-generate-autocleanup all --output $(BTRCORE_IFCE_GDBUS_INC)/gdbusFreedesktop.h  $(DBUS_FD_SPECS)
	$(GDBUS_CODEGEN) --body --interface-prefix org.freedesktop --c-namespace "" --c-generate-autocleanup all --output gdbusFreedesktop.c  $(DBUS_FD_SPECS)

gdbusBluez.c: $(BLUEZ5_SPECS)
	$(GDBUS_CODEGEN) --header --interface-prefix org.bluez. --c-namespace Bluez --c-generate-autocleanup all --c-generate-object-manager --output $(BTRCORE_IFCE_GDBUS_INC)/gdbusBluez.h  $(BLUEZ5_SPECS)
	$(GDBUS_CODEGEN) --body --interface-prefix org.bluez. --c-namespace Bluez --c-generate-autocleanup all --c-generate-object-manager --output gdbusBluez.c   $(BLUEZ5_SPECS)

$(BTRCORE_IFCE_GDBUS_SRC): $(BTRCORE_IFCE_GDBUS_GEN_SRC)

#clean-local: clean-local-check
#.PHONY: clean-local-check
#clean-local-check:
#	-rm -f $(BTRCORE_IFCE_GDBUS_GEN_SRC) $(BTRCORE_IFCE_GDBUS_INC)/gdbusFreedesktop.h $(BTRCORE_IFCE_GDBUS_INC)/gdbusBluez.h

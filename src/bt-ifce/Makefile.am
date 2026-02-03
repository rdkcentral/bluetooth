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

INCLUDE_FILES = 

if BTR_IFCE_DBUS_BLUEZ4
BTRCORE_IFCE_SRCS = btrCore_dbus_bluez4.c
BTRCORE_IFCE_BLUEZ_INC = $(DBUS_CFLAGS)
BTRCORE_IFCE_BLUEZ_LIB = $(DBUS_LIBS)
else !BTR_IFCE_DBUS_BLUEZ4
if BTR_IFCE_DBUS_BLUEZ5
BTRCORE_IFCE_SRCS = btrCore_dbus_bluez5.c
BTRCORE_IFCE_BLUEZ_INC = $(DBUS_CFLAGS)
BTRCORE_IFCE_BLUEZ_LIB = $(DBUS_LIBS)
UDEV_LIB = -ludev
else !BTR_IFCE_DBUS_BLUEZ5
if BTR_IFCE_GDBUS_BLUEZ5
SUBDIRS += gdbus
BTRCORE_IFCE_SRCS = btrCore_gdbus_bluez5.c
BTRCORE_IFCE_BLUEZ_INC = -I${top_srcdir}/include/bt-ifce/gdbus -I${top_srcdir}/soc/include
BTRCORE_IFCE_BLUEZ_LIB = $(top_builddir)/src/bt-ifce/gdbus/libbtrCoreIfceGdbus.la
endif
endif
endif

AM_CPPFLAGS = -Wall -Werror -I${top_srcdir}/include/logger -I${top_srcdir}/include/safec -I${top_srcdir}/include/bt-ifce $(BTRCORE_IFCE_BLUEZ_INC)

if IS_LIBSYSWRAPPER_ENABLED
AM_LDFLAGS = -lsecure_wrapper
endif

lib_LTLIBRARIES = libbtrCoreIfce.la
libbtrCoreIfce_la_SOURCES = $(BTRCORE_IFCE_SRCS)
libbtrCoreIfce_la_CFLAGS = $(INCLUDE_FILES) $(GLIB_CFLAGS) $(LIBSYSWRAPPER_FLAG) $(SAFEC_CFLAGS)
libbtrCoreIfce_la_LIBADD = $(GLIB_LIBS) $(BTR_CORE_LOGGER_LIBS) $(BTRCORE_IFCE_BLUEZ_LIB) $(UDEV_LIB) $(SAFEC_LFLAGS) -ltelemetry_msgsender

if BTR_IFCE_GDBUS_BLUEZ5
libbtrCoreIfce_la_CFLAGS += $(GIO_CFLAGS) $(GIO_UNIX_CFLAGS)
libbtrCoreIfce_la_LIBADD += $(GIO_LIBS) $(GIO_UNIX_LIBS)
endif

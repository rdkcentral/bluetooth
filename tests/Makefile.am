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

AM_CPPFLAGS = -Wall -Werror -I${top_srcdir}/include -I${top_srcdir}/include/safec
bin_PROGRAMS = btrCoreTest

btrCoreTest_SOURCES = btrCore_test.c
btrCoreTest_CFLAGS = $(INCLUDE_FILES) $(GLIB_CFLAGS) $(SAFEC_CFLAGS)
btrCoreTest_LDADD = $(top_builddir)/src/libbtrCore.la $(top_builddir)/src/bt-ifce/libbtrCoreIfce.la $(GLIB_LIBS) $(SAFEC_LFLAGS)

if BTR_IFCE_GDBUS_BLUEZ5
bin_PROGRAMS += btrLeAdvGDBusTest btrLeAutoPairGDBusTest

btrLeAdvGDBusTest_SOURCES = btr_leAdvGDBusTest.c
btrLeAdvGDBusTest_CFLAGS = $(INCLUDE_FILES) -I${top_srcdir}/include/bt-ifce/gdbus -I${top_srcdir}/include/bt-ifce/gdbus/bluezAPI -I${top_srcdir}/include/bt-ifce/gdbus/bleconfd $(FFI_CFLAGS) $(GLIB_CFLAGS) $(GIO_CFLAGS) $(GIO_UNIX_CFLAGS) $(SAFEC_CFLAGS)
btrLeAdvGDBusTest_LDADD = $(top_builddir)/src/bt-ifce/gdbus/libbtrCoreIfceGdbus.la $(FFI_LIBS) $(GLIB_LIBS) $(GIO_LIBS) $(GIO_UNIX_LIBS) $(SAFEC_LFLAGS)

btrLeAutoPairGDBusTest_SOURCES = btr_leAutoPairGDBusTest.c
btrLeAutoPairGDBusTest_CFLAGS = $(INCLUDE_FILES) -I${top_srcdir}/include/bt-ifce/gdbus -I${top_srcdir}/include/bt-ifce/gdbus/bluezAPI -I${top_srcdir}/include/bt-ifce/gdbus/bleconfd $(FFI_CFLAGS) $(GLIB_CFLAGS) $(GIO_CFLAGS) $(GIO_UNIX_CFLAGS) $(SAFEC_CFLAGS)
btrLeAutoPairGDBusTest_LDADD = $(top_builddir)/src/bt-ifce/gdbus/libbtrCoreIfceGdbus.la $(FFI_LIBS) $(GLIB_LIBS) $(GIO_LIBS) $(GIO_UNIX_LIBS)  $(SAFEC_LFLAGS)
endif

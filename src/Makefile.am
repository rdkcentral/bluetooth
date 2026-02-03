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
SUBDIRS = bt-ifce

INCLUDE_FILES =

BTRCORE_IFCE_INC = -I${top_srcdir}/include/bt-ifce
AM_CPPFLAGS = -Wall -Werror -I${top_srcdir}/include -I${top_srcdir}/include/logger -I${top_srcdir}/include/safec $(BTRCORE_IFCE_INC) 

if IS_LIBSYSWRAPPER_ENABLED
AM_LDFLAGS = -lsecure_wrapper
endif

AM_LDFLAGS += -ltelemetry_msgsender

lib_LTLIBRARIES = libbtrCore.la

if LE_ONLY
libbtrCore_la_SOURCES = btrCore.c btrCore_le.c
else
libbtrCore_la_SOURCES = btrCore.c btrCore_avMedia.c btrCore_le.c
endif

libbtrCore_la_CFLAGS = $(INCLUDE_FILES) $(GLIB_CFLAGS) $(LIBSYSWRAPPER_FLAG) $(SAFEC_CFLAGS)
libbtrCore_la_LIBADD = $(GLIB_LIBS) $(BTR_CORE_LOGGER_LIBS) $(SAFEC_LFLAGS) $(top_builddir)/src/bt-ifce/libbtrCoreIfce.la

# Explicitly declare the header file U want to expose out of libbtrCore.so here
include_HEADERS = $(top_srcdir)/include/btrCoreTypes.h
include_HEADERS += $(top_srcdir)/include/btrCore.h
include_HEADERS += $(top_srcdir)/include/btrCore_service.h

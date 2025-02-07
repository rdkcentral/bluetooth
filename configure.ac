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
#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([btr-core], [1.0], [damiano.barone@sky.uk])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([cfg])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign no-dist-gzip dist-bzip2 1.9])
AC_CONFIG_HEADERS([config.h])

m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

LT_INIT
AC_LANG([C])

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])],
[AM_DEFAULT_VERBOSITY=1
AC_SUBST(AM_DEFAULT_VERBOSITY)])


# Checks for programs.
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h unistd.h])

PKG_CHECK_MODULES([DBUS], [dbus-1])


BTR_CORE_SRC="src"
BTR_CORE_TEST="tests"
BTR_CORE_LOGGER_LIBS=" "
BTR_IFCE_DBUS_BLUEZ4=" "
BTR_IFCE_DBUS_BLUEZ5=" "
BTR_IFCE_GDBUS_BLUEZ5=" "
LIBSYSWRAPPER_FLAG=" "
IS_LIBSYSWRAPPER_ENABLED=" "

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Check Btr-ifce
AC_ARG_ENABLE(btr-ifce,
              AS_HELP_STRING([--enable-btr-ifce], [Use bluez4/bluez5/gdbus_bluez5/custom]),
              [
                case "${enableval}" in
                    bluez4) have_bluez4="yes";;
                    bluez5) have_bluez5="yes";;
                    gdbus_bluez5) have_gdbusbluez5="yes";;
                    *) AC_MSG_ERROR([bad value ${enableval} for --enable-btr-ifce]) ;;
                esac
              ] ,
              [have_bluez4="yes"])

AS_IF([test "x$have_bluez4" = "xyes"], [
    PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.32.0])
    AC_DEFINE(USE_BLUEZ4, 1, [Build with bluez4 support])
], [])

AS_IF([test "x$have_bluez5" = "xyes"], [
    PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.32.0])
    AC_DEFINE(USE_BLUEZ5, 1, [Build with bluez5 support])
], [])

AS_IF([test "x$have_gdbusbluez5" = "xyes"], [
    PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.58.0])
    PKG_CHECK_MODULES([GIO], [gio-2.0 >= 2.58.0])
    PKG_CHECK_MODULES([GIO_UNIX], [gio-unix-2.0 >= 2.58.0])
	PKG_CHECK_MODULES([FFI], [libffi >= 3.0.0])
    AC_PATH_PROG(GDBUS_CODEGEN, [gdbus-codegen], [])

    AS_IF([test "x$GDBUS_CODEGEN" = "x"], [
        AC_MSG_ERROR([Could not find gdbus-codegen])
    ])

    AC_DEFINE(USE_GDBUSBLUEZ5, 1, [Build with GDBus bluez5 support])
], [])

AM_CONDITIONAL([BTR_IFCE_DBUS_BLUEZ4], [test x$have_bluez4 = xyes])
AM_CONDITIONAL([BTR_IFCE_DBUS_BLUEZ5], [test x$have_bluez5 = xyes])
AM_CONDITIONAL([BTR_IFCE_GDBUS_BLUEZ5], [test x$have_gdbusbluez5 = xyes])

AC_ARG_ENABLE([libsyswrapper],
        AS_HELP_STRING([--enable-wrapper],[enables system wrapper replacement (default is no)]),
        [
          case "${enableval}" in
           yes) IS_LIBSYSWRAPPER_ENABLED=true
                LIBSYSWRAPPER_FLAG=" -DLIBSYSWRAPPER_BUILD ";;
           no)  IS_LIBSYSWRAPPER_ENABLED=false ;;
          *) AC_MSG_ERROR([bad value ${enableval} for --enable-wrapper]) ;;
           esac
           ],
        [echo "libsyswrapper is disabled"])
AM_CONDITIONAL([IS_LIBSYSWRAPPER_ENABLED], [test x$IS_LIBSYSWRAPPER_ENABLED = xtrue])
AC_SUBST(LIBSYSWRAPPER_FLAG)

#check for safec
SAFEC_CFLAGS=" "
SAFEC_LFLAGS=" "
IS_SAFEC_ENABLED=" "
        echo "safec check"
AC_ARG_ENABLE([safec],
        AS_HELP_STRING([--enable-safec],[enables safec replacement (default is no)]),
        [
          case "${enableval}" in
           yes) IS_SAFEC_ENABLED=true
                SAFEC_CFLAGS=" `pkg-config --cflags libsafec` "
                SAFEC_LFLAGS=" `pkg-config --libs libsafec` ";;
           no)  IS_SAFEC_ENABLED=false
                SAFEC_CFLAGS="-DSAFEC_DUMMY_API";;
          *) AC_MSG_ERROR([bad value ${enableval} for --enable-safec]) ;;
           esac
           ],
        [SAFEC_CFLAGS="-DSAFEC_DUMMY_API"; echo "safec is disabled"])
AM_CONDITIONAL([IS_SAFEC_ENABLED], [test x$IS_SAFEC_ENABLED = xtrue])
AC_SUBST(SAFEC_CFLAGS)
AC_SUBST(SAFEC_LFLAGS)

# Check for rdk logger
AC_ARG_ENABLE([rdk-logger],
              AS_HELP_STRING([--enable-rdk-logger],[enable rdk logger (default is no)]),
              [
                case "${enableval}" in
                 yes) rdk_logger_ok=yes;;
                 no)  rdk_logger_ok=no;;
                 *) AC_MSG_ERROR([bad value ${enableval} for --enable-rdk-logger]) ;;
                esac
              ],
              [echo "rdk logger is disabled"])

AS_IF([test "x$rdk_logger_ok" = "xyes"], [
    AC_CHECK_LIB(rdkloggers, rdk_logger_init,
                       enable_rdk_logger=yes, enable_rdk_logger=no
                       AC_MSG_ERROR([RDK_LOGGER library (librdkloggers) not found]))
    if test "x$enable_rdk_logger" = "xyes"; then
       AC_MSG_CHECKING([for rdkloggers])
       AC_TRY_COMPILE(
    [#include <stdio.h>
    #undef PACKAGE
    #undef VERSION
    #undef HAVE_STDLIB_H
    #include "rdk_debug.h"],,
             enable_rdk_logger=yes,
             enable_rdk_logger=no)
       AC_MSG_RESULT($enable_rdk_logger)
       if test "x$enable_rdk_logger" = "xyes"; then
         AC_DEFINE(RDK_LOGGER_ENABLED, 1, [Enabled RDK Logger]) 
         BTR_CORE_LOGGER_LIBS="-lrdkloggers"
       else
          AC_MSG_ERROR([RDK_LOGGER library (librdkloggers) not found])
       fi
    fi
], [])

AC_ARG_ENABLE([streaming-in],
              AS_HELP_STRING([--enable-streaming-in],[enable streaming in (a2dp sink) (default is yes)]),
              [
               case "${enableval}" in
                yes) ENABLE_STREAMING_IN=true ;;
                no)  ENABLE_STREAMING_IN=false ;;
                *) AC_MSG_ERROR([bad value ${enableval} for --enable-streaming-in]) ;;
               esac
               if test "$ENABLE_STREAMING_IN" = true; then
                 AC_DEFINE(STREAM_IN_SUPPORTED, 1, [Enabled ENABLE_STREAMING_IN flag])
               else
                [echo "Not enabled auto connect feature flag"]
               fi
              ],
              [
               ENABLE_STREAMING_IN=true
               AC_DEFINE(STREAM_IN_SUPPORTED, 1, [Enabled ENABLE_STREAMING_IN flag])
               ])
AM_CONDITIONAL([STREAM_IN_SUPPORTED], [test "x$ENABLE_STREAMING_IN" = xtrue])

AC_ARG_ENABLE([leonly],
    [AS_HELP_STRING([--enable-leonly],[enable le device only (default is no)])],
    [
        case "${enableval}" in
            yes) LE_ONLY=true ;;
            no)  LE_ONLY=false ;;
            *) AC_MSG_ERROR([bad value ${enableval} for --enable-leonly]) ;;
        esac
        if test "$LE_ONLY" = true; then
            AC_DEFINE(LE_MODE, 1, [Enabled LE only])
        else
            [echo "LE Only Mode not enabled"]
        fi
    ],
    [echo "LE Only Mode Not enabled"])
AM_CONDITIONAL([LE_ONLY], [test x$LE_ONLY = xtrue])

AC_ARG_ENABLE([gattclient],
    [AS_HELP_STRING([--enable-gattclient],[enable gatt client (default is no)])],
    [
        case "${enableval}" in
            yes) GATT_CLIENT_ENABLE=true ;;
            no)  GATT_CLIENT_ENABLE=false ;;
            *) AC_MSG_ERROR([bad value ${enableval} for --enable-gattclient]) ;;
        esac
        if test "$GATT_CLIENT_ENABLE" = true; then
            AC_DEFINE(GATT_CLIENT, 1, [Enabled Gatt Client])
        else
            [echo "Gatt Client not enabled"]
        fi
    ],
    [echo "Gatt Client Not enabled"])
AM_CONDITIONAL([GATT_CLIENT], [test x$GATT_CLIENT = xtrue])

AC_SUBST(BTR_CORE_SRC)
AC_SUBST(BTR_CORE_TEST)
AC_SUBST(BTR_CORE_LOGGER_LIBS)

# Checks for library functions.

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/bt-ifce/Makefile
                 src/bt-ifce/gdbus/Makefile
                 tests/Makefile])

AC_OUTPUT

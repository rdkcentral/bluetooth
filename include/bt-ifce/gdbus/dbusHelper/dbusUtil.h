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
 * @file dbusUtil.h
 * 
*/

#ifndef __DBUSUTIL_H__
#define __DBUSUTIL_H__

#include <gio/gio.h>

/**
 * Find immediate subpaths (non-recursive) below a given root path.
 * @param objectPath Any valid D-Bus object path, including root ('/')
 * @return a list of strings, use g_slist_free_full(list, free) to release
 */
GSList *dbusUtilFindSubpaths(const char *busName, const char *objectPath);

/**
 * Introspect an object.
 * @param objectPath
 * @return a linked list
 */
GDBusNodeInfo *dbusUtilIntrospect(const char *busName, const char *objectPath);


#endif //__DBUSUTIL_H__

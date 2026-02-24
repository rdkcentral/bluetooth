/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
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
 * bt-telemetry.c
 * Implementation of telemetry wrapper utilities for Bluetooth Core and Bluetooth Manager
 */

#include <stdio.h>
#include <telemetry_busmessage_sender.h>

#include "btrCore_logger.h"
#include "bt-telemetry.h"

/**
 * @brief Initialize telemetry with component name
 */
void telemetry_init(const char* name)
{
    if (name == NULL) {
        BTRCORELOG_ERROR("T2: Failed to initialize telemetry - component name is NULL\n");
        return;
    }
    BTRCORELOG_INFO("T2: Initializing telemetry with component name=\"%s\"\n", name);
    t2_init(name);
}

/**
 * @brief Send marker with string value to T2
 */
void telemetry_event_s(const char* marker, char* value)
{
    if (marker == NULL) {
        BTRCORELOG_ERROR("T2: telemetry_event_s - marker is NULL\n");
        return;
    }
    if (value == NULL) {
        BTRCORELOG_ERROR("T2: telemetry_event_s - value is NULL\n");
        return;
    }
    T2ERROR t2error = t2_event_s(marker, value);
    if (t2error != T2ERROR_SUCCESS) {
        BTRCORELOG_ERROR("t2_event_s(\"%s\", \"%s\") returned error code %d\n", marker, value, t2error);
    }
}

/**
 * @brief Send marker with integer value to T2 or report count based markers
 */
void telemetry_event_d(const char* marker, int value)
{
    if (marker == NULL) {
        BTRCORELOG_ERROR("T2: telemetry_event_d - marker is NULL\n");
        return;
    }
    T2ERROR t2error = t2_event_d(marker, value);
    if (t2error != T2ERROR_SUCCESS) {
        BTRCORELOG_ERROR("t2_event_d(\"%s\", %d) returned error code %d\n", marker, value, t2error);
    }
}

/**
 * @brief Send marker with double value to T2
 */
void telemetry_event_f(const char* marker, double value)
{
    if (marker == NULL) {
        BTRCORELOG_ERROR("T2: telemetry_event_f - marker is NULL\n");
        return;
    }
    T2ERROR t2error = t2_event_f(marker, value);
    if (t2error != T2ERROR_SUCCESS) {
        BTRCORELOG_ERROR("t2_event_f(\"%s\", %f) returned error code %d\n", marker, value, t2error);
    }
}

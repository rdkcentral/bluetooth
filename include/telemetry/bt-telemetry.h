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
 * @file bt-telemetry.h
 * Includes telemetry wrapper utilities for Bluetooth Core and Bluetooth Manager
 */

#ifndef __BT_TELEMETRY_H__
#define __BT_TELEMETRY_H__

/**
 * @brief Initialize telemetry with component name
 * 
 * @param name Component name for telemetry identification
 */
void telemetry_init(const char* name);

/**
 * @brief Send marker with string value to T2
 * 
 * @param marker Telemetry marker/event name (use _split suffix for split markers)
 * @param value String value to send
 */
void telemetry_event_s(const char* marker, char* value);

/**
 * @brief Send marker with integer value to T2 or report count based markers
 * 
 * @param marker Telemetry marker/event name
 * @param value Integer value to send
 */
void telemetry_event_d(const char* marker, int value);

/**
 * @brief Send marker with double value to T2
 * 
 * @param marker Telemetry marker/event name (use _split suffix for split markers)
 * @param value Double value to send
 * 
 * Usage: telemetry_event_f("HWREV_split", 2.2);
 */
void telemetry_event_f(char* marker, double value);

#endif /* __BT_TELEMETRY_H__ */

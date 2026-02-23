
/*
 * mock_bt-telemetry.h
 * Mock implementations for telemetry wrapper utilities used in Bluetooth Core and Bluetooth Manager unit tests.
 *
 * Copyright 2026 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MOCK_BT_TELEMETRY_H
#define MOCK_BT_TELEMETRY_H

#include <stdio.h>

// Mock implementations for telemetry_event_s and telemetry_event_d

static inline void telemetry_event_s(char* marker, char* value) {
    printf("[MOCK] telemetry_event_s called with marker: %s, value: %s\n", marker, value);
}

static inline void telemetry_event_d(char* marker, int value) {
    printf("[MOCK] telemetry_event_d called with marker: %s, value: %d\n", marker, value);
}

static inline void telemetry_event_f(char* marker, double value) {
    printf("[MOCK] telemetry_event_f called with marker: %s, value: %f\n", marker, value);
}

#include <string.h>

static inline void telemetry_init(char* name) {
    printf("[MOCK] telemetry_init called with name: %s\n", name);
}

#endif // MOCK_BT_TELEMETRY_H

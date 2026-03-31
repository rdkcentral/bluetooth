#!/bin/bash
set -e

echo "Installing system dependencies..."

sudo apt-get update
sudo apt-get install -y \
  autoconf automake libtool pkg-config \
  gcc g++ make \
  libglib2.0-dev libdbus-1-dev libbluetooth-dev \
  git

WORKSPACE=$(pwd)

echo "Setting up telemetry stub..."

git clone https://github.com/rdkcentral/telemetry.git

mkdir -p ${WORKSPACE}/external/include
mkdir -p ${WORKSPACE}/external/lib

cp telemetry/include/telemetry_busmessage_sender.h ${WORKSPACE}/external/include/
cp telemetry/include/telemetry2_0.h ${WORKSPACE}/external/include/

cat << 'EOF' > telemetry_stub.c
int t2_init(void) { return 0; }
int t2_event_s(const char* n, const char* v) { return 0; }
int t2_event_f(const char* n, float v) { return 0; }
int t2_event_d(const char* n, double v) { return 0; }
EOF

gcc -shared -fPIC telemetry_stub.c \
  -o ${WORKSPACE}/external/lib/libtelemetry_msgsender.so

echo "Setting up BlueZ legacy headers..."

git clone https://github.com/bluez/bluez.git

mkdir -p ${WORKSPACE}/external/include/bluetooth/audio
mkdir -p ${WORKSPACE}/external/include/bluetooth

git -C bluez checkout tags/4.101

cp bluez/audio/ipc.h \
  ${WORKSPACE}/external/include/bluetooth/audio/ipc.h

git -C bluez checkout tags/5.48

cp bluez/profiles/audio/a2dp-codecs.h \
  ${WORKSPACE}/external/include/bluetooth/audio/a2dp-codecs.h

cp bluez/lib/bluetooth.h \
  ${WORKSPACE}/external/include/bluetooth/bluetooth.h

sed -i '1i#include <stdbool.h>\n' \
  ${WORKSPACE}/external/include/bluetooth/audio/ipc.h

echo "Dependencies setup completed."

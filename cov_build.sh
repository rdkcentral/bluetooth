#!/bin/bash
set -e

WORKSPACE=$(pwd)

echo "Bootstrapping autotools..."

libtoolize --force
aclocal
autoheader
automake --force-missing --add-missing
autoconf

echo "Configuring project..."

export CPPFLAGS="-I${WORKSPACE}/external/include"
export LDFLAGS="-L${WORKSPACE}/external/lib"
export CFLAGS="-Wno-error"
export CXXFLAGS="-Wno-error"
export LD_LIBRARY_PATH="${WORKSPACE}/external/lib"

ac_cv_header_telemetry_busmessage_sender_h=yes ./configure

echo "Building BT-Core..."

make -C src/bt-ifce -j$(nproc)
make -C src -j$(nproc)

echo "Build completed successfully."

#!/bin/bash
set -e

echo "[Coverity] Building Bluetooth project"

# Ensure clean state for full scans
if [ "$COVERITY_SCAN_TYPE" = "full" ]; then
  make clean || true
fi

# Example build – adjust if Bluetooth uses different targets
make all

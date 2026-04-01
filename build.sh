#!/bin/bash
set -e

echo "========== Coverity build: Bluetooth =========="

# Ensure required autotools exist
command -v autoreconf >/dev/null 2>&1 || {
  echo "autoreconf not found. Autotools must be installed."
  exit 1
}

# Step 1: Generate configure script & Makefiles if missing
if [ ! -f "./configure" ]; then
  echo "[Coverity] Running autoreconf..."
  autoreconf -fi
fi

# Step 2: Configure project (adjust flags if needed)
if [ ! -f "Makefile" ]; then
  echo "[Coverity] Running configure..."
  ./configure
fi

# Step 3: Clean only for full scan
if [ "$COVERITY_SCAN_TYPE" = "full" ]; then
  echo "[Coverity] Cleaning build (full scan)"
  make clean || true
fi

# Step 4: Build (Coverity captures here)
echo "[Coverity] Running make"
make -j$(nproc)

echo "========== Coverity build complete =========="

#!/usr/bin/env bash
# =====================================================================
# Mcl0v1n vs android - build the manager APK (KitKat-era Ant toolchain).
# Produces app/bin/Mcl0v1n-*.apk
# =====================================================================
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT/app"

: "${ANDROID_SDK:?Set ANDROID_SDK}"
export PATH="$ANDROID_SDK/tools:$ANDROID_SDK/platform-tools:$PATH"

# Generate a throwaway debug keystore if none exists (reproducible local build).
KS="$ROOT/prebuilt/mclovin.keystore"
if [ ! -f "$KS" ]; then
  echo "==> Creating signing keystore ($KS)"
  keytool -genkeypair -v \
    -keystore "$KS" -alias mclovin \
    -keyalg RSA -keysize 2048 -validity 10000 \
    -storepass mclovin -keypass mclovin \
    -dname "CN=Mcl0v1n vs android, O=mclovin, C=US"
fi

echo "==> Generating Ant build files"
android update project -p . --target android-19 --name Mcl0v1n >/dev/null

echo "==> ant release"
ant release

# Normalise output name
if [ -f bin/Mcl0v1n-release.apk ]; then
  echo "APK: app/bin/Mcl0v1n-release.apk"
else
  echo "APK: app/bin/Mcl0v1n-debug.apk (unsigned-release not produced)"
fi

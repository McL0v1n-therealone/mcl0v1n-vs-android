# Build Guide — Mcl0v1n vs android

Reproducible build of the native binaries, the manager APK, and the flashable
zip. Everything here runs on your **computer (host)**, not the phone.

## 1. Prerequisites

| Tool | Version | Why |
|------|---------|-----|
| Android NDK | **r10e** | GCC 4.8 ARM toolchain matching KitKat ABI |
| Android SDK | Platform **API 19** + Build-tools **19.1.0** | compile the manager APK |
| Apache Ant | 1.9.x | KitKat-era APK build (`ant release`) |
| JDK | 7 | required by API 19 build tools / keytool |
| zip, unzip | any | package the flashable zip |
| Java (for signing) | 7+ | run `zipsigner.jar` (optional) |

Install locations are referenced via environment variables:

```bash
export NDK_HOME=/opt/android-ndk-r10e
export ANDROID_SDK=/opt/android-sdk
export PATH="$ANDROID_SDK/tools:$ANDROID_SDK/platform-tools:$PATH"
```

> Why r10e / API 19 and not a modern NDK? Newer NDKs drop GCC and raise the
> minimum API; their libc/ABI assumptions can produce binaries that misbehave on
> a KitKat kernel. r10e is the last reliably KitKat-friendly NDK.

## 2. One-command build

```bash
./scripts/build.sh
```

This runs, in order:

1. `ndk-build` on `native/su/Android.mk` → static `su` binary
   (also used as `mclovind`/`daemonsu` via symlink).
2. `ndk-build` on `native/resetprop/Android.mk` → `mclovin_sepatch`.
3. Copies both into `flashable/mclovin/bin/`.
4. `scripts/build_app.sh` → builds `Mcl0v1n.apk` (creates a keystore on first run).
5. `scripts/make_zip.sh` → zips + signs → `out/Mcl0v1n-vs-android-i9300-signed.zip`.

### Outputs

```
out/
├── Mcl0v1n-vs-android-i9300-signed.zip   # flash this in recovery
└── Mcl0v1n.apk                           # standalone manager apk (also inside the zip)
```

## 3. Building pieces individually

### Native only
```bash
cd native
$NDK_HOME/ndk-build NDK_PROJECT_PATH=. \
  APP_BUILD_SCRIPT=su/Android.mk \
  NDK_APPLICATION_MK=Application.mk
# result: native/obj/local/armeabi-v7a/su
file native/obj/local/armeabi-v7a/su   # -> ELF 32-bit ARM, statically linked
```

### Manager APK only
```bash
./scripts/build_app.sh     # -> app/bin/Mcl0v1n-release.apk
```

### Flashable zip only (after binaries + apk staged)
```bash
./scripts/make_zip.sh
```

## 4. Signing

- **APK:** signed by `ant release` using `app/ant.properties` (keystore at
  `prebuilt/mclovin.keystore`, auto-generated on first build).
  - Because the manifest declares `android:sharedUserId="android.uid.system"`,
    to *actually* obtain the system UID you must sign the APK with the ROM's
    **platform key** (`platform.pk8`/`platform.x509.pem`). For a personal CM11
    build, sign with your ROM's test-keys. If you don't need the system UID,
    remove the `sharedUserId` line and rebuild — the app still works as a normal
    `/system/app`.
- **Flashable zip:** drop AOSP `SignApk`/`zipsigner.jar` into `prebuilt/` to get
  a signed zip. Otherwise the zip ships unsigned and you disable "Zip signature
  verification" in TWRP.

## 5. Reproducibility notes

- The build is deterministic given fixed NDK r10e + Build-tools 19.1.0; the only
  non-deterministic input is the signing keystore. Commit a fixed keystore to
  `prebuilt/` if you need byte-identical signed artifacts.
- `scripts/build.sh` is `set -euo pipefail`; a missing tool fails loudly rather
  than producing a half-built zip.

## 6. Source layout

```
mcl0v1n-vs-android/
├── native/
│   ├── Application.mk              # ABI=armeabi-v7a, platform=android-19
│   ├── su/
│   │   ├── su.h                    # shared defs / wire protocol
│   │   ├── su.c                    # client entry + arg parsing + role dispatch
│   │   ├── client.c                # connect to daemon, fd passing
│   │   ├── daemon.c                # mclovind: listen, authenticate, exec
│   │   ├── policy.c                # policy file + manager prompt + logging
│   │   └── Android.mk
│   └── resetprop/
│       ├── mclovin_sepatch.c       # boot-time SELinux/perm fixer
│       └── Android.mk
├── app/                            # manager APK (Java, API 19, Ant)
│   ├── AndroidManifest.xml
│   ├── project.properties / ant.properties
│   ├── res/{layout,values,drawable}/
│   └── src/com/mcl0v1n/superuser/*.java
├── flashable/
│   ├── META-INF/com/google/android/{update-binary,updater-script}
│   └── mclovin/{bin/,common/,Mcl0v1n.apk}
├── scripts/{build.sh,build_app.sh,make_zip.sh}
└── docs/
```

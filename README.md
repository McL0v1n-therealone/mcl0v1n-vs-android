# Mcl0v1n vs android

**A custom, persistent root solution for Android 4.4.4 (KitKat) on the Samsung Galaxy S III (GT-I9300).**

`Mcl0v1n vs android` ("Mcl0v1n" for short) is a SuperSU-style root solution: a
privileged `su` binary + background daemon installed into the system partition,
together with an Android **root-manager app** that lets the user grant, deny,
remember, revoke and audit root requests.

This is a **real, system-level root** — not a runtime exploit that vanishes on
reboot. Once installed into `/system`, root survives normal reboots because the
`su` binary and daemon live on the read-only system partition and are launched
by `init` on every boot.

---

## What this project is / is not

| | |
|---|---|
| ✅ Persistent `su` binary installed to `/system/xbin` | ❌ Not a one-shot "temp root" exploit |
| ✅ Background `mclovind` daemon started by `init` | ❌ Not a Magisk-style systemless/boot-image patch (KitKat predates that) |
| ✅ SuperSU-like manager app (grant/deny/remember/revoke/log) | ❌ Not a Play-Store app that *simulates* root |
| ✅ Flashable recovery ZIP + ADB install path | ❌ Does not bypass a **locked** bootloader (S3 i9300 ships unlocked) |

---

## Component map — phone vs. computer

Understanding **where each piece runs** is essential.

### Built on your computer (host)
- `native/su/` — C source for the `su` client + `mclovind` daemon. Cross-compiled
  with the Android NDK for **ARM (armeabi-v7a)**.
- `native/resetprop/` — small helper to set SELinux/props at boot.
- `app/` — the Android root-manager APK (`Mcl0v1n.apk`).
- `flashable/` — the recovery-flashable ZIP packaging (updater-script + installer).
- `scripts/` — build + packaging automation.

### Flashed/run on the phone (Galaxy S III)
- Custom recovery (**TWRP** / CWM) — used once to flash the ZIP.
- `/system/xbin/su`, `/system/xbin/mclovind`, `/system/xbin/daemonsu` — installed binaries.
- `/system/etc/init.d/` or `install-recovery.sh` hook — boot-time daemon launcher.
- `/system/app/Mcl0v1n.apk` — the manager app (system app so it survives factory reset-less wipes and can be granted its policy DB).
- `/data/data/com.mcl0v1n.superuser/` — runtime permission database + logs.

---

## Quick start

```bash
# 1. Point to your NDK (r10e recommended for KitKat-era ABI)
export NDK_HOME=/opt/android-ndk-r10e
export ANDROID_SDK=/opt/android-sdk     # API 19 build tools for the app

# 2. Build everything and produce the flashable zip
./scripts/build.sh

# 3. Output:
#    out/Mcl0v1n-vs-android-i9300-signed.zip   <- flash in TWRP
#    out/Mcl0v1n.apk                            <- bundled inside the zip
```

Then on the phone:

```
Reboot to recovery (Vol-Up + Home + Power) -> Install -> select the zip -> Reboot System
```

See [`docs/INSTALL.md`](docs/INSTALL.md) for the full procedure and the
ADB/`adbd`-insecure alternative.

---

## Documentation

| Doc | Purpose |
|-----|---------|
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | How the su/daemon/app talk to each other |
| [`docs/BUILD.md`](docs/BUILD.md)               | Full reproducible build process |
| [`docs/INSTALL.md`](docs/INSTALL.md)           | Flashing & installation on the i9300 |
| [`docs/COMPATIBILITY.md`](docs/COMPATIBILITY.md) | Bootloader, kernel, recovery, SELinux, partitions |
| [`docs/TROUBLESHOOTING.md`](docs/TROUBLESHOOTING.md) | Boot loops, denials, "su not found", etc. |

---

## Legal / safety note

This targets **your own** Galaxy S III. Rooting trips the Knox/efuse flags and
voids warranty, and a bad `/system` edit can bootloop the device. Everything here
is reversible with Odin + stock firmware (see troubleshooting). Use responsibly.

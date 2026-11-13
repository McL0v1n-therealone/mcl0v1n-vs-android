# Installation Guide — Mcl0v1n vs android

Two supported paths:

- **A. Recovery flash (recommended, persistent):** flash the zip in TWRP/CWM.
- **B. ADB/manual (advanced):** push files to `/system` yourself.

Both install the **same** on-`/system` components, so both are persistent.

---

## Before you start (do this once)

1. **Back up EFS** (`/efs`) — contains your IMEI. In TWRP: Backup → EFS. Keep it
   off-device. Losing EFS can brick cellular permanently.
2. **Full NANDroid backup** in recovery (Boot + System + Data).
3. Confirm the device: in recovery terminal or `adb shell getprop ro.product.device`
   → should be `m0` / `i9300`.
4. Have **stock firmware + Odin/Heimdall** ready for recovery (see Troubleshooting).

---

## Prerequisite: custom recovery

If you don't already have TWRP/CWM:

```bash
# Heimdall (Linux) — put device in Download mode (Vol-Down + Home + Power)
heimdall flash --RECOVERY twrp-2.8.7.0-i9300.img --no-reboot
```
or flash the `.tar.md5` recovery via **Odin** (Windows) in the **AP/PDA** slot.

---

## Path A — Flash the zip (recommended)

1. Copy `out/Mcl0v1n-vs-android-i9300-signed.zip` to the phone (internal storage
   or SD card):
   ```bash
   adb push out/Mcl0v1n-vs-android-i9300-signed.zip /sdcard/
   ```
2. Reboot to recovery: **Vol-Up + Home + Power** (release Power at the logo).
3. In **TWRP**: `Install` → select the zip → swipe to confirm.
   In **CWM**: `install zip` → `choose zip from /sdcard` → select → confirm.
   - If TWRP complains about signature: `Settings → disable Zip signature
     verification` (our zip is self/zipsigner-signed; stock verify may reject it).
4. `Reboot System`.
5. First boot takes a little longer (init runs `install-recovery.sh`, launches
   `mclovind`). When booted you'll find **"Mcl0v1n vs android"** in the launcher.

### Verify
```bash
adb shell
$ su -v
1.0 (Mcl0v1n vs android)
$ su -c id
uid=0(root) gid=0(root) ...
```
The first `su` from a real app triggers the grant dialog; `adb shell` (uid 2000)
is auto-allowed so you can bootstrap.

---

## Path B — Manual / ADB install (advanced)

Use this if you can already write `/system` (e.g. insecure `adbd`, or from a
recovery adb shell). Run as root:

```bash
adb push flashable/mclovin/bin/su            /sdcard/su
adb push flashable/mclovin/bin/mclovin_sepatch /sdcard/mclovin_sepatch
adb push out/Mcl0v1n.apk                     /sdcard/Mcl0v1n.apk
adb push flashable/mclovin/common/install-recovery.sh /sdcard/
adb shell

# inside a ROOT shell / recovery shell:
mount -o rw,remount /system           # or: mount -t ext4 -o rw /dev/block/mmcblk0p9 /system
cp /sdcard/su /system/xbin/su
ln -sf /system/xbin/su /system/xbin/mclovind
ln -sf /system/xbin/su /system/xbin/daemonsu
ln -sf /system/xbin/su /system/bin/su
cp /sdcard/mclovin_sepatch /system/xbin/mclovin_sepatch
chown 0.0 /system/xbin/su /system/xbin/mclovin_sepatch
chmod 06755 /system/xbin/su
chmod 0755  /system/xbin/mclovin_sepatch

cp /sdcard/Mcl0v1n.apk /system/app/Mcl0v1n.apk
chmod 0644 /system/app/Mcl0v1n.apk

cp /sdcard/install-recovery.sh /system/etc/install-recovery.sh
chmod 0755 /system/etc/install-recovery.sh

sync
reboot
```

---

## What ends up on the phone

| On-device path | Component | Purpose |
|----------------|-----------|---------|
| `/system/xbin/su` | native binary (setuid root 06755) | client apps call this |
| `/system/xbin/mclovind` → `su` | symlink | the persistent daemon |
| `/system/xbin/daemonsu` → `su` | symlink | daemon launcher for init hook |
| `/system/bin/su` → `su` | symlink | compatibility for `/system/bin/su` callers |
| `/system/xbin/mclovin_sepatch` | native helper | boot-time SELinux/perm fix |
| `/system/etc/install-recovery.sh` | boot hook | **persistence** — launches daemon each boot |
| `/system/etc/init.d/99mclovin` | boot hook (fallback) | for init.d-capable ROMs |
| `/system/app/Mcl0v1n.apk` | manager app | grant/deny/revoke/log UI |
| `/data/data/com.mcl0v1n.superuser/mclovin.db` | runtime | rule database |
| `/data/data/com.mcl0v1n.superuser/mclovin.policy` | runtime | flat mirror the daemon reads |
| `/data/data/com.mcl0v1n.superuser/mclovin.log` | runtime | access log |
| `/data/data/com.mcl0v1n.superuser/mclovin_sockets/` | runtime | req-/resp- handoff files |

The **manager app is installed automatically** by the flashable zip (copied to
`/system/app`), so there is no separate APK install step for Path A.

---

## Uninstall / revert

Flash the included `out/` zip's counterpart or do it manually in recovery:

```bash
mount -o rw,remount /system
rm -f /system/xbin/su /system/xbin/mclovind /system/xbin/daemonsu \
      /system/xbin/mclovin_sepatch /system/bin/su \
      /system/app/Mcl0v1n.apk /system/etc/init.d/99mclovin
# restore stock recovery hook if present
[ -f /system/etc/install-recovery.sh.stock ] && \
  mv /system/etc/install-recovery.sh.stock /system/etc/install-recovery.sh || \
  rm -f /system/etc/install-recovery.sh
rm -rf /data/data/com.mcl0v1n.superuser
sync; reboot
```
A full un-root/clean is also achieved by reflashing stock firmware with Odin.

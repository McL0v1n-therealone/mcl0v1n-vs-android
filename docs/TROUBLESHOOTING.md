# Troubleshooting — Mcl0v1n vs android

## `su: not found` after reboot
- Confirm the binary exists and perms are correct:
  ```bash
  adb shell ls -l /system/xbin/su     # expect -rwsr-sr-x root root
  ```
- If missing: a ROM OTA likely overwrote `/system`. Re-flash the Mcl0v1n zip.
- If present but perms wrong: run `mclovin_sepatch`, or in recovery:
  `chmod 06755 /system/xbin/su && chown 0.0 /system/xbin/su`.

## `su` prints "cannot reach daemon (is mclovind running?)"
The client connected but no daemon is listening.
```bash
adb shell ps | grep mclovind          # is it running?
adb shell /system/xbin/daemonsu --daemon &   # start it manually
```
Persistent fix — ensure the boot hook is in place and executable:
```bash
adb shell ls -l /system/etc/install-recovery.sh   # expect 0755
```
If your ROM ignores `install-recovery.sh`, rely on `init.d` (`99mclovin`) or the
app's `BootReceiver`. Some stripped-down ROMs have neither; in that case add a
`service`/`on boot` entry to the boot image ramdisk `init.rc` (advanced).

## Root prompt never appears / everything is denied
- The daemon **fails closed**: if the manager can't answer within ~30s it
  returns DENY. Check the manager is installed and its service runs:
  ```bash
  adb shell pm path com.mcl0v1n.superuser
  adb shell ls -ld /data/data/com.mcl0v1n.superuser
  ```
- Check the handoff dir is writable by both (root daemon + app uid):
  ```bash
  adb shell ls -l /data/data/com.mcl0v1n.superuser/mclovin_sockets
  ```
- Open the Mcl0v1n app once so `RequestWatcherService` starts and the DB/dirs
  get created.

## Bootloop after flashing
Usually a bad `install-recovery.sh` or SELinux edit.
1. Boot to recovery (Vol-Up+Home+Power).
2. `adb shell` into recovery, mount system, neutralise the hook:
   ```bash
   mount -o rw,remount /system
   mv /system/etc/install-recovery.sh /system/etc/install-recovery.sh.bak
   reboot
   ```
3. If still looping, restore your NANDroid or reflash stock via Odin.

## SELinux denials (enforcing ROMs)
- Check mode: `adb shell getenforce` → want `Permissive` for this solution.
- `mclovin_sepatch` sets permissive at boot; confirm it ran:
  `adb shell dmesg | grep mclovin` or re-run it manually as root.
- If your kernel refuses to go permissive (locked policy), you must use a
  permissive-booting i9300 kernel — Mcl0v1n deliberately does not ship sepolicy
  live-patching.

## Manager app not getting the system UID
Only relevant if you kept `android:sharedUserId="android.uid.system"`. That
requires signing the APK with the ROM's **platform key**. Either sign with your
CM11 build's keys, or remove the `sharedUserId` line (the app functions without
it; it just runs as a normal system app). Rebuild after changing.

## "Custom" status / binary counter set
Expected after flashing. Not harmful. Reset by reflashing full stock firmware
(and/or Triangle Away on older bootloaders). Mcl0v1n does not modify the
bootloader or counter.

## Apps still say "no root"
- Some apps only probe `/system/bin/su`. Confirm the compat symlink exists:
  `adb shell ls -l /system/bin/su`.
- Grant the app explicitly: open Mcl0v1n, or trigger its root request and tap
  **Grant** (+ Remember).
- A cached DENY rule blocks it — open Mcl0v1n, tap the rule, **Revoke**, retry.

## Lost IMEI / no signal after rooting
You (or an app) corrupted `/efs`. Restore the EFS backup you made pre-root
(TWRP → Restore → EFS). This is why the EFS backup in INSTALL.md step 1 is
mandatory.

## Verifying a healthy install (quick checklist)
```bash
adb shell 'ls -l /system/xbin/su /system/xbin/mclovind /system/xbin/daemonsu'
adb shell 'ls -l /system/etc/install-recovery.sh'
adb shell 'pm path com.mcl0v1n.superuser'
adb shell 'su -v'
adb shell 'su -c id'          # should print uid=0 after you Grant
adb shell 'getenforce'        # Permissive
```

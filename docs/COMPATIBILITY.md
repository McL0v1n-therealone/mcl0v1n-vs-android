# Device Compatibility — Galaxy S III (GT-I9300) on Android 4.4.4

Everything below is specific to the **international Exynos i9300** (codename
`m0`, board `smdk4x12`, SoC Exynos 4412). It does **not** apply unchanged to the
Qualcomm variants (i9305 LTE, d2lte/SGH-* US models) — those use different
partitions and kernels.

## 1. Bootloader

- The i9300 ships with an **unlockable/already-unlocked** bootloader (unlike
  US carrier models). No bootloader unlock step is required to flash custom
  recovery or to write `/system`.
- It uses **Samsung Odin / download mode** (Vol-Down + Home + Power) for flashing
  `.tar.md5` images, and **recovery mode** (Vol-Up + Home + Power).
- **Knox is not present** on original i9300 firmware (Knox arrived on S4+), so
  there is no Knox e-fuse to trip — but flashing custom binaries still sets the
  "Custom" status / increments the binary-flash counter visible in download mode.
  A Triangle-Away/stock reflash resets the counter.
- Mcl0v1n does **not** touch the bootloader. It only writes `/system`. A locked
  bootloader (not the case on i9300) would block this approach entirely.

## 2. Kernel

- Mcl0v1n does **not** require a custom kernel for stock-based 4.4.4 where
  SELinux is permissive. The daemon runs fine under the stock boot image.
- If you run an **enforcing** KitKat kernel (some CM11 nightlies), you need one
  of:
  - a kernel that boots **permissive** (most custom i9300 KitKat kernels do), or
  - `mclovin_sepatch` flipping `/sys/fs/selinux/enforce` to `0` at boot
    (included — runs from the init hook while still privileged), or
  - a patched `sepolicy` (out of scope; permissive is the pragmatic KitKat path).
- The stock Samsung i9300 kernel enforces a **`RESTRICT_ROOT` / Samsung reserved
  partition** check only on later firmwares; 4.4.4 i9300 (if it ever shipped) and
  CM-based 4.4.4 are fine. Note: Samsung's **official** last i9300 build is 4.3;
  **4.4.4 on i9300 is community (CyanogenMod 11 / AOSP)**. See §7.

## 3. Recovery

- You need a **custom recovery** to flash the zip:
  - **TWRP 2.8.x** for i9300 (`twrp-2.8.x.x-i9300.img`), or
  - **CWM 6.x** for i9300.
- Flash recovery via Odin (`.tar`) or `heimdall flash --RECOVERY recovery.img`.
- Our `update-binary` is a **shell** script (not an edify/ELF binary) precisely
  so it runs on both CWM6 and TWRP 2.x on this device.
- **Caveat:** our `install-recovery.sh` persistence hook can be overwritten if
  the recovery's own "flash stock recovery on boot" runs — we chain the stock
  script and most custom recoveries don't re-flash, so this is fine in practice.

## 4. SELinux

- KitKat introduced SELinux **enforcing** in AOSP, but on the i9300:
  - Samsung stock (where applicable) and most CM11 builds run **permissive** on
    this older SoC, so a userspace `su`/daemon works without policy injection.
  - `mclovin_sepatch` defensively sets permissive at boot if it finds the system
    enforcing, mirroring early-KitKat SuperSU behaviour (before sepolicy live
    patching existed).
- We intentionally **do not** rely on `supolicy`/sepolicy live-injection (a
  later-KitKat SuperSU feature) to keep the solution simple and auditable.

## 5. Partition layout (i9300, `mmcblk0`)

| Partition | Mount | Typical block dev | Notes |
|-----------|-------|-------------------|-------|
| BOOT      | —      | `mmcblk0p5`  | kernel + ramdisk (boot.img) |
| RECOVERY  | —      | `mmcblk0p6`  | custom recovery goes here |
| RADIO     | —      | `mmcblk0p7`  | modem |
| EFS       | `/efs` | `mmcblk0p3`  | IMEI/NV — **never** touch, back it up |
| SYSTEM    | `/system` | `mmcblk0p9` | where Mcl0v1n installs (ext4) |
| USERDATA  | `/data`   | `mmcblk0p12`| app data / policy DB & log |
| CACHE     | `/cache`  | `mmcblk0p11`| |

The installer prefers `mount /system` by name and falls back to
`/dev/block/mmcblk0p9`. If your ROM uses a different mapping, edit `update-binary`.

- `/system` is ~1.5 GB ext4 on the i9300 — plenty of room for a ~60 KB su binary
  and a small APK.

## 6. ABI / toolchain

- Exynos 4412 = Cortex-A9 → **armeabi-v7a**, 32-bit. Build with NDK **r10e**
  (GCC 4.8) targeting **android-19**. No arm64 (KitKat/this SoC is 32-bit only).

## 7. Important reality check about "4.4.4 on the i9300"

Samsung's **official** final firmware for GT-I9300 is **Android 4.3**. Android
**4.4.4** on this phone is delivered via **CyanogenMod 11 / community AOSP
KitKat** ROMs. Mcl0v1n therefore assumes you are running such a 4.4.4 ROM. If you
are on stock 4.3 and only want root, the same package still works (KitKat-era
assumptions are backward-compatible with 4.3 on this device), but SELinux is a
non-issue there.

## 8. Known device-specific limitations

- **Odin binary counter / "Custom" status** will flip. Reversible with a stock
  firmware reflash; not reset by Mcl0v1n itself.
- **EFS safety:** rooting lets apps touch `/efs`. Back up `/efs` before rooting.
- **OTA updates** (from a custom ROM's updater) may overwrite `/system` and
  remove root; re-flash the Mcl0v1n zip after a ROM update.
- **Encryption:** if `/data` is encrypted, the policy DB/log paths still work
  post-decrypt, but the manager app's first-run must occur after unlock.

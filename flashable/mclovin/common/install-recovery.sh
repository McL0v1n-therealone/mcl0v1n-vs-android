#!/system/bin/sh
# =====================================================================
# Mcl0v1n vs android - boot hook
# Placed at /system/etc/install-recovery.sh. Android's init starts the
# "flash_recovery" one-shot service at boot, which runs this script in a
# root context. We use it to (re)patch SELinux if needed and launch the
# persistent daemon. This is why root survives reboots.
# =====================================================================

# 1. Chain the original stock script if we preserved one (keeps recovery
#    re-flash behaviour intact so OTAs don't fight us as hard).
if [ -f /system/etc/install-recovery.sh.stock ]; then
  /system/bin/sh /system/etc/install-recovery.sh.stock
fi

# 2. Fix SELinux mode / su perms (see native/resetprop/mclovin_sepatch.c).
if [ -x /system/xbin/mclovin_sepatch ]; then
  /system/xbin/mclovin_sepatch
fi

# 3. Launch the persistent root daemon if not already running.
if ! pgrep mclovind >/dev/null 2>&1; then
  /system/xbin/daemonsu --daemon &
fi

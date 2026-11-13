package com.mcl0v1n.superuser;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

/**
 * Mcl0v1n vs android - boot receiver.
 *
 * The primary persistence mechanism is the init hook (install-recovery.sh /
 * init.d) that launches /system/xbin/daemonsu at boot. This receiver is a
 * SECOND safety net: on BOOT_COMPLETED it (a) starts the watcher service and
 * (b) re-launches the daemon via `su --daemon` in case the init hook was
 * stripped by an OTA. It does NOT by itself provide root - it relies on the
 * already-installed /system binary.
 */
public class BootReceiver extends BroadcastReceiver {
    @Override
    public void onReceive(Context ctx, Intent intent) {
        if (!Intent.ACTION_BOOT_COMPLETED.equals(intent.getAction())) return;

        ctx.startService(new Intent(ctx, RequestWatcherService.class));

        // Best-effort: ensure the daemon is running.
        try {
            Runtime.getRuntime().exec(
                    new String[]{"/system/xbin/daemonsu", "--daemon"});
        } catch (Exception e) {
            // If the binary is missing the device simply isn't rooted anymore.
        }
    }
}

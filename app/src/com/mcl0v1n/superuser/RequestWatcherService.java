package com.mcl0v1n.superuser;

import android.app.Service;
import android.content.Intent;
import android.os.FileObserver;
import android.os.IBinder;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;

/**
 * Mcl0v1n vs android - bridge between the native daemon and the UI.
 *
 * Watches the handoff directory for req-<pid> files written by mclovind's
 * send_request_to_manager(). When one appears we parse it and pop the
 * SuRequestActivity so the user can decide. (Daemon polls for resp-<pid>.)
 */
public class RequestWatcherService extends Service {

    private FileObserver observer;

    @Override
    public void onCreate() {
        super.onCreate();
        final File dir = new File(SuRequestActivity.DIR);
        if (!dir.exists()) dir.mkdirs();

        observer = new FileObserver(dir.getAbsolutePath(),
                FileObserver.CREATE | FileObserver.MOVED_TO) {
            @Override
            public void onEvent(int event, String name) {
                if (name == null || !name.startsWith("req-")) return;
                handleRequest(new File(dir, name));
            }
        };
        observer.startWatching();
    }

    private void handleRequest(File req) {
        try {
            int fromUid = -1, toUid = 0, pid = -1;
            String caller = null;
            BufferedReader r = new BufferedReader(new FileReader(req));
            String line;
            while ((line = r.readLine()) != null) {
                int eq = line.indexOf('=');
                if (eq < 0) continue;
                String k = line.substring(0, eq);
                String v = line.substring(eq + 1);
                if (k.equals("from_uid")) fromUid = Integer.parseInt(v.trim());
                else if (k.equals("to_uid")) toUid = Integer.parseInt(v.trim());
                else if (k.equals("pid")) pid = Integer.parseInt(v.trim());
                else if (k.equals("caller")) caller = v.trim();
            }
            r.close();

            Intent i = new Intent(this, SuRequestActivity.class);
            i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK
                    | Intent.FLAG_ACTIVITY_CLEAR_TOP);
            i.putExtra("from_uid", fromUid);
            i.putExtra("to_uid", toUid);
            i.putExtra("pid", pid);
            i.putExtra("caller", caller);
            startActivity(i);
        } catch (Exception e) {
            // ignore malformed request; daemon fails closed on timeout
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        return START_STICKY;   // keep the watcher alive
    }

    @Override
    public IBinder onBind(Intent intent) { return null; }

    @Override
    public void onDestroy() {
        if (observer != null) observer.stopWatching();
        super.onDestroy();
    }
}

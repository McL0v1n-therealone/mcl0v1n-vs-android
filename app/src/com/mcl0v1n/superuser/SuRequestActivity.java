package com.mcl0v1n.superuser;

import android.app.Activity;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.TextView;

import java.io.File;
import java.io.FileWriter;

/**
 * Mcl0v1n vs android - interactive grant/deny dialog.
 *
 * Launched by RequestWatcherService when the daemon drops a req-<pid> file.
 * The user chooses ALLOW/DENY; if "Remember" is checked we persist a rule.
 * We write the decision to resp-<pid> which the native daemon is polling.
 */
public class SuRequestActivity extends Activity {

    static final String DIR =
            "/data/data/com.mcl0v1n.superuser/mclovin_sockets";

    private int fromUid, toUid, pid;
    private String caller;
    private PolicyDatabase db;

    @Override
    protected void onCreate(Bundle b) {
        super.onCreate(b);
        setContentView(R.layout.activity_request);
        db = new PolicyDatabase(this);

        fromUid = getIntent().getIntExtra("from_uid", -1);
        toUid   = getIntent().getIntExtra("to_uid", 0);
        pid     = getIntent().getIntExtra("pid", -1);
        caller  = getIntent().getStringExtra("caller");

        String label = labelForUid(fromUid, caller);

        ((TextView) findViewById(R.id.app_name)).setText(label);
        ((TextView) findViewById(R.id.details)).setText(
                "UID " + fromUid + "  (pid " + pid + ")\n"
                + "is requesting root access (uid " + toUid + ").");

        final CheckBox remember = (CheckBox) findViewById(R.id.remember);

        ((Button) findViewById(R.id.btn_allow)).setOnClickListener(
                new View.OnClickListener() {
            public void onClick(View v) { decide(true, remember.isChecked(), label); }
        });
        ((Button) findViewById(R.id.btn_deny)).setOnClickListener(
                new View.OnClickListener() {
            public void onClick(View v) { decide(false, remember.isChecked(), label); }
        });
    }

    private String labelForUid(int uid, String fallback) {
        try {
            PackageManager pm = getPackageManager();
            String[] pkgs = pm.getPackagesForUid(uid);
            if (pkgs != null && pkgs.length > 0) {
                ApplicationInfo ai = pm.getApplicationInfo(pkgs[0], 0);
                return pm.getApplicationLabel(ai).toString();
            }
        } catch (Exception ignore) {}
        return fallback != null ? fallback : ("uid " + uid);
    }

    private String pkgForUid(int uid) {
        String[] pkgs = getPackageManager().getPackagesForUid(uid);
        return (pkgs != null && pkgs.length > 0) ? pkgs[0] : null;
    }

    private void decide(boolean allow, boolean remember, String label) {
        if (remember) {
            db.setRule(fromUid, toUid, pkgForUid(fromUid), label, allow);
        }
        try {
            File resp = new File(DIR, "resp-" + pid);
            FileWriter w = new FileWriter(resp, false);
            w.write(allow ? "ALLOW" : "DENY");
            w.flush();
            w.close();
            resp.setReadable(true, false);
        } catch (Exception e) {
            // If we can't write, the daemon times out and fails closed (DENY).
        }
        finish();
    }
}

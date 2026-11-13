package com.mcl0v1n.superuser;

import android.app.Activity;
import android.os.Bundle;
import android.widget.ScrollView;
import android.widget.TextView;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * Mcl0v1n vs android - basic root-access log viewer.
 *
 * Renders the newest-first contents of /data/data/.../mclovin.log which the
 * native daemon appends to on every ALLOW/DENY/PROMPT decision.
 */
public class LogActivity extends Activity {

    static final String LOG =
            "/data/data/com.mcl0v1n.superuser/mclovin.log";

    @Override
    protected void onCreate(Bundle b) {
        super.onCreate(b);
        TextView tv = new TextView(this);
        tv.setPadding(24, 24, 24, 24);
        tv.setTextSize(13f);
        tv.setTypeface(android.graphics.Typeface.MONOSPACE);
        tv.setText(readLogReversed());

        ScrollView sv = new ScrollView(this);
        sv.addView(tv);
        setContentView(sv);
        setTitle("Mcl0v1n access log");
    }

    private String readLogReversed() {
        File f = new File(LOG);
        if (!f.exists()) return "No root access has been logged yet.";
        List<String> lines = new ArrayList<String>();
        try {
            BufferedReader r = new BufferedReader(new FileReader(f));
            String line;
            while ((line = r.readLine()) != null) lines.add(line);
            r.close();
        } catch (Exception e) {
            return "Unable to read log: " + e.getMessage();
        }
        Collections.reverse(lines);
        StringBuilder sb = new StringBuilder();
        for (String l : lines) sb.append(l).append('\n');
        return sb.length() == 0 ? "Log is empty." : sb.toString();
    }
}

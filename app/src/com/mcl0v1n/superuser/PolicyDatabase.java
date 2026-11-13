package com.mcl0v1n.superuser;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;

import java.io.File;
import java.io.FileWriter;
import java.util.ArrayList;
import java.util.List;

/**
 * Mcl0v1n vs android - authoritative policy store.
 *
 * Holds the grant/deny rules the UI manages, and mirrors them into a flat file
 * (mclovin.policy) that the native mclovind daemon parses with no SQLite dep.
 */
public class PolicyDatabase extends SQLiteOpenHelper {

    public static final int ALLOW = 0;
    public static final int DENY  = 1;

    private static final String DB_NAME = "mclovin.db";
    private static final int    DB_VER  = 1;

    /** Flat mirror the C daemon reads: "<fromUid> <toUid> ALLOW|DENY" per line. */
    public static final String POLICY_FILE =
            "/data/data/com.mcl0v1n.superuser/mclovin.policy";

    private final Context ctx;

    public PolicyDatabase(Context context) {
        super(context, DB_NAME, null, DB_VER);
        this.ctx = context;
    }

    @Override
    public void onCreate(SQLiteDatabase db) {
        db.execSQL("CREATE TABLE policy ("
                + " _id INTEGER PRIMARY KEY AUTOINCREMENT,"
                + " from_uid INTEGER NOT NULL,"
                + " to_uid INTEGER NOT NULL DEFAULT 0,"
                + " package TEXT,"
                + " label TEXT,"
                + " allow INTEGER NOT NULL,"       // 0 allow, 1 deny
                + " until INTEGER NOT NULL DEFAULT 0," // 0 = forever (remember)
                + " created INTEGER NOT NULL,"
                + " UNIQUE(from_uid, to_uid));");
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldV, int newV) {
        db.execSQL("DROP TABLE IF EXISTS policy");
        onCreate(db);
    }

    /** Insert or replace a remembered rule, then refresh the flat mirror. */
    public synchronized void setRule(int fromUid, int toUid, String pkg,
                                     String label, boolean allow) {
        SQLiteDatabase db = getWritableDatabase();
        ContentValues v = new ContentValues();
        v.put("from_uid", fromUid);
        v.put("to_uid", toUid);
        v.put("package", pkg);
        v.put("label", label);
        v.put("allow", allow ? ALLOW : DENY);
        v.put("created", System.currentTimeMillis() / 1000);
        db.replace("policy", null, v);
        syncFlatFile(db);
    }

    /** Revoke a previously granted/denied rule. */
    public synchronized void revoke(int fromUid, int toUid) {
        SQLiteDatabase db = getWritableDatabase();
        db.delete("policy", "from_uid=? AND to_uid=?",
                new String[]{String.valueOf(fromUid), String.valueOf(toUid)});
        syncFlatFile(db);
    }

    public List<Rule> listRules() {
        List<Rule> out = new ArrayList<Rule>();
        Cursor c = getReadableDatabase().query("policy",
                null, null, null, null, null, "label COLLATE NOCASE ASC");
        while (c.moveToNext()) {
            Rule r = new Rule();
            r.fromUid = c.getInt(c.getColumnIndex("from_uid"));
            r.toUid   = c.getInt(c.getColumnIndex("to_uid"));
            r.pkg     = c.getString(c.getColumnIndex("package"));
            r.label   = c.getString(c.getColumnIndex("label"));
            r.allow   = c.getInt(c.getColumnIndex("allow")) == ALLOW;
            r.created = c.getLong(c.getColumnIndex("created"));
            out.add(r);
        }
        c.close();
        return out;
    }

    /** Rewrite the flat policy file consumed by the native daemon. */
    private void syncFlatFile(SQLiteDatabase db) {
        try {
            File f = new File(POLICY_FILE);
            FileWriter w = new FileWriter(f, false);
            Cursor c = db.query("policy", null, null, null, null, null, null);
            while (c.moveToNext()) {
                int fu = c.getInt(c.getColumnIndex("from_uid"));
                int tu = c.getInt(c.getColumnIndex("to_uid"));
                boolean allow = c.getInt(c.getColumnIndex("allow")) == ALLOW;
                w.write(fu + " " + tu + " " + (allow ? "ALLOW" : "DENY") + "\n");
            }
            c.close();
            w.flush();
            w.close();
            // daemon runs as root; make sure it can read the mirror.
            f.setReadable(true, false);
        } catch (Exception e) {
            // Non-fatal: UI still works; daemon falls back to INTERACTIVE.
        }
    }

    /** Simple value object for the UI list. */
    public static class Rule {
        public int fromUid, toUid;
        public String pkg, label;
        public boolean allow;
        public long created;
    }
}

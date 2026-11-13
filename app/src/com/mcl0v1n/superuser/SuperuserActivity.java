package com.mcl0v1n.superuser;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.List;
import java.util.Locale;

/**
 * Mcl0v1n vs android - main manager UI.
 *
 * Shows the list of apps/uids that have requested root, their current
 * ALLOW/DENY rule, lets the user revoke rules, and opens the access log.
 * Mirrors SuperSU's core screens but is intentionally lightweight for KitKat.
 */
public class SuperuserActivity extends Activity {

    private PolicyDatabase db;
    private ListView list;
    private RuleAdapter adapter;

    @Override
    protected void onCreate(Bundle b) {
        super.onCreate(b);
        setContentView(R.layout.activity_main);

        db = new PolicyDatabase(this);
        list = (ListView) findViewById(R.id.rule_list);

        // Make sure the watcher/daemon is up when the user opens the app.
        startService(new Intent(this, RequestWatcherService.class));

        refresh();

        list.setOnItemClickListener(new android.widget.AdapterView.OnItemClickListener() {
            public void onItemClick(android.widget.AdapterView<?> parent,
                                    View view, int pos, long id) {
                confirmRevoke(adapter.getItem(pos));
            }
        });
    }

    @Override
    protected void onResume() {
        super.onResume();
        refresh();
    }

    private void refresh() {
        List<PolicyDatabase.Rule> rules = db.listRules();
        adapter = new RuleAdapter(this, rules);
        list.setAdapter(adapter);
        TextView empty = (TextView) findViewById(R.id.empty);
        empty.setVisibility(rules.isEmpty() ? View.VISIBLE : View.GONE);
    }

    private void confirmRevoke(final PolicyDatabase.Rule r) {
        new AlertDialog.Builder(this)
                .setTitle(r.label != null ? r.label : ("uid " + r.fromUid))
                .setMessage("Current: " + (r.allow ? "ALLOWED" : "DENIED")
                        + "\n\nRevoke this rule? The app will be asked again "
                        + "next time it requests root.")
                .setPositiveButton("Revoke", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface d, int w) {
                        db.revoke(r.fromUid, r.toUid);
                        Toast.makeText(SuperuserActivity.this,
                                "Revoked", Toast.LENGTH_SHORT).show();
                        refresh();
                    }
                })
                .setNegativeButton("Cancel", null)
                .show();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu m) {
        m.add(0, 1, 0, "Access log");
        m.add(0, 2, 0, "About");
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == 1) {
            startActivity(new Intent(this, LogActivity.class));
            return true;
        } else if (item.getItemId() == 2) {
            new AlertDialog.Builder(this)
                    .setTitle("Mcl0v1n vs android")
                    .setMessage("Persistent root for Android 4.4.4\n"
                            + "Samsung Galaxy S III (GT-I9300)\nv1.0")
                    .setPositiveButton("OK", null).show();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    /** Adapter rendering one rule row. */
    static class RuleAdapter extends ArrayAdapter<PolicyDatabase.Rule> {
        private final SimpleDateFormat fmt =
                new SimpleDateFormat("yyyy-MM-dd", Locale.US);

        RuleAdapter(Context c, List<PolicyDatabase.Rule> items) {
            super(c, 0, items);
        }

        @Override
        public View getView(int pos, View convert, ViewGroup parent) {
            if (convert == null) {
                convert = View.inflate(getContext(), R.layout.row_rule, null);
            }
            PolicyDatabase.Rule r = getItem(pos);
            TextView title = (TextView) convert.findViewById(R.id.title);
            TextView sub   = (TextView) convert.findViewById(R.id.subtitle);
            TextView badge = (TextView) convert.findViewById(R.id.badge);

            title.setText(r.label != null ? r.label
                    : (r.pkg != null ? r.pkg : "uid " + r.fromUid));
            sub.setText("uid " + r.fromUid + " \u2192 " + r.toUid
                    + "  \u00b7  " + fmt.format(new Date(r.created * 1000)));
            badge.setText(r.allow ? "ALLOW" : "DENY");
            badge.setTextColor(r.allow ? 0xFF2E7D32 : 0xFFC62828);
            return convert;
        }
    }
}

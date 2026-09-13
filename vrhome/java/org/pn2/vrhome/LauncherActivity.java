package org.pn2.vrhome;

import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.graphics.Color;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.GridView;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.util.List;

// The app library, itself just a 2D activity living on a panel display.
// Tapping an entry asks the shell for a new panel and launches the app there.
public class LauncherActivity extends Activity {

    @Override protected void onCreate(Bundle b) {
        super.onCreate(b);
        final PackageManager pm = getPackageManager();
        Intent q = new Intent(Intent.ACTION_MAIN).addCategory(Intent.CATEGORY_LAUNCHER);
        final List<ResolveInfo> apps = pm.queryIntentActivities(q, 0);

        GridView grid = new GridView(this);
        grid.setNumColumns(4);
        grid.setBackgroundColor(0xff101418);
        grid.setVerticalSpacing(24);
        grid.setPadding(24, 24, 24, 24);

        grid.setAdapter(new BaseAdapter() {
            @Override public int getCount() { return apps.size(); }
            @Override public Object getItem(int i) { return apps.get(i); }
            @Override public long getItemId(int i) { return i; }

            @Override public View getView(int i, View v, ViewGroup parent) {
                LinearLayout cell = new LinearLayout(LauncherActivity.this);
                cell.setOrientation(LinearLayout.VERTICAL);
                cell.setGravity(android.view.Gravity.CENTER);
                ImageView icon = new ImageView(LauncherActivity.this);
                icon.setImageDrawable(apps.get(i).loadIcon(pm));
                cell.addView(icon, new LinearLayout.LayoutParams(140, 140));
                TextView label = new TextView(LauncherActivity.this);
                label.setText(apps.get(i).loadLabel(pm));
                label.setTextColor(Color.WHITE);
                label.setTextSize(13);
                label.setGravity(android.view.Gravity.CENTER);
                label.setSingleLine(true);
                cell.addView(label);
                return cell;
            }
        });

        grid.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override public void onItemClick(AdapterView<?> p, View v, int i, long id) {
                String pkg = apps.get(i).activityInfo.packageName;
                if (!pkg.equals(getPackageName()))
                    ShellBridge.openPackage(pkg);
            }
        });

        setContentView(grid);
    }
}

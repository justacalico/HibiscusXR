package gitlab.neosalsa.hud;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

// Backup boot path for the HUD; the init rc starts it too, but
// BOOT_COMPLETED covers a boot where the rc fired before the app was
// installed. startForegroundService is allowed from this broadcast.
public class BootReceiver extends BroadcastReceiver {
    @Override public void onReceive(Context ctx, Intent i) {
        ctx.startForegroundService(new Intent(ctx, HudService.class));
    }
}

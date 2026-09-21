package gitlab.neosalsa.home;

import android.app.NativeActivity;
import android.os.Bundle;

// The home environment's only activity: the HOME target on the physical
// display, rendering scenery. Panels, the app library and the summonable
// menu all live in the HUD service (gitlab.neosalsa.hud).
public class PanelActivity extends NativeActivity {
    @Override protected void onCreate(Bundle b) {
        super.onCreate(b);
        CoverWatch.start();
    }
}

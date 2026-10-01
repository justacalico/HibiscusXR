package gitlab.neosalsa.home;

import android.Manifest;
import android.app.NativeActivity;
import android.content.pm.PackageManager;
import android.graphics.SurfaceTexture;
import android.os.Bundle;
import android.view.Surface;

// The home environment's only activity: the HOME target on the physical
// display, rendering the passthrough feed. Panels, the app library and
// the summonable menu all live in the HUD service (gitlab.neosalsa.hud).
public class PanelActivity extends NativeActivity {
    private static final int REQ_CAM = 44;

    @Override protected void onCreate(Bundle b) {
        super.onCreate(b);
        CoverWatch.start();
        // a dev install without the default-permissions grant still gets
        // the prompt; a properly flashed build is granted silently
        if (checkSelfPermission(Manifest.permission.CAMERA) !=
                PackageManager.PERMISSION_GRANTED)
            requestPermissions(new String[]{Manifest.permission.CAMERA},
                               REQ_CAM);
    }

    // ------------------------------------------------------------ camera

    // The tracking pair is exposed as a single camera whose 1280x400
    // stream carries both eyes side by side. The render thread owns a
    // SurfaceTexture on an external texture and hands its Surface to the
    // camera session; frames land without an ImageReader round-trip.
    private static SurfaceTexture camTex;
    private static Surface camSurf;

    public static boolean camPermOk() {
        return sThis != null &&
               sThis.checkSelfPermission(Manifest.permission.CAMERA) ==
                       PackageManager.PERMISSION_GRANTED;
    }

    public static SurfaceTexture camTexture(int texId, int w, int h) {
        camRelease();
        camTex = new SurfaceTexture(texId);
        camTex.setDefaultBufferSize(w, h);
        camSurf = new Surface(camTex);
        return camTex;
    }

    public static Surface camSurface() { return camSurf; }

    public static void camRelease() {
        if (camSurf != null) { camSurf.release(); camSurf = null; }
        if (camTex != null) { camTex.release(); camTex = null; }
    }

    private static PanelActivity sThis;

    @Override protected void onResume() {
        super.onResume();
        sThis = this;
    }

    @Override protected void onPause() {
        sThis = null;
        super.onPause();
    }
}

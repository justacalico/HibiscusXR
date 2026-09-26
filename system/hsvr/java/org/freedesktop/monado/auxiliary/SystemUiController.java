// Copyright 2021, Qualcomm Innovation Center, Inc.
// SPDX-License-Identifier: BSL-1.0
// Ported from SystemUiController.kt to plain Java for the minimal build.
package org.freedesktop.monado.auxiliary;

import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.view.WindowInsets;
import android.view.WindowInsetsController;

/** Helper class that handles system ui visibility. */
public class SystemUiController {
    private final Impl impl;

    public SystemUiController(View view) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            impl = new WindowInsetsControllerImpl(view);
        } else {
            impl = new SystemUiVisibilityImpl(view);
        }
    }

    /** Hide system ui and make fullscreen. */
    public void hide() {
        impl.hide();
    }

    private abstract static class Impl {
        final View view;
        private final Handler uiHandler = new Handler(Looper.getMainLooper());

        Impl(View view) {
            this.view = view;
        }

        abstract void hide();

        void runOnUiThread(Runnable runnable) {
            uiHandler.post(runnable);
        }
    }

    @SuppressWarnings("deprecation")
    private static class SystemUiVisibilityImpl extends Impl {
        private static final int FLAG_FULL_SCREEN_IMMERSIVE_STICKY =
                View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;

        SystemUiVisibilityImpl(View view) {
            super(view);
            runOnUiThread(() -> view.setOnSystemUiVisibilityChangeListener(
                    visibility -> {
                        if (0 == (visibility & View.SYSTEM_UI_FLAG_FULLSCREEN)) {
                            hide();
                        }
                    }));
        }

        @Override
        void hide() {
            runOnUiThread(() -> view.setSystemUiVisibility(FLAG_FULL_SCREEN_IMMERSIVE_STICKY));
        }
    }

    private static class WindowInsetsControllerImpl extends Impl {
        WindowInsetsControllerImpl(View view) {
            super(view);
            runOnUiThread(() -> {
                WindowInsetsController c = view.getWindowInsetsController();
                if (c != null) {
                    c.addOnControllableInsetsChangedListener((controller, typeMask) -> {
                        if ((typeMask & WindowInsets.Type.displayCutout()) != 0
                                || (typeMask & WindowInsets.Type.statusBars()) != 0
                                || (typeMask & WindowInsets.Type.navigationBars()) != 0) {
                            hide();
                        }
                    });
                }
            });
        }

        @Override
        void hide() {
            runOnUiThread(() -> {
                WindowInsetsController c = view.getWindowInsetsController();
                if (c != null) {
                    c.hide(WindowInsets.Type.displayCutout()
                            | WindowInsets.Type.statusBars()
                            | WindowInsets.Type.navigationBars());
                    c.setSystemBarsBehavior(
                            WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
                }
            });
        }
    }
}

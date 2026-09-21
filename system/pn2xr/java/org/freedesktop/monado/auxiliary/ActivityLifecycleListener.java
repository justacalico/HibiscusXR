// Copyright 2022, Qualcomm Innovation Center, Inc.
// SPDX-License-Identifier: BSL-1.0
// Ported from ActivityLifecycleListener.kt to plain Java for the minimal build.
package org.freedesktop.monado.auxiliary;

import android.app.Activity;
import android.app.Application;
import android.os.Bundle;
import android.util.Log;

/** Monitor activity lifecycle of application. */
public class ActivityLifecycleListener implements Application.ActivityLifecycleCallbacks {
    private static final String TAG = "ActivityLifecycleListener";
    private final long nativePtr;

    public ActivityLifecycleListener(long nativePtr) {
        this.nativePtr = nativePtr;
    }

    /** Register callback with the Application from the given activity. */
    public void registerCallback(Activity activity) {
        activity.getApplication().registerActivityLifecycleCallbacks(this);
    }

    /** Unregister callback with the Application from the given activity. */
    public void unregisterCallback(Activity activity) {
        activity.getApplication().unregisterActivityLifecycleCallbacks(this);
    }

    @Override
    public void onActivityCreated(Activity activity, Bundle savedInstanceState) {
        Log.i(TAG, activity + " onActivityCreated");
        nativeOnActivityCreated(nativePtr, activity);
    }

    @Override
    public void onActivityStarted(Activity activity) {
        Log.i(TAG, activity + " onActivityStarted");
        nativeOnActivityStarted(nativePtr, activity);
    }

    @Override
    public void onActivityResumed(Activity activity) {
        Log.i(TAG, activity + " onActivityResumed");
        nativeOnActivityResumed(nativePtr, activity);
    }

    @Override
    public void onActivityPaused(Activity activity) {
        Log.i(TAG, activity + " onActivityPaused");
        nativeOnActivityPaused(nativePtr, activity);
    }

    @Override
    public void onActivityStopped(Activity activity) {
        Log.i(TAG, activity + " onActivityStopped");
        nativeOnActivityStopped(nativePtr, activity);
    }

    @Override
    public void onActivitySaveInstanceState(Activity activity, Bundle outState) {
        Log.i(TAG, activity + " onActivitySaveInstanceState");
        nativeOnActivitySaveInstanceState(nativePtr, activity);
    }

    @Override
    public void onActivityDestroyed(Activity activity) {
        Log.i(TAG, activity + " onActivityDestroyed");
        nativeOnActivityDestroyed(nativePtr, activity);
    }

    private native void nativeOnActivityCreated(long nativePtr, Activity activity);
    private native void nativeOnActivityStarted(long nativePtr, Activity activity);
    private native void nativeOnActivityResumed(long nativePtr, Activity activity);
    private native void nativeOnActivityPaused(long nativePtr, Activity activity);
    private native void nativeOnActivityStopped(long nativePtr, Activity activity);
    private native void nativeOnActivitySaveInstanceState(long nativePtr, Activity activity);
    private native void nativeOnActivityDestroyed(long nativePtr, Activity activity);
}

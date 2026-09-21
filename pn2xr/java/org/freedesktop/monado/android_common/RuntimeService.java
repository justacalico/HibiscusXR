// Copyright 2020, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
package org.freedesktop.monado.android_common;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;

/** Service whose metadata exposes the OpenXR runtime to the Khronos loader. */
public class RuntimeService extends Service {
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
}

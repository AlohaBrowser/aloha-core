/*
 * This file is part of eyeo Chromium SDK,
 * Copyright (C) 2006-present eyeo GmbH
 *
 * eyeo Chromium SDK is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * eyeo Chromium SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with eyeo Chromium SDK.  If not, see <http://www.gnu.org/licenses/>.
 */

package org.chromium.adblock.shell;

import android.app.Application;
import android.content.Context;

import org.chromium.android_webview.AwLocaleConfig;
import org.chromium.base.CommandLine;
import org.chromium.base.ContextUtils;
import org.chromium.base.PathUtils;
import org.chromium.ui.base.ResourceBundle;

/** The android_webview shell Application subclass. */
public class AdblockShellApplication extends Application {
    // Called by the framework for ALL processes. Runs before ContentProviders are created.
    // Quirk: context.getApplicationContext() returns null during this method.
    @Override
    protected void attachBaseContext(Context context) {
        super.attachBaseContext(context);
        ContextUtils.initApplicationContext(this);
        PathUtils.setPrivateDataDirectorySuffix("webview", "WebView");
        CommandLine.initFromFile("/data/local/tmp/adblock-webview-command-line");
        ResourceBundle.setAvailablePakLocales(AwLocaleConfig.getWebViewSupportedPakLocales());
    }
}

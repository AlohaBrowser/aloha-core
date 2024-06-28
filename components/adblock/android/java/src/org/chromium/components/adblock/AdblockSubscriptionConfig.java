// Copyright 2024 Aloha Mobile Ltd.

// Permission is hereby granted, free of charge, to any person obtaining 
// a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the Software
// is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION 
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
package org.chromium.components.adblock;

import android.util.Log;
import android.webkit.URLUtil;

import androidx.annotation.UiThread;

import org.chromium.base.ThreadUtils;
import org.chromium.base.library_loader.LibraryLoader;

import java.net.URL;
import java.util.Arrays;
import java.util.List;

import org.jni_zero.CalledByNative;
import org.jni_zero.NativeMethods;

// ALOHA https://app.clickup.com/t/86eph6cjh
public class AdblockSubscriptionConfig {
    private static final String TAG = AdblockSubscriptionConfig.class.getSimpleName();
   
    public static class Subscription {
        private URL mUrl;
        private String mTitle;
        private String mVersion = "";
        private String[] mLanguages = {};

        public Subscription(final URL url, final String title, final String version) {
            this.mUrl = url;
            this.mTitle = title;
            this.mVersion = version;
        }

        @CalledByNative("Subscription")
        public Subscription(
                final URL url, final String title, final String version, final String[] languages) {
            this.mUrl = url;
            this.mTitle = title;
            this.mVersion = version;
            this.mLanguages = languages;
        }

        public String title() {
            return mTitle;
        }

        public URL url() {
            return mUrl;
        }

        public String version() {
            return mVersion;
        }

        public String[] languages() {
            return mLanguages;
        }

        @Override
        public boolean equals(final Object object) {
            if (object == null) return false;
            if (getClass() != object.getClass()) return false;

            Subscription other = (Subscription) object;
            return url().equals(other.url());
        }
    }
    
    @UiThread
    public static List<Subscription> getRecommendedSubscriptions() {
        return (List<Subscription>)
                (List<?>) Arrays.asList(AdblockSubscriptionConfigJni.get().getRecommendedSubscriptions());
    }

    @NativeMethods
    interface Natives {
        Object[] getRecommendedSubscriptions();
    }
}

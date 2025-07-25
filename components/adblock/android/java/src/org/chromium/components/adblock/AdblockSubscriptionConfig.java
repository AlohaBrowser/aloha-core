
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

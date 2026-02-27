// Copyright 2026 Aloha Mobile Ltd.

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

package com.alohamobile.bromium;

import android.os.Looper;
import android.util.Log;
import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;

/**
 * Process-wide "is the engine started" state shared by {@code BromiumInitializer} (glue layer)
 * and the lower-level facades such as {@link BromiumProfiles} (browser layer), which cannot
 * depend on the glue layer. The initializer installs a synchronous flush via
 * {@link #setSyncFlush} so that any UI-thread entry point can finish a pending asynchronous
 * startup inline with {@link #ensureStarted()}.
 *
 * <p>The state is terminal in both directions: once {@link #markReady()} or
 * {@link #markFailed(Throwable)} ran, the other is a no-op. After a failure every entry point
 * throws {@link IllegalStateException} carrying the original cause rather than reaching a
 * half-started engine.
 */
public final class BromiumStartupState {
    private static final String TAG = "BromiumStartup";

    /** Released on ready and on failure, so that background waiters never hang. */
    private static final CountDownLatch sDoneLatch = new CountDownLatch(1);

    private static volatile boolean sReady;
    private static volatile @Nullable Throwable sFailure;
    private static final List<Runnable> sReadyCallbacks = new ArrayList<>();
    private static @Nullable Runnable sSyncFlush;
    private static boolean sFlushing;

    private BromiumStartupState() {}

    public static boolean isReady() {
        return sReady;
    }

    /** The latched startup failure, or null while starting / when ready. */
    public static @Nullable Throwable getFailure() {
        return sFailure;
    }

    /** True while {@link #ensureStarted()} is finishing the startup inline (UI thread). */
    @MainThread
    public static boolean isFlushing() {
        return sFlushing;
    }

    /**
     * Runs a Runnable on the UI thread once the engine is ready (inline if it already is). The
     * Runnable is dropped, with a log line, if the startup has failed.
     */
    @MainThread
    public static void runWhenReady(@NonNull Runnable r) {
        assertOnMainThread();
        if (sReady) {
            r.run();
        } else if (sFailure != null) {
            Log.w(TAG, "runWhenReady(): dropping a callback, the engine failed to start");
        } else {
            sReadyCallbacks.add(r);
        }
    }

    /**
     * Finishes a pending asynchronous startup synchronously on the UI thread. No-op when the
     * engine is ready; throws {@link IllegalStateException} when the startup has failed (with
     * the original cause, also when it fails inside this very call) or was never requested.
     */
    @MainThread
    public static void ensureStarted() {
        if (sReady) {
            return;
        }
        assertOnMainThread();
        throwIfFailed();
        if (sFlushing) {
            // A startup task (or host code it reached) came back here before the engine was
            // marked ready; let it proceed as before, but leave a trace of the misuse.
            Log.e(TAG, "ensureStarted(): re-entrant call during the inline startup", new Throwable());
            return;
        }
        Runnable flush = sSyncFlush;
        if (flush == null) {
            // Nothing installed: BromiumInitializer.initializeAllAsync() was never called in
            // this process (or returned early in a child process). Touching the engine now
            // would reach JNI without a library; make the cause visible instead.
            throw new IllegalStateException("Bromium startup was not requested in this process");
        }
        sFlushing = true;
        Log.w(TAG, "ensureStarted(): finishing the asynchronous startup inline on the UI thread");
        try {
            flush.run();
        } finally {
            sFlushing = false;
        }
    }

    /**
     * Blocks a background thread until the engine is ready. Never call on the UI thread. Throws
     * with the original cause when the startup has failed. Blocks forever if the startup is never
     * requested in this process (child processes): callers decide that before waiting.
     */
    public static void awaitReady() {
        if (Looper.getMainLooper().isCurrentThread()) {
            throw new IllegalStateException("awaitReady() on the UI thread; use ensureStarted()");
        }
        boolean interrupted = false;
        while (true) {
            try {
                sDoneLatch.await();
                break;
            } catch (InterruptedException e) {
                // Keep waiting: callers cannot do anything useful with a half-started engine.
                interrupted = true;
            }
        }
        if (interrupted) {
            Thread.currentThread().interrupt();
        }
        throwIfFailed();
    }

    // ---- for BromiumInitializer only ----

    /** Installs the UI-thread routine that completes startup inline. */
    @MainThread
    public static void setSyncFlush(@Nullable Runnable flush) {
        sSyncFlush = flush;
    }

    /** Marks the engine ready and drains {@link #runWhenReady} callbacks (UI thread). */
    @MainThread
    public static void markReady() {
        if (sReady || sFailure != null) {
            return;
        }
        sReady = true;
        sSyncFlush = null;
        sDoneLatch.countDown();
        List<Runnable> callbacks = new ArrayList<>(sReadyCallbacks);
        sReadyCallbacks.clear();
        for (Runnable r : callbacks) {
            try {
                r.run();
            } catch (RuntimeException e) {
                // One misbehaving callback must not starve the others of the ready signal.
                Log.e(TAG, "runWhenReady() callback failed", e);
            }
        }
    }

    /**
     * Latches a startup failure (UI thread). Returns true for the first failure, false for the
     * rest and after {@link #markReady()}; the queued callbacks are dropped and background
     * waiters are released.
     */
    @MainThread
    public static boolean markFailed(@NonNull Throwable cause) {
        if (sReady || sFailure != null) {
            return false;
        }
        sFailure = cause;
        sSyncFlush = null;
        sReadyCallbacks.clear();
        sDoneLatch.countDown();
        return true;
    }

    private static void throwIfFailed() {
        Throwable failure = sFailure;
        if (failure != null) {
            throw new IllegalStateException("Bromium startup failed", failure);
        }
    }

    private static void assertOnMainThread() {
        if (!Looper.getMainLooper().isCurrentThread()) {
            throw new IllegalStateException("Must be called on the main thread");
        }
    }
}

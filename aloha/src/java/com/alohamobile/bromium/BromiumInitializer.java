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

package com.alohamobile.bromium;

import android.content.Context;
import android.os.Build;
import android.os.Looper;
import android.os.Trace;
import android.util.Log;
import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import org.chromium.android_webview.AwBrowserProcess;
import org.chromium.android_webview.AwClassPreloader;
import org.chromium.android_webview.AwThreadUtils;
import org.chromium.android_webview.StartupCallSite;
import org.chromium.android_webview.StartupTasks;
import org.chromium.android_webview.StartupTasksRunner;
import org.chromium.android_webview.AwContents;
import org.chromium.android_webview.common.AwResource;
import org.chromium.android_webview.common.AwSwitches;
import org.chromium.base.CommandLine;
import org.chromium.android_webview.common.WebViewCachedFlags;
import org.chromium.android_webview.gfx.AwDrawFnImpl;
import org.chromium.base.ContextUtils;
import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.base.metrics.ScopedSysTraceEvent;
import org.chromium.base.task.PostTask;
import org.chromium.content_public.browser.BrowserStartupController;
import org.chromium.content_public.browser.BrowserStartupController.StartupCallback;
import org.chromium.content_public.browser.ChildProcessCreationParams;
import org.chromium.content_public.browser.ChildProcessLauncherHelper;
import org.chromium.ui.base.ResourceBundle;
import com.alohamobile.bromium.BromiumResources;
import com.android.webview.chromium.DrawFunctor;
import com.android.webview.chromium.GraphicsUtils;

import java.util.ArrayDeque;

/**
 * Engine bootstrap for the embedded (AAR) build.
 *
 * <p>{@link #initializeAllAsync} loads the library and does the other thread-safe work on a
 * background thread, then starts the browser process asynchronously in UI-looper-sized chunks
 * (upstream {@code StartupTasksRunner} + {@code startBrowserProcessesAsync}). The host receives
 * {@link Listener#onReady()} on the UI thread; anything that needs the engine before that must
 * call {@link #ensureStarted()} (UI thread) or {@link #awaitReady()} (background threads).
 *
 * <p>A failure of any step is latched in {@link BromiumStartupState}: the engine stays unusable,
 * the host gets {@link Listener#onFailure} once, and {@link #ensureStarted()} /
 * {@link #awaitReady()} rethrow the cause instead of touching a half-started engine.
 *
 * <p>The steps are wrapped in {@code android.os.Trace} sections ({@code Bromium.*}) so that a
 * Macrobenchmark / Perfetto trace shows how much of the app startup the engine takes.
 */
public class BromiumInitializer {
    private static final String TAG = "BromiumInitializer";
    private static final String CHROMIUM_PREFS_NAME = "WebViewChromiumPrefs";
    private static final String TRACE_STARTUP = "Bromium.startup";

    /** Completion callbacks of {@link #initializeAllAsync}; both run on the UI thread. */
    public interface Listener {
        void onReady();

        /** Library load, pre-native setup or browser process start failed; the engine is unusable. */
        void onFailure(@NonNull Throwable error);
    }

    private static final String[] UNCOMPRESS_LOCALES = {
        // Default locale:
        "en-US",
        // Locales from aloha-browser-android/app/build.gradle.kts:
        "ar", "de", "el", "es", "fa", "fr", "iw", "hi", "in", "it", "ja", "ko", "ms", "nl", "pt",
        "ru", "sv", "th", "tr", "vi", "zh-TW", "zh-CN", "uk", "cs", "ro", "pl", "hu", "ca", "hr",
        "da", "fi", "no", "sk", "bn",
    };

    // ---- startup state (UI thread unless noted); ready/failed/callbacks live in
    // BromiumStartupState so that browser-layer facades can flush without depending on glue.
    private static volatile @Nullable Thread sPreloadThread;
    private static volatile @Nullable Throwable sPreloadFailure;
    private static boolean sUiStartRequested;
    private static boolean sStartupCompleted;
    private static @Nullable StartupTasksRunner sRunner;
    private static @Nullable Listener sListener;
    private static @Nullable Context sAppContext;

    public static boolean isReady() {
        return BromiumStartupState.isReady();
    }

    /**
     * Starts the engine without blocking the UI thread. Returns immediately; the listener is
     * invoked on the UI thread. Must be called on the UI thread, once per process.
     */
    @MainThread
    public static void initializeAllAsync(
            Context context, Integer configKeySystemUuidMapping, @NonNull Listener listener) {
        // ALOHA MultiProcess CU-86eqe7rd2: the renderer isolatedProcess
        // services also run the app's Application.onCreate — browser
        // initialization must NOT run in child processes (they are bootstrapped
        // via ChildProcessService), otherwise it double-inits and crashes.
        if (!context.getPackageName().equals(ContextUtils.getProcessName())) {
            return;
        }
        assertOnMainThread();
        if (sListener != null || sPreloadThread != null) {
            throw new IllegalStateException("initializeAllAsync() called twice");
        }
        sListener = listener;
        BromiumStartupState.setSyncFlush(BromiumInitializer::flushSyncOnUi);
        beginAsyncTrace();

        final Context appContext = context.getApplicationContext();
        sAppContext = appContext;
        ContextUtils.initApplicationContext(appContext);
        // ALOHA: as in StartupTasks.preBrowserProcessStartStepOne — Java UI PostTasks queue up
        // until native enables the UI runner in completeStartup(); UI_STARTUP still runs.
        PostTask.disablePreNativeUiTasks(true);

        // Thread-safe part: library load, resources, cached flags. Same pattern as Chrome's
        // AsyncInitTaskRunner (plain Thread so it is not throttled).
        Thread preload =
                new Thread(
                        () -> {
                            try (ScopedSysTraceEvent e = ScopedSysTraceEvent.scoped("Bromium.preload")) {
                                preloadOnAnyThread(appContext, configKeySystemUuidMapping);
                            } catch (Throwable t) {
                                Log.e(TAG, "Bromium preload failed", t);
                                sPreloadFailure = t;
                            }
                            AwThreadUtils.postToUiThreadLooper(
                                    BromiumInitializer::onPreloadFinishedOnUi);
                        },
                        "BromiumPreload");
        sPreloadThread = preload;
        preload.start();
    }

    /** Runs a Runnable on the UI thread once the engine is ready (inline if it already is). */
    @MainThread
    public static void runWhenReady(@NonNull Runnable r) {
        BromiumStartupState.runWhenReady(r);
    }

    /**
     * Finishes startup synchronously on the UI thread if it has not completed yet. Throws
     * {@link IllegalStateException} with the original cause if the startup has failed.
     */
    @MainThread
    public static void ensureStarted() {
        BromiumStartupState.ensureStarted();
    }

    /**
     * Blocks a background thread until the engine is ready. Never call on the UI thread. Throws
     * {@link IllegalStateException} with the original cause if the startup has failed.
     */
    public static void awaitReady() {
        BromiumStartupState.awaitReady();
    }

    // ---- internals (UI thread) ----

    /** Installed into BromiumStartupState; runs only while an async startup is pending. */
    private static void flushSyncOnUi() {
        Thread preload = sPreloadThread;
        if (preload != null && preload.isAlive()) {
            joinUninterruptibly(preload);
        }
        Throwable failure = sPreloadFailure;
        if (failure != null) {
            failOnUi(failure);
            throw new IllegalStateException("Bromium startup failed", failure);
        }
        startOnUi();
        StartupTasksRunner runner = sRunner;
        if (runner != null && !BromiumStartupState.isReady()) {
            // Upstream PARTIAL_ASYNC_THEN_SYNC: run whatever is left of the task queues
            // inline; BrowserStartupController flushes the pending native startup tasks.
            runner.run(StartupCallSite.ASYNC_WEBVIEW_STARTUP, /* triggeredFromUIThread= */ true);
        }
        failure = BromiumStartupState.getFailure();
        if (failure != null) {
            throw new IllegalStateException("Bromium startup failed", failure);
        }
    }

    private static void onPreloadFinishedOnUi() {
        if (sUiStartRequested || BromiumStartupState.isReady()) {
            // ensureStarted() got there first.
            return;
        }
        Throwable failure = sPreloadFailure;
        if (failure != null) {
            failOnUi(failure);
            return;
        }
        startOnUi();
        StartupTasksRunner runner = sRunner;
        if (runner != null) {
            runner.run(StartupCallSite.ASYNC_WEBVIEW_STARTUP, /* triggeredFromUIThread= */ false);
        }
    }

    /**
     * Latches the failure once; the engine stays unusable and the remaining startup tasks become
     * no-ops. The listener is notified on the UI thread: inline normally, posted when the failure
     * surfaced inside a synchronous flush (the caller of that flush gets the exception instead).
     */
    private static void failOnUi(Throwable failure) {
        if (!BromiumStartupState.markFailed(failure)) {
            return;
        }
        endAsyncTrace();
        sRunner = null;
        PostTask.disablePreNativeUiTasks(false);
        Listener l = sListener;
        if (l == null) {
            return;
        }
        if (BromiumStartupState.isFlushing()) {
            AwThreadUtils.postToUiThreadLooper(() -> l.onFailure(failure));
        } else {
            l.onFailure(failure);
        }
    }

    /** Idempotent: UI-thread-only setup after preload, builds the startup runner. */
    private static void startOnUi() {
        if (sUiStartRequested) {
            return;
        }
        sUiStartRequested = true;
        Context context = sAppContext;
        configureChildProcessLauncher(context);

        // ALOHA MultiProcess CU-86eqe7rd2: the switch must be in CommandLine
        // BEFORE finishBrowserProcessStart() — AwBrowserProcess.start()
        // computes singleProcess = !isMultiProcess() BEFORE launching
        // ContentMain, i.e. earlier than the force in
        // AwMainDelegate::BasicStartupComplete. After loadLibrary() CommandLine
        // is already native-backed (switchCommandLineForWebView), so the append
        // is visible to both the Java and C++ sides. Done on the UI thread: the
        // native CommandLine is not thread-safe and the host may read it here.
        if (!CommandLine.getInstance().hasSwitch(AwSwitches.WEBVIEW_SANDBOXED_RENDERER)) {
            CommandLine.getInstance().appendSwitch(AwSwitches.WEBVIEW_SANDBOXED_RENDERER);
        }

        // ALOHA: upstream M157 moved AwBrowserProcess.runPreBrowserProcessStart() /
        // finishBrowserProcessStart() into StartupTasks (driven by StartupController in the
        // glue layer, which Bromium does not use). These two steps are their 1:1 equivalents;
        // the rest of StartupTasks (preBrowserProcessStartStepOne, post-start steps) is
        // system-WebView-only work that Bromium never ran before either.
        ArrayDeque<Runnable> pre = new ArrayDeque<>();
        ArrayDeque<Runnable> post = new ArrayDeque<>();
        pre.addLast(
                () ->
                        runStartupTask(
                                "Bromium.preBrowserProcessStart",
                                StartupTasks::preBrowserProcessStartStepTwo));
        // In the async run the browser process is already started here and
        // startBrowserProcessesSync() inside is a no-op; in a sync flush it does the actual
        // start. Either way PowerMonitor etc. get created.
        post.addLast(
                () ->
                        runStartupTask(
                                "Bromium.finishBrowserProcessStart",
                                StartupTasks::finishBrowserProcessStart));
        post.addLast(BromiumInitializer::completeStartupOnUi);

        sRunner =
                new StartupTasksRunner(
                        new StartupTasksRunner.Delegate() {
                            @Override
                            public void onStartupTimingsReady(
                                    StartupTasksRunner.StartupTimings timings) {
                                if (BromiumStartupState.getFailure() != null) {
                                    return;
                                }
                                // Called right after the last task, outside the runner's timed
                                // region, for both the async run and a sync flush: hand over to
                                // the host here so its work is not accounted as engine startup.
                                BromiumStartupState.markReady();
                                Listener l = sListener;
                                if (l != null) {
                                    l.onReady();
                                }
                            }

                            @Override
                            public void onStartupFailed(RuntimeException e) {
                                failOnUi(e);
                            }

                            @Override
                            public void onStartupFailed(Error e) {
                                failOnUi(e);
                            }

                            @Override
                            public boolean isStartupFinished() {
                                return sStartupCompleted;
                            }

                            @Override
                            public void doAsyncBrowserStartup(StartupCallback callback) {
                                if (BromiumStartupState.getFailure() != null) {
                                    // A failed pre-task: leave the runner parked, the host has
                                    // been told already.
                                    return;
                                }
                                // Thrown here (prepareToStartBrowserProcess, command line
                                // setup) the exception would escape the runner's onCompletion
                                // onto the UI looper; latch it like every other step.
                                try (ScopedSysTraceEvent e =
                                        ScopedSysTraceEvent.scoped(
                                                "Bromium.startBrowserProcessesAsync")) {
                                    BrowserStartupController.getInstance()
                                            .startBrowserProcessesAsync(
                                                    LibraryProcessType.PROCESS_WEBVIEW,
                                                    /* startGpuProcess= */ false,
                                                    /* startMinimalBrowser= */ false,
                                                    !isMultiProcess(),
                                                    new StartupCallback() {
                                                        @Override
                                                        public void onSuccess(
                                                                @Nullable
                                                                BrowserStartupController
                                                                                .StartupMetrics
                                                                        metrics) {
                                                            callback.onSuccess(metrics);
                                                        }

                                                        @Override
                                                        public void onFailure() {
                                                            // The runner would throw on the UI
                                                            // looper; latch instead.
                                                            failOnUi(
                                                                    new IllegalStateException(
                                                                            "Browser process"
                                                                                + " startup failed"));
                                                        }
                                                    });
                                } catch (Throwable t) {
                                    Log.e(TAG, "startBrowserProcessesAsync failed", t);
                                    failOnUi(t);
                                }
                            }
                        },
                        pre,
                        post,
                        StartupTasksRunner.StartupRequestMode.ASYNC);
    }

    /**
     * Runs one startup task unless the startup has already failed; a throwing task latches the
     * failure instead of escaping to the UI looper (where the runner would crash the process).
     */
    private static void runStartupTask(String traceName, Runnable task) {
        if (BromiumStartupState.getFailure() != null) {
            return;
        }
        try (ScopedSysTraceEvent e = ScopedSysTraceEvent.scoped(traceName)) {
            task.run();
        } catch (Throwable t) {
            Log.e(TAG, "Bromium startup task failed: " + traceName, t);
            failOnUi(t);
        }
    }

    /** Last runner task; the ready hand-over happens in onStartupTimingsReady right after. */
    private static void completeStartupOnUi() {
        if (sStartupCompleted || BromiumStartupState.getFailure() != null) {
            return;
        }
        sStartupCompleted = true;
        // ALOHA: since M157 (323399593b365) native UI task execution stays disabled until
        // AwContentBrowserClient::OnStartupComplete(); without this call no navigation ever
        // commits. Mirrors StartupController.finishStartup() / AwBrowserProcess.startForTesting().
        runStartupTask(
                "Bromium.onStartupComplete",
                () -> {
                    PostTask.disablePreNativeUiTasks(false);
                    AwBrowserProcess.onStartupComplete();
                });
        if (BromiumStartupState.getFailure() != null) {
            // failOnUi() already closed the trace; nothing below may touch the engine.
            return;
        }
        endAsyncTrace();
        Bromium.clearTemporaryDownloads();
    }

    // ---- preload steps (any thread) ----

    /** Everything that only needs the library file and the app context. */
    private static void preloadOnAnyThread(Context appContext, Integer configKeySystemUuidMapping) {
        AwResource.setResources(appContext.getResources());
        AwResource.setConfigKeySystemUuidMapping(configKeySystemUuidMapping);
        BromiumResources.init(appContext);

        try (ScopedSysTraceEvent e = ScopedSysTraceEvent.scoped("Bromium.loadLibrary")) {
            System.loadLibrary("webviewchromium_plat_support");
            AwBrowserProcess.loadLibrary(null);
        }

        // Upstream runs this off the UI thread too (StartupTasks.runNonUiThreadCapableStartupTasks
        // under WebViewMoveWorkToProviderInitThreadPool); otherwise it would land inside
        // BrowserStartupController.prepareToStartBrowserProcess() on the UI thread.
        try (ScopedSysTraceEvent e = ScopedSysTraceEvent.scoped("Bromium.libraryInit")) {
            LibraryLoader.getInstance().ensureInitialized();
        }

        ResourceBundle.setAvailablePakLocales(UNCOMPRESS_LOCALES);
        AwDrawFnImpl.setDrawFnFunctionTable(DrawFunctor.getDrawFnFunctionTable());
        AwContents.setAwDrawSWFunctionTable(GraphicsUtils.getDrawSWFunctionTable());

        WebViewCachedFlags.init(
                appContext.getSharedPreferences(CHROMIUM_PREFS_NAME, Context.MODE_PRIVATE));

        // Upstream default (kWebViewBackgroundClassPreloading): warm the heavy classes early.
        AwClassPreloader.preloadClasses();
    }

    private static void configureChildProcessLauncher(Context context) {
        // ALOHA MultiProcess CU-86eqe7rd2: the app package hosts the sandboxed
        // services (also needed for crash/metrics intents via
        // getWebViewPackageName()).
        AwBrowserProcess.setWebViewPackageName(context.getPackageName());

        // ALOHA MultiProcess CU-86eqe7rd2: embedded child-launcher config
        // instead of AwBrowserProcess.configureChildProcessLauncher(). The
        // stock variant targets the system WebView: isExternalService=true →
        // binding with Context.BIND_EXTERNAL_SERVICE, which requires the
        // services to be exported="true" + android:externalService="true". Our
        // SandboxedProcessService0..N are declared in the app manifest with
        // exported="false" + a signature permission — so we bind the normal way
        // within our own package (like configureChildProcessLauncherForTesting).
        ChildProcessCreationParams.set(
                context.getPackageName(),
                context.getPackageName(),
                /* isExternalSandboxedService= */ false,
                LibraryProcessType.PROCESS_WEBVIEW_CHILD,
                /* bindToCallerCheck= */ true,
                /* ignoreVisibilityForImportance= */ true,
                /* forceNativeSandboxedService= */ false);
        ChildProcessLauncherHelper.initialize();
    }

    // ---- helpers ----

    private static boolean isMultiProcess() {
        return CommandLine.getInstance().hasSwitch(AwSwitches.WEBVIEW_SANDBOXED_RENDERER);
    }

    /** One async atrace slice from the kick-off to ready/failed: the engine's share of startup. */
    private static void beginAsyncTrace() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            Trace.beginAsyncSection(TRACE_STARTUP, 0);
        }
    }

    private static void endAsyncTrace() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            Trace.endAsyncSection(TRACE_STARTUP, 0);
        }
    }

    private static void assertOnMainThread() {
        if (!Looper.getMainLooper().isCurrentThread()) {
            throw new IllegalStateException("Must be called on the main thread");
        }
    }

    private static void joinUninterruptibly(Thread t) {
        boolean interrupted = false;
        while (true) {
            try {
                t.join();
                break;
            } catch (InterruptedException e) {
                interrupted = true;
            }
        }
        if (interrupted) {
            Thread.currentThread().interrupt();
        }
    }
}

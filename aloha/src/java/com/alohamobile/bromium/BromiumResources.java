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
import android.content.res.Resources;
import android.content.res.XmlResourceParser;
import android.util.Log;

import org.xmlpull.v1.XmlPullParser;

import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

/**
 * Remaps the baked resource IDs in chromium's precompiled R classes to the IDs the host assigned
 * when it merged the AAR's {@code res/} into its own resource table ("model A").
 *
 * <p>Chromium ships precompiled R classes with IDs baked at build time (package {@code 0x7f}). The
 * AAR ships raw {@code res/}, and the host {@code aapt2} merges those resources into the app's own
 * {@code 0x7f} package — so chromium resources are present in EVERY context's AssetManager by
 * construction (stable across vendors, no per-context patching, no dynamic id). But the merge
 * renumbers entries, so the baked IDs are stale and must be remapped by name at startup.
 *
 * <p>Source of the merged IDs: the AAR build ({@code make_aar.py}, {@code generate_res_index})
 * ships {@code res/xml/bromium_res_index.xml} whose {@code <i n="type/field" r="@type/name"/>}
 * entries reference every shipped resource. The host {@code aapt2} resolves each reference into
 * the final merged ID at link time (the same mechanism that fixes up every {@code @drawable} in
 * every layout, and it fails the HOST build loudly if a resource goes missing). Here we just
 * bulk-read the ready IDs from the compiled XML's attributes — no per-name lookups, no binary
 * format parsing. A compiled-XML attribute (unlike a {@code TypedArray} item, which follows alias
 * chains to the final target) holds the RAW reference, i.e. the id of the named resource itself —
 * exactly what an R field must contain. {@code <f n="type/field"/>} entries list the few R fields
 * that shipped code reads but whose resources we don't ship (host androidx copies may satisfy
 * them by name) — only those go through {@code getIdentifier}. {@code getIdentifier} also remains
 * the full fallback if the index is absent; in verbose mode every indexed ID is cross-checked
 * against {@code getIdentifier}.
 */
public class BromiumResources {
    private static final String TAG = "BromiumResources";

    // Chromium's precompiled R to fix. org.chromium.ui.R$<type> extends the shared
    // gen.base_module.R$<type>, so writing that superclass's fields fixes the whole webview R.
    private static final String CHROMIUM_R = "org.chromium.ui.R";
    // Must stay in sync with res_index_types in make_aar.py.
    private static final String[] TYPES = {
        "string", "id", "dimen", "style", "layout", "color", "attr", "drawable", "menu", "integer"
    };

    // Verbose logging + per-field index/getIdentifier cross-check. OFF by default (incl. release).
    // Enable on ANY build WITHOUT rebuilding:
    //     adb shell setprop log.tag.BromiumResources DEBUG    (then relaunch the app)
    private static boolean sVerbose;

    public static void init(Context context) {
        Context app = context.getApplicationContext();
        sVerbose = Log.isLoggable(TAG, Log.DEBUG);
        long t0 = System.nanoTime();
        Resources res = app.getResources();
        String pkg = app.getPackageName();

        Map<String, Integer> index = null;
        List<String> fallbackKeys = new ArrayList<>();
        long indexNs = 0;
        try {
            long i0 = System.nanoTime();
            index = readIndex(res, pkg, fallbackKeys);
            indexNs = System.nanoTime() - i0;
        } catch (Throwable t) {
            Log.w(TAG, "resource index read failed; falling back to getIdentifier", t);
        }
        int indexSize = (index != null) ? index.size() : -1;

        // Fields read by shipped code but not backed by a shipped resource: they may still exist
        // in the host by name (its own androidx copies) — resolve those few via getIdentifier,
        // exactly like the full remap used to. See generate_res_index in make_aar.py.
        int fbTotal = 0, fbResolved = 0;
        long fallbackNs = 0;
        if (index != null) {
            long f0 = System.nanoTime();
            for (String entry : fallbackKeys) {
                fbTotal++;
                int slash = entry.indexOf('/');
                int id = res.getIdentifier(
                        entry.substring(slash + 1), entry.substring(0, slash), pkg);
                if (id != 0) {
                    index.put(entry, id);
                    fbResolved++;
                }
            }
            fallbackNs = System.nanoTime() - f0;
        }

        int total = 0, viaIndex = 0, viaGetId = 0, missing = 0;
        int mismatch = 0, hostByName = 0, indexOnly = 0;
        long getIdNs = 0;
        for (String type : TYPES) {
            Field[] targets;
            try {
                targets = Class.forName(CHROMIUM_R + "$" + type).getSuperclass().getDeclaredFields();
            } catch (ClassNotFoundException e) {
                if (sVerbose) Log.i(TAG, "no chromium R$" + type + ", skipping");
                continue;
            }
            for (Field f : targets) {
                total++;
                int id;
                if (index != null) {
                    // The index covers every R field backed by a shipped resource, so a miss means
                    // the resource is genuinely absent (phantom R entry) — 0, same as before.
                    Integer fromIndex = index.get(type + "/" + f.getName());
                    if (fromIndex != null) {
                        id = fromIndex;
                        viaIndex++;
                    } else {
                        id = 0;
                        missing++;
                    }
                } else {
                    // Index failed entirely: fall back to per-field getIdentifier (slow, correct).
                    long g = System.nanoTime();
                    id = res.getIdentifier(f.getName(), type, pkg);
                    getIdNs += System.nanoTime() - g;
                    if (id != 0) viaGetId++;
                    else missing++;
                }
                try {
                    f.setAccessible(true);
                    f.setInt(null, id);
                } catch (Throwable ignore) {
                }
                if (sVerbose && index != null) {
                    int real = res.getIdentifier(f.getName(), type, pkg);
                    if (id != 0 && real != 0 && real != id) {
                        // An indexed entry disagreeing with getIdentifier is a REAL error.
                        mismatch++;
                        if (mismatch <= 10) {
                            Log.w(TAG, "index/getId MISMATCH " + type + "/" + f.getName()
                                    + " index=0x" + Integer.toHexString(id)
                                    + " real=0x" + Integer.toHexString(real));
                        }
                    } else if (id == 0 && real != 0) {
                        // Not shipped by us, but the host defines the same name (its own androidx
                        // copies). Unread by shipped code (jar scan) → left 0 by design.
                        hostByName++;
                    } else if (id != 0 && real == 0) {
                        // Indexed, but getIdentifier can't see it — dotted names; index is better.
                        indexOnly++;
                    }
                }
            }
        }

        if (missing > 0 && sVerbose) {
            Log.i(TAG, missing + "/" + total + " chromium resources unresolved (phantom R entries)");
        }
        Log.i(TAG, String.format(Locale.US,
                "remap %.2f ms  [index %.2f, fallback %.2f, getIdentifier %.2f]  total=%d"
                        + " viaIndex=%d viaGetId=%d fallback=%d/%d missing=%d indexSize=%d%s",
                (System.nanoTime() - t0) / 1e6, indexNs / 1e6, fallbackNs / 1e6, getIdNs / 1e6,
                total, viaIndex, viaGetId, fbResolved, fbTotal, missing, indexSize,
                sVerbose ? (" validateMismatch=" + mismatch + " hostByName=" + hostByName
                        + " indexOnly=" + indexOnly) : ""));
    }

    /**
     * Reads the generated index XML merged into the host package: {@code <i n="type/field"
     * r="@..."/>} entries whose {@code r} references the host aapt2 already resolved into final
     * merged IDs at link time. The compiled-XML attribute is read RAW (no alias-chain following),
     * so the id is that of the named resource itself. {@code <f n="type/field"/>} entries are
     * collected into {@code fallbackOut} for per-name getIdentifier resolution by the caller.
     * Returns null (→ full getIdentifier fallback) if the index is absent.
     */
    private static Map<String, Integer> readIndex(
            Resources res, String pkg, List<String> fallbackOut) throws Exception {
        int xmlId = res.getIdentifier("bromium_res_index", "xml", pkg);
        if (xmlId == 0) {
            Log.w(TAG, "bromium_res_index.xml not found in package " + pkg);
            return null;
        }
        Map<String, Integer> map = new HashMap<>(4096);
        XmlResourceParser p = res.getXml(xmlId);
        try {
            for (int ev = p.next(); ev != XmlPullParser.END_DOCUMENT; ev = p.next()) {
                if (ev != XmlPullParser.START_TAG) continue;
                String tag = p.getName();
                if ("i".equals(tag)) {
                    String key = p.getAttributeValue(null, "n");
                    if (key == null) continue;
                    int id = p.getAttributeResourceValue(null, "r", 0);
                    if (id == 0) {
                        // ?attr/name entries compile to TYPE_ATTRIBUTE, which
                        // getAttributeResourceValue doesn't treat as a reference; its raw string
                        // form is "?<decimal id>".
                        String v = p.getAttributeValue(null, "r");
                        if (v != null && v.length() > 1
                                && (v.charAt(0) == '?' || v.charAt(0) == '@')) {
                            try {
                                id = Integer.parseInt(v.substring(1));
                            } catch (NumberFormatException ignore) {
                            }
                        }
                    }
                    if (id != 0) map.put(key, id);
                } else if ("f".equals(tag)) {
                    String key = p.getAttributeValue(null, "n");
                    if (key != null) fallbackOut.add(key);
                }
            }
        } finally {
            p.close();
        }
        return map;
    }
}

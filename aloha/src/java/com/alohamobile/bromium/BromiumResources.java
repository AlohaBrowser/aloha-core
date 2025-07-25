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

import java.lang.NoSuchFieldException;
import java.lang.reflect.Field;
import android.content.Context;
import android.content.res.Resources;
import android.util.Log;

public class BromiumResources {
    private static final String TAG = "BromiumResources";

    public static void init(Context context) {
        processPackage(context.getApplicationContext(), "org.chromium.ui");
    }

    private static void processPackage(Context appContext, String packageName) {
        processType(appContext, packageName, "string");
        processType(appContext, packageName, "id");
        processType(appContext, packageName, "dimen");
        processType(appContext, packageName, "style");
        processType(appContext, packageName, "layout");
        processType(appContext, packageName, "color");
        processType(appContext, packageName, "attr");
        processType(appContext, packageName, "drawable");
        processType(appContext, packageName, "menu");
        processType(appContext, packageName, "integer");
    }

    private static void processType(Context appContext, String packageName, String type) {
        try {
            Class<?> Rclass = Class.forName(packageName + ".R$" + type).getSuperclass();
            Field[] fields = Rclass.getDeclaredFields();
            Resources resources = appContext.getResources();
            String contextPackageName = appContext.getPackageName();
            for(Field f: fields) {

                try {
                    f.setAccessible(true);
                    int id = resources.getIdentifier(f.getName(), type, contextPackageName);
                    f.setInt(null, id);
                }
                catch(RuntimeException e) {
                    try {
                        f.setInt(null, 0);
                    }
                    catch(Exception estub) {
                    }
                    Log.w(TAG, "Skipping field in type <" + type + "> for package " + packageName);
                }
            }
        } catch (ClassNotFoundException e) {
            Log.w(TAG, "Skipping type <" + type + "> for package " + packageName);
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        }
    }
}

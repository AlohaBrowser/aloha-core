// Copyright 2025 Aloha Mobile Ltd.

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

package com.alohamobile.bromium.ui;

import android.view.View;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.base.Callback;


// Delegates select dialog actions to Kotlin implementation
// ALOHA https://app.clickup.com/t/86evkdxey
@NullMarked
public abstract class SelectDialogHandler {

    // Show select dialog
    // returns true if the action is overridden in Kotlin
    public abstract boolean shouldOverrideShow(View anchorView,
            long nativeSelectPopupSourceFrame,
            String[] items,
            int[] itemTypes,
            boolean multiple,
            int[] selectedIndices,
            boolean rightAligned,
            Callback<int[]> callback);

    // Can be called from native to hide the dialog without triggering cancel action
    public abstract void hideWithoutCancel();
}

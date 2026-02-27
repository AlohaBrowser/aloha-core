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

package com.alohamobile.bromium;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import com.alohamobile.bromium.SelectionPopupHandler;
import com.alohamobile.bromium.ui.SelectDialogHandler;

// This class delegates some UI-related behaviors to BromiumUI Kotlin implementation
// ALOHA https://app.clickup.com/t/31a1wgq
public class BromiumUI {

    private final String TAG = "BromiumUI";

    private static @Nullable BromiumUI sInstance;

    // Handler for selection popup actions
    private @Nullable SelectionPopupHandler selectionPopupHandler;

    // Handler for select dialog actions
    private @Nullable SelectDialogHandler selectDialogHandler;

    public static synchronized BromiumUI getInstance() {
        if (sInstance == null) {
            sInstance = new BromiumUI();
        }
        return sInstance;
    }

    // ALOHA https://app.clickup.com/t/86et68rp5
    public void setSelectionPopupHandler(SelectionPopupHandler handler) {
        selectionPopupHandler = handler;
    }
    // ALOHA https://app.clickup.com/t/86et68rp5
    public SelectionPopupHandler getSelectionPopupHandler() {
        return selectionPopupHandler;
    }

    // ALOHA https://app.clickup.com/t/86evkdxey
    public void setSelectDialogHandler(SelectDialogHandler dialogHandler) {
        selectDialogHandler = dialogHandler;
    }
    // ALOHA https://app.clickup.com/t/86evkdxey
    public SelectDialogHandler getSelectDialogHandler() {
        return selectDialogHandler;
    }
}

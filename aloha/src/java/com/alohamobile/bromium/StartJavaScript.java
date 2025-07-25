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

import android.os.Build;
import org.chromium.android_webview.AwContents;

// ALOHA https://app.clickup.com/t/86ert96vk
public class StartJavaScript {
    
    public static void addJavaScript(AwContents awContents) {
        String[] origins = {"*"};
        awContents.addDocumentStartJavaScript(getBarcodeDetectorMockScrip(), origins);
    }
    
    private static String getBarcodeDetectorMockScrip() {
        String script = "class AlohaFaceDetector {\n" +
                "  constructor(options = {}) {\n" +
                "  }\n" +
                "\n" +
                "  async detect(image) {\n" +
                "    return Promise.reject(new Error('Not supported'));\n" +
                "  }\n" +
                "}\n" +
                "\n" +
                "class AlohaTextDetector {\n" +
                "  constructor() {\n" +
                "  }\n" +
                "\n" +
                "  async detect(image) {\n" +
                "    return Promise.reject(new Error('Not supported'));\n" +
                "  }\n" +
                "}\n" +
                "\n" +
                "class AlohaBarcodeDetector {\n" +
                "  constructor(options = {}) {\n" +
                "  }\n" +
                "\n" +
                "  async detect(image) {\n" +
                "    return Promise.reject(new Error('Not supported'));\n" +
                "  }\n" +
                "\n" +
                "  static async getSupportedFormats() {\n" +
                "    return [''];\n" +
                "  }\n" +
                "}\n" +
                "\n" +
                "Object.defineProperty(window, 'BarcodeDetector', {\n" +
                "  value: AlohaBarcodeDetector,\n" +
                "  writable: false,\n" +
                "  configurable: true\n" +
                "});\n" +
                "\n" +
                "\n" +
                "Object.defineProperty(window, 'FaceDetector', {\n" +
                "  value: AlohaFaceDetector,\n" +
                "  writable: false,\n" +
                "  configurable: true\n" +
                "});\n" +
                "\n" +
                "Object.defineProperty(window, 'TextDetector', {\n" +
                "  value: AlohaTextDetector,\n" +
                "  writable: false,\n" +
                "  configurable: true\n" +
                "});\n";

        return script;
    }
}
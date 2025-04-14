# Element hiding

Intercepting network requests is not enough to filter some content.

On some web pages, eyeo Chromium SDK hides content after it has loaded by injecting CSS or JavaScript code.

This is implemented as follows:

1. `TabHelpers::AttachTabHelpers` for chromium, or `AwContents` constructor for android_webview based integration, registers `AdblockWebContentObserver` to receive a notification that the page is loaded.
2. `ElementHider` generates CSS and injects it via `RenderFrameHost::InsertAbpElemhideStylesheet`
3. `ElementHider` generates JavaScript code and injects via `RenderFrameHost::ExecuteJavaScriptInIsolatedWorld`

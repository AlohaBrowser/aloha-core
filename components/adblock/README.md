# Eyeo Chromium SDK integration

Eyeo Chromium SDK is a fork of the [Chromium project](https://chromium.googlesource.com/chromium/src) that integrates ad-filtering capabilities. This fork serves as a Software Development Kit (SDK) for extending browsers based on Chromium.

For a detailed reasoning on why we implement our own ad-filtering solution instead of basing it in Chromium's Subresource Filter, check the corresponding [decision record](docs/adr/not-extending-subresource-filter.md).

Please send any questions or report any issues to distribution-partners@eyeo.com.

## General information

The [ad-filtering documentation](docs/ad-filtering) describes the available ad-blocking functionalities and how they relate to filter lists. This is done not only from a functional perspective, but also providing more in-depth details, like sequence diagrams illustrating the ad-blocking flow.

The [data collection documentation](docs/data-collection) describes what information is sent to the eyeo services, and our commitment to preserving user's privacy.

The [settings documentation](docs/settings) describes the interfaces to configure the ad-blocking integration.

Finally, the [Architecture Decision Record](docs/adr) documents the reasoning behind certain decisions taken during the development of eyeo Chromium SDK.


## Partner integration steps

### Check dependencies and prerequisites

Eyeo Chromium SDK depends on several parts of Chromium, such as the following:

* [KeyedService](/components/keyed_service/core/keyed_service.h)
* [Network Service](/services/network/)
* [PrefService](/components/prefs/pref_service.h)
* [Profile](/chrome/browser/profiles/profile.h)
* [Resources](/components/resources/)
* [Version Info](/components/version_info/)

If you cannot include these or any other parts of Chromium in your browser, you will have to re-implement them or work around them.

### Understand the available interfaces

The eyeo Chromium SDK APIs allow to interact with the ad-filtering engine:
- to enable/disable it,
- configure selected filter lists and filters,
- receive notifications about blocking or allowing events.

The SDK can be controlled by:
- C++ API
- Java API (on Android)
- JavaScript Extension API (on Linux, Windows, MacOS)

The SDK extends the browser's Settings UI with an "Ad blocking" section on:
- Android
- Linux, Windows, MacOS

In order to understand what settings are available and which API is best for your use case, check the [settings documentation](docs/settings/README.md).

### Prepare your application

Follow the [integration how-to](docs/integration-how-to.md) to configure the ad-filtering engine. You will also find information about how to set up your application name and version.

### Upgrade scenario: Find out what has changed between eyeo Chromium SDK releases

Differences across versions are listed in [the changelog](components/adblock/CHANGELOG.md).

You can also use our [interdiff script](tools/eyeo/generate_interdiffs.sh) to compare two git revision ranges. You can find more information in the [integration how-to](docs/integration-how-to.md).


## For eyeo Chromium SDK contributors

Adblock strives to follow the [layered component design](https://sites.google.com/a/chromium.org/dev/developers/design-documents/layered-components-design)

You will find most of eyeo Chromium SDK specific code in the following places:

* `components/adblock/core`: Platform-agnostic ad filtering integration
* `components/adblock/content`: `content` dependent ad filtering integration
* `chrome/browser/adblock`: OS-agnostic but Chrome-specific integration
* `components/adblock/android`: Android-specific JNI code for UI
* `chrome/renderer/adblock`: Hooks in the Renderer process to observe Renderer-issued resource loads
* `chrome/common/extensions`: Implementation of the `adblockPrivate` Extension API
* `components/adblock/android/java/src/org/chromium/components/adblock`: Android implementation of Settings UI

You can find how our implementation maps to Chromium's design in the [design overview](docs/design-overview.md).

General information for developers, like options for logging, testing, etc, can be found in the [developer notes](docs/developer-notes.md).

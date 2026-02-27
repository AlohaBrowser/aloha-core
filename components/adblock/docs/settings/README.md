# Settings

eyeo Chromium SDK can be configured to provide the experience most adequate for the end-user.

## Default settings
By default eyeo Chromium SDK creates and uses a filtering configuration named "adblock" and populates it with
the recommended subscriptions defined in `components/adblock/core/subscription/subscription_config.cc`.

In order to build the browser with resource filtering disabled by default `gn gen` argument `eyeo_disable_filtering_by_default=true` needs to be specified.

At the moment, the following settings are supported for this default configuration:

| Setting                 | Type    | Description |
| ----------------------- | ------- | ----------- |
| Ad-blocking             | Boolean | Whether ad-blocking is enabled or disabled. |
| Acceptable Ads          | Boolean | Whether Acceptable Ads are displayed or hidden. |
| Subscriptions           | List    | Subscriptions to filter lists. |
| Allowed domains         | List    | Domains on which no ads will be blocked, even when ad-blocking is enabled. |
| Custom filters          | List    | Additional filters implemented via [filter language](https://help.eyeo.com/adblockplus/how-to-write-filters) |

### Implementation details

Default settings can be configured in C++ via `FilteringConfiguration` class instance representing "adblock" configuration or in Java via `org.chromium.components.adblock.AdblockController`.

The Browser Extension API defined and documented in `chrome/common/extensions/api/adblock_private.idl` uses the C++ implementation. Android UI fragments consume the Java one.

The [user setting sequence diagram](user-setting-sequence.png) describes the flow of updating a setting, and how it varies depending on the API being used.

## Advanced settings
eyeo Chromium SDK allows to dynamically create multiple independent filtering configurations which can be managed separately.
During resource filtering, each filtering configuration is queried for filters and the logic is as follows:
- If `every` configuration contains a `$document` allowlisting filter then the whole page is allowlisted
- If `any` configuration returns a blocking decision for the request then the request is blocked.

At the moment, the following settings are supported for each filtering configuration:

| Setting                 | Type    | Description |
| ----------------------- | ------- | ----------- |
| Enabled                 | Boolean | Whether filtering configuration is enabled or disabled. Disabled configurations don't participate in resource filtering. |
| Subscriptions           | List    | Subscriptions to filter lists. |
| Allowed domains         | List    | Domains on which no requests will be blocked, even when filtering configuration is enabled. |
| Custom filters          | List    | Additional filters implemented via [filter language](https://help.eyeo.com/adblockplus/how-to-write-filters) |

### Implementation details

Settings for each filtering configuration can be configured via the C++ instance of `FilteringConfiguration` class or its Java counterpart, `org.chromium.components.adblock.FilteringConfiguration`.

The Browser Extension API defined and documented in `chrome/common/extensions/api/eyeo_filtering_private.idl` uses the C++ implementation.

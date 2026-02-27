# Integration how-to guides

This section provides guidance (including code snippets) on setting up ad-filtering in your browser.


## How to set the application identifier?

We recommend overriding the values returned from [version_info::GetProductName()](components/version_info/version_info.cc) and [version_info::GetVersionNumber()](components/version_info/version_info.cc) by setting the values of `PRODUCT_NAME` and `PRODUCT_VERSION` constants:

* `PRODUCT_NAME` is derived from [chrome/app/theme/chromium/BRANDING](chrome/app/theme/chromium/BRANDING) file.
* `PRODUCT_VERSION` is derived from [chrome/VERSION](chrome/VERSION) file.

You can either modify those files or have the functions return different strings.

If this is not convenient (changes of the above affect the whole application) this still can be overridden with the gn `eyeo_application_name` and `eyeo_application_version` variables:

```sh
gn gen --args='eyeo_application_name="My great browser" eyeo_application_version="99.99.0.1"...' ...
```

## How to set build arguments for user counting?

These `gn` arguments are required for the SDK to correctly count active users and attribute usage to the product.

* `eyeo_telemetry_client_id`
* `eyeo_telemetry_activeping_auth_token`

The values of these arguments shall be provided by eyeo. To set them, generate the project build files like this:

```sh
gn gen --args='eyeo_telemetry_client_id="mycompany" eyeo_telemetry_activeping_auth_token="peyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9" ...' ...
```

If these arguments are not set, eyeo may not be able to correctly attribute users to your product.

Always keep the auth token secret, do not embed it in open-source repositories or documents. If the auth token leaks out, contact eyeo and you will receive a new token.

User counting does not transfer any user-identifiable data to eyeo, nor does it allow tracking or profiling users by eyeo.

## How to disable ad-blocking on a specific domain?

Call [AdblockController](components/adblock/android/java/src/org/chromium/components/adblock/AdblockController.java)'s `addAllowedDomain()` method.

The following example makes eyeo Chromium SDK cease blocking ads on the specified domain:

```java
import org.chromium.components.adblock.AdblockController;

String domain = "example.com";
AdblockController.getInstance(Profile.getLastUsedRegularProfile()).addAllowedDomain(domain);
```

To re-enable blocking ads on the specified domain:

```java
import org.chromium.components.adblock.AdblockController;

String domain = "example.com";
AdblockController.getInstance(Profile.getLastUsedRegularProfile()).removeAllowedDomain(domain);
```

Note: Pass a *domain* (`example.com`) as an argument, not a URL (`http://www.example.com/page.html`).


## How to receive notifications about blocked or allowed network requests?

Implement the `ResourceClassificationNotifier.ResourceFilteringObserver` interface.

Add your observer by calling `ResourceClassificationNotifier.getInstance(...).addResourceFilteringObserver()` and remove it by
calling `ResourceClassificationNotifier.getInstance(...).removeResourceFilteringObserver()`. Do not add a `ResourceFilteringObserver` if you are not going to consume these notifications, as it has a small performance penalty.


## How to add and remove subscription(s) to my filter lists?

Call [AdblockController](chrome/android/java/src/org/chromium/chrome/browser/adblock/AdblockController.java)'s `installSubscription()` method.

The following example makes eyeo Chromium SDK install (download and parse) the filter list:

```java
import org.chromium.components.adblock.AdblockController;

URL myFilterList = new URL("http://example.com/my_list.txt");
AdblockController.getInstance(Profile.getLastUsedRegularProfile()).installSubscription(myFilterList);
```

For snippet filters from the filter list to be supported, the list must be listed in `config::GetKnownSubscriptions` in `components/adblock/core/subscription_config.cc` with SubscriptionPrivilegedFilterStatus::Allowed.

To remove a custom filter list:

```java
import org.chromium.components.adblock.AdblockController;

URL myFilterList = new URL("http://example.com/my_list.txt");
AdblockController.getInstance(Profile.getLastUsedRegularProfile()).uninstallSubscription(myFilterList);
```


## How to change the default filter lists?

By default, the following filter lists are installed:

* ABP Anti-circumvention filter list
* EasyList + filters specific for the currently set device language (e.g. EasyList Italy)
* Acceptable Ads

To install different filter lists by default, modify `config::GetKnownSubscriptions` in `components/adblock/core/subscription_config.cc`. Here is a sample structure to add:

```cpp
static std::vector<SubscriptionInfo> recommendations = {
    ...
     {"https://domain.com/subscription.txt",        // URL
      "My custom filters",                          // Display name for settings
      {},                                           // Supported languages list, considered for
                                                    // SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch
      SubscriptionUiVisibility::Visible,            // Should the app show a subscription in the settings
      SubscriptionFirstRunBehavior::Subscribe       // Should the app subscribe on first run
      SubscriptionPrivilegedFilterStatus::Forbidden // Allow snippets and header filters or not
     },
```

Consider adding filter lists also as bundled filter lists (see below).


## How to bundle filter lists?

Bundling filter lists allows the browser to use them without previously downloading them from a server. This is especially useful when the network is unreliable or slow.

We provide the following bundled filter lists:

* `components/resources/adblocking/anticv.txt`
* `components/resources/adblocking/easylist.txt`
* `components/resources/adblocking/exceptionrules.txt`

In order to bundle more filter lists:
1. Download the filter list and place it in `components/resources/adblocking`, e.g. `components/resources/adblocking/my_list.txt`.
2. Add a target that converts the text-format list into a FlatBuffer in `components/resources/adblocking/BUILD.gn`:
    ```
    make_preloaded_subscription("make_my_list") {
      input = "//components/resources/adblocking/my_list.txt"
      url = "https://my.server.com/my_list.txt"
      output = "${target_gen_dir}/my_list.fb"
    }

    group("make_all_preloaded_subscriptions") {
      deps = [
        ":make_anticv",
        ":make_easylist",
        ":make_exceptionrules",
        ":make_my_list",
      ]
    }

    ```
3. Add the generated FlatBuffer file into the ResourceBundle via `components/adblock/core/resources/adblock_resources.grd`:
    ```
    <include name="IDR_ADBLOCK_FLATBUFFER_MY_LIST" file="${root_gen_dir}/components/resources/adblocking/my_list.fb" use_base_dir="false" type="BINDATA" compress="gzip" />
    ```
4. Teach `PreloadedSubscriptionProviderImpl` at `components/adblock/core/subscription/preloaded_subscription_provider_impl.h` when to provide the bundled list:
    ```cpp
    class PreloadedSubscriptionProviderImpl final
        : public PreloadedSubscriptionProvider {

      ...
      scoped_refptr<Subscription> preloaded_anticv_;
      scoped_refptr<Subscription> preloaded_my_list_;
    };
    ```
    And in `components/adblock/core/preloaded_subscription_provider_impl.cc`
    ```cpp
    std::vector<scoped_refptr<Subscription>>
    PreloadedSubscriptionProviderImpl::GetCurrentPreloadedSubscriptions() const {
      std::vector<scoped_refptr<Subscription>> result;
      if (preloaded_easylist_)
        result.push_back(preloaded_easylist_);
      if (preloaded_exceptionrules_)
        result.push_back(preloaded_exceptionrules_);
      if (preloaded_anticv_)
        result.push_back(preloaded_anticv_);
      if (preloaded_my_list_)
        result.push_back(preloaded_my_list_);
      return result;
    }

    void PreloadedSubscriptionProviderImpl::UpdateSubscriptionsInternal() {
      UpdatePreloadedSubscription(preloaded_easylist_, "*easylist.txt",
                                  installed_subscriptions_, pending_subscriptions_,
                                  IDR_ADBLOCK_FLATBUFFER_EASYLIST);
      UpdatePreloadedSubscription(preloaded_exceptionrules_, "*exceptionrules.txt",
                                  installed_subscriptions_, pending_subscriptions_,
                                  IDR_ADBLOCK_FLATBUFFER_EXCEPTIONRULES);
      UpdatePreloadedSubscription(preloaded_anticv_, "*abp-filters-anti-cv.txt",
                                  installed_subscriptions_, pending_subscriptions_,
                                  IDR_ADBLOCK_FLATBUFFER_ANTICV);
      UpdatePreloadedSubscription(preloaded_my_list_, "*my_list.txt",
                                  installed_subscriptions_, pending_subscriptions_,
                                  IDR_ADBLOCK_FLATBUFFER_MY_LIST);
    }
    ```

These steps have added a version of `https://my.server.com/my_list.txt` into the ResourceBundle. This bundled filter list will be used whenever:
- `https://my.server.com/my_list.txt` has been requested to install but has yet to finish downloading
- A website is loading and requires ad-filtering functions immediately

The bundled filter list will be replaced by a version of the filter list downloaded from the Internet as soon as it becomes available.


## How to test filters?

Eyeo provides testing pages for eyeo Chromium SDK's ad-blocking features on https://abptestpages.org. You need to subscribe to the accompanying [test filter list](https://abptestpages.org/en/abp-testcase-subscription.txt) before loading them. To learn how to subscribe to a specific filter list, consult "How to add and remove subscription(s) to my filter lists" in this page.


## How to find out what has changed between eyeo Chromium SDK releases?

Differences across versions are listed in [the changelog](components/adblock/CHANGELOG.md).

You can also use our [interdiff script](tools/eyeo/generate_interdiffs.sh) to compare two git revision ranges.

To get a combined patch, execute the script to obtain the changes introduced by eyeo Chromium SDK compared to vanilla Chromium in the old branch, and the same for the new new branch. You can obtain the necessary hashes for the revision ranges from release announcements. To get help on how to use the script, run `./generate_interdiffs.sh --help`.

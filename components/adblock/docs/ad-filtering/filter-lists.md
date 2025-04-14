# Filter lists

In order to decide which requests should be blocked and which elements hidden, eyeo Chromium SDK consults [Adblock Plus filter rules](https://help.eyeo.com/adblockplus/how-to-write-filters), distributed within filter lists.

Filter lists are hosted online in plain text format. eyeo Chromium SDK downloads them, converts them to [FlatBuffer](https://google.github.io/flatbuffers/) format and stores them on disk. The [decision record](../adr/storing-filter-lists-in-flatbuffers-format.md) explains the reasoning for choosing FlatBuffers.


## Default filter lists

eyeo Chromium SDK comes with the following filter lists selected by default:
* EasyList + language-specific rules based on system region (e.g. [EasyList Germany+EasyList](https://easylist-downloads.adblockplus.org/easylistgermany+easylist.txt) for a user based in Germany)
* Acceptable Ads (maintained by eyeo)
* ABP Filters (maintained by eyeo)


## Bundled filter lists

eyeo Chromium SDK aims to provide some level of ad-filtering on startup, but the following factors (among others) could delay or prevent it:

* Network connectivity isn't available
* Filter list downloads are restricted to certain networks
* Conversion from the downloaded text files to FlatBuffers takes more time than loading the first page

A collection of filter lists (known as "bundled filter lists") is always distributed with the browser. Bundled filter lists are the latest available at the time of release, and will be replaced by up-to-date ones downloaded from the Internet as soon as possible. Downloads will be retried in case of error.

You can update the bundled filter lists at any time using the `components/resources/adblocking/update.sh` script.


## Full versus minified filter lists

Minified filter lists contain a subset of the entries from a (full) filter list, in order to optimize for very resource-constrained environments by reducing disk and memory usage.

Bundled filter lists are minified, to reduce the Resource Bundle size while providing an acceptable browsing experience. Downloaded subscriptions are full ones.

The [decision record](../adr/consuming-full-filter-lists.md) explains the decision to use full filter lists.


## Implementation details

`SubscriptionService` holds a collection of the active Subscriptions installed, which is used to create a `SubscriptionCollection`. A `SubscriptionCollection` can be asked whether to block or allow a network request, or what element hiding content should be injected to a page. It will check in all the lists individually and make a collective decision.

All network requests are paused while `SubscriptionService` is initializing.

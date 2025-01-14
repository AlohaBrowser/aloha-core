# Ad-filtering

Eyeo Chromium SDK supports several ad-filtering techniques that apply in different stages of loading a web page:

* [Resource filtering](resource-filtering.md): Blocking network resources from downloading
* [Element hiding](element-hiding.md): Hiding content that couldn't be stopped by blocking requests
* [Snippets](snippets.md): Specialized code snippets to deal with most complex scenarios (like video ads)

In order to decide which requests should be blocked and which elements hidden, eyeo Chromium SDK consults
[Adblock Plus filters](https://help.eyeo.com/adblockplus/how-to-write-filters), distributed within
[filter lists](filter-lists.md).

# Frame hierarchy

Deciding whether to block a network request is much more complicated than just matching an URL against a list of
filters. The SDK considers the filters that forbid blocking of specific categories of content at the website
level. This means that a top-level page or one loaded in a parent iframe may have associated filters. The
implementation operates with the URL of the current frame, as well as the URL of all parent frames.

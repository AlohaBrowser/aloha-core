# Request filtering

The following two entry points in `AdblockURLLoaderFactory` are important for the resource filtering:

* At the very beginning of network request lifetime (`AdblockURLLoaderFactory::InProgressRequest::InProgressRequest`). `AdblockURLLoaderFactory` may pause the request, and then cancel or resume it, depending on classification result. Or `AdblockURLLoaderFactory` may also rewrite url response if there is a rewrite filter which matches the resource request.
* When receiving headers of HTTP responses (`AdblockURLLoaderFactory::InProgressRequest::OnReceiveResponse`) in order to apply CSP filters and header filters, and to extract a Sitekey.

When classifying requests:

* Request URL
* Request type (content/script/image, etc.)
* ID of the render frame that requested the resource

When classifying response:

* Response URL
* Response headers
* ID of the render frame that requested the resource

Within the browser process, `ResourceClassificationRunner` receives the data, makes a blocking decision (`ResourceClassificationRunner::CheckRequestFilterMatch`/ `ResourceClassificationRunner::CheckRewriteFilterMatch` / `ResourceClassificationRunner::CheckResponseFilterMatch`) and sends the decision back.

When validating and storing SiteKey, these are the forwarded pieces of data:

* Response URL
* User-Agent header of the request
* `X-Adblock-Key` response header

In this scenario, `SitekeyStorage` is the class that receives the data.

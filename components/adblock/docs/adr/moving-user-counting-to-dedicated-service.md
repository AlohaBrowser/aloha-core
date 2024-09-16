# Moving user counting to a dedicated service

Having the user counting service coupled with the filter lists service prevents partners from serving and monetizing filter lists from their own servers, while still allowing us to understand how the SDK usage is distributed across Chromium, OS and platform versions, and to monitor the Acceptable Ads opt-out rate.

By moving to a dedicated service, filter lists can be downloaded from any source, as the structure of the user counting "pings" and the URL they are directed to are completely independent from those for filter lists.

# Transitioning from minified to full filter lists

The main reason for using minified filter lists in previous implementations of our ad-filtering core was to reduce memory consumption for mobile users. On the other hand, more recent implementations achieved this reduction by:

* Adopting FlatBuffers as file format for filter lists
* Moving away from depending on V8 to a native implementation

It is now possible to use full filter lists, which include more filter rules. This improves user experience as more intrusive ads will be blocked. This also increases revenue for publishers, who are part of the Acceptable Ads program, as more acceptable ads will be allowed.


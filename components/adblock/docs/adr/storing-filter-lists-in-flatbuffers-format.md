# Storing filter lists in FlatBuffers format

Filter lists aren't consumed in plain text format right after download. Instead, they are converted from plain text into FlatBuffers format due to the following advantages:

* It greatly reduces memory consumption, down to approximately 15 MB
* It greatly reduces startup time, from the order of seconds to milliseconds
* Page load time isn't negatively affected except in certain sites with very long URLs
* It provides facilities for multi-threading and synchronous pop-up blocking (which is a Chromium restriction)
* It doesn't require deserialization, as it's a binary format that's ready to use.
* It can be memory-mapped and accessed as memory directly from disk. Being allocated in a contiguous memory buffer also makes it a cache-friendly format.
* Accessing data in a flatbuffer is as fast as dereferencing pointers to memory, there's very little additional "unwrapping" overhead.

A FlatBuffers file contains only one filter list, instead of combining all selected ones into one file, for the following reasons:

* Filter lists updated at different times can be downloaded independently
* Less time consumed in conversion than if all selected filter lists had to be combined in one file
* For potential long-term distribution of FlatBuffers files, having to provide a file containing all selected filter lists would cause an explosion of combinations

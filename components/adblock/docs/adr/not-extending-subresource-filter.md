# Implementing ad-filtering from scratch rather than extending Chromium's Subresource Filter

Chromium already contains some ad-blocking functionality, named Subresource Filter, but it is limited, for instance:

* Doesn't support element hiding via CSS
* Doesn't support element hiding emulation via JavaScript
* Doesn't support snippets

These limitations could be patched, but that comes with significant trade-offs:

* Bigger cost of Chromium updates, with more potential conflicts to solve
* Less flexibility to introduce optimizations and new features
* Potentially irreconcilable differences between what Chrome authors aim for and what we aim for. Chromium authors may have different ad-related business incentives, and evolve Subresource Filtering in an undesired direction

# parse
neic-glass3 parsing library.

The neic-glass3 parsing library parses CCData, GPick, and [earthquake-detection-formats](https://github.com/usgs/earthquake-detection-formats/tree/master/format-docs)
messages into messages understandable by glasscore

Additionally the library converts [Event](https://github.com/usg/neic-glass3/blob/code-review/doc/internal-formats/Event.md),
[Cancel](https://github.com/usg/neic-glass3/blob/code-review/doc/internal-formats/Cancel.md),
[SiteList](https://github.com/usg/neic-glass3/blob/code-review/doc/internal-formats/SiteList.md),
and [SiteLookup](https://github.com/usg/neic-glass3/blob/code-review/doc/internal-formats/SiteLookup.md)
messages into earthquake-detection-formats messages.

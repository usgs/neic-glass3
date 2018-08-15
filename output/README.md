# output
neic-glass3 output formatting and publication library.

The neic-glass3 output library is a library that performs the publication tracking
and translation tasks for neic-glass3.

The Output library utilizes the internal glasscore [Event](https://github.com/usg/neic-glass3/blob/code-review/doc/internal-formats/Event.md),
[Cancel](https://github.com/usg/neic-glass3/blob/code-review/doc/internal-formats/Cancel.md),
and [Expire](https://github.com/usg/neic-glass3/blob/code-review/doc/internal-formats/Expire.md)
for publication tracking. The library uses a configurable set of fixed publication
times to determine when to publish. These messages are passed to the Output
library via the sendToOutput function from the iOutput interface

The output library generates the internal formats [ReqHypo](https://github.com/usg/neic-glass3/blob/code-review/doc/internal-formats/ReqHypo.md)
and [ReqSiteList](https://github.com/usg/neic-glass3/blob/code-review/doc/internal-formats/ReqSiteList.md)
messagesto request detailed event detection and site list information from glasscore
via the m_Associator pointer.

The output library translates the internal glasscore [Hypo](https://github.com/usg/neic-glass3/blob/code-review/doc/internal-formats/Hypo.md),
[SiteList](https://github.com/usg/neic-glass3/blob/code-review/doc/internal-formats/SiteList.md)
and [SiteLookup](https://github.com/usg/neic-glass3/blob/code-review/doc/internal-formats/SiteLookup.md)
into detection format json messages as defined in
[earthquake-detection-formats](https://github.com/usgs/earthquake-detection-formats/tree/master/format-docs)

The Output library is intended to be inherited from to define application specific
output mechanisms (i.e. output to disk files).

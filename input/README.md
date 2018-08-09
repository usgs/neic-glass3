# input
neic-glass3 input parsing and queuing library.

The neic-glass3 Input library is a thread library encapsulating the data Input logic
for neic-glass3. The Input library handles reading Input data, parsing it,
validating it, and queuing it for later use by glasscore via the Associator
library. If the internal queue is full, the library will pause reading Input data
until space is available.

The Input library generates detection format json messages as defined in
[earthquake-detection-formats](https://github.com/usgs/earthquake-detection-formats/tree/master/format-docs)
that are then passed to the associator/glasscore via a queue.

The Input library is intended to be inherited from to define application specific
input mechanisms (i.e. input from disk files).

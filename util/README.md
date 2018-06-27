# util
A set of utility functions, interfaces, and base classes used by neic-glass3.

## Interfaces
* **iInput** - an interface implemented by classes that wish to input data to
neic-glass3
* **iAssociator** - an interface implemented by a class that acts as a wrapper
around glass core, and manages communication between glasscore and the rest of
neic-glass3
* **iOutput** - an interface implemented by a class that processes output data from
neic-glass3

## Classes
* **BaseClass** - The base class for all neic-glass3 classes (except glasscore),
encapsulates the most basic setup and configuration logic.
* **ThreadBaseClass** - The base class for classes that require background threads
* **Cache** - a class that implements a cache of json objects.
* **Queue** - a class that implements a queue of json objects (SuperEasyJSON)
* **ThreadPool** - a class that provides a job queue and manages a pool of threads to
process those jobs.

## Functions
* **fileutil** - a set of functions for finding, moving, copying, and deleting files.
* **stringutil** - a set of functions for converting, splitting, and checking strings.
* **timeutl** - a set of functions used for converting between various time string
formats and epoch time,

For more information, please see the [util Doxygen documentation](https://usgs.github.io/neic-glass3/util/html/namespaceutil.html)

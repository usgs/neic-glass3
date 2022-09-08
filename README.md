> **THIS PROJECT HAS BEEN ARCHIVED**
> 
> Please see [https://code.usgs.gov/ghsc/neic/algorithms/neic-glass3)

[![codecov](https://codecov.io/gh/usgs/neic-glass3/branch/master/graph/badge.svg)](https://codecov.io/gh/usgs/neic-glass3)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/5584cd7bb81f412c9d6dda9ebd787d13)](https://www.codacy.com/app/jpatton-USGS/neic-glass3?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=usgs/neic-glass3&amp;utm_campaign=Badge_Grade)
[![Documentation](https://usgs.github.io/neic-glass3/codedocumented.svg)](https://usgs.github.io/neic-glass3/)

neic-glass3
------

Next generation seismic event detection and association algorithm
developed by the United States Geological Survey National Earthquake Information
Center and Caryl Erin Johnson, PhD, Introspective Systems LLC.

This algorithm converts a time series of seismic waveform phase arrival times,
back azimuth estimates from array beams, and cross-correlated detections into a
catalog of earthquake detections.

The algorithm nucleates detections via a Bayesian stacking algorithm using an
in-memory graph database of detection nodes. Once a detection is made, the
algorithm associates other available arrival times, back azimuths, etc. using
affinity statistics and waveform phase travel-time table lookups.

* [License](LICENSE.md)
* [Disclaimer](DISCLAIMER.md)
* [Contributing](CONTRIBUTING.md)

Dependencies
------
* Glass utilizes [JSON](www.json.org) for formatting.
* Glass uses a [CMake](http://www.cmake.org/) build script
([CMakeLists.txt](CMakeLists.txt)) for cross platform compilation.
A copy of CMake is not included in this project
* Glass utilizes [rapidjson](https://github.com/miloyip/rapidjson)
to format, parse, and write JSON.  A copy of rapidjson is included in
this project.
* The glass core library utilizes [SuperEasyJSON](https://sourceforge.net/projects/supereasyjson/)
to format, parse, and write JSON.  A copy of SuperEasyJSON is included in
this project.
* Glass uses uuid on linux for unique identifiers, this package is  **not**
included in this project, and may need to be installed via the
`sudo yum install libuuid libuuid-devel` command.
* Glass uses the [earthquake-detection-formats](https://github.com/usgs/earthquake-detection-formats)
library for input and output formatting. A copy of earthquake-detection-formats is
downloaded as part of the build.
* Glass optionally uses the [hazdev-broker](https://github.com/usgs/hazdev-broker)
library for input and output processing. A copy of hazdev-broker is
downloaded as part of the build.
* Glass optionally uses the [librdkafka](https://github.com/edenhill/librdkafka)
library for input and output processing. A copy of librdkafka is included in
this project.
* Glass optionally uses [doxygen](http://www.stack.nl/~dimitri/doxygen/) for
documentation generation.  A copy of doxygen is optionally downloaded as part of
the build.
* Glass optionally uses [cpplint](https://github.com/google/styleguide/tree/gh-pages/cpplint)
to check coding style. A copy of cpplint is included in this project.
* Glass optionally uses [cppcheck](http://cppcheck.sourceforge.net/)
for static code analysis. A copy of cppcheck is **not** included in this project.
* Glass optionally uses [googletest](https://github.com/google/googletest) for
unit testing.  A copy of googletest is optionally downloaded as part of the
build.
* Glass optionally uses lcov/gcov for code coverage analysis. A copy of lcov/gcov
is **not** included in this project.

Building
------
The steps to get and build neic-glass3 using CMake are as follows:

1. Clone neic-glass3.
2. Open a command window and change directories to Glass
3. Make a build directory `mkdir build`
4. Make a distribution directory `mkdir dist`
5. Change to the build directory `cd build`
6. Run the the appropriate CMake command:<br>
a. `cmake .. -DCMAKE_INSTALL_PREFIX=../dist -DRAPIDJSON_PATH=../lib/rapidjson -DBUILD_GLASS-APP=0`
to build just the glass core libraries <br>
b. `cmake .. -DCMAKE_INSTALL_PREFIX=../dist -DRAPIDJSON_PATH=../lib/rapidjson`
to build the glass core libraries and glass-app application. <br>
c. `cmake .. -DCMAKE_INSTALL_PREFIX=../dist -DRAPIDJSON_PATH=../lib/rapidjson -DBUILD_GEN-TRAVELTMES-APP=1`
to build the glass core libraries, glass-app, and gen-traveltimes-app applications. <br>
d. `cmake .. -DCMAKE_INSTALL_PREFIX=../dist -DRAPIDJSON_PATH=../lib/rapidjson -DBUILD_GLASS-BROKER-APP=1 -DLIBRDKAFKA_C_LIB=/usr/local/lib/librdkafka.a -DLIBRDKAFKA_CPP_LIB=/usr/local/lib/librdkafka++.a -DLIBRDKAFKA_PATH=/usr/local/include/librdkafka`
to build the glass core libraries, glass-app, and glass-broker-app applications. <br>**NOTE:** Requires that librdkafa be built and installed.
7. If you are on a \*nix system, you should now see a Makefile in the current
directory.  Just type 'make' to build the glass libraries and desired applcations.  
8. If you are on Windows and have Visual Studio installed, a `Glass.sln` file
and several `.vcproj` files will be created.  You can then build them using
Visual Studio.  
9. Note that for \*nix you must generate seperate build directories for x86 vs
x64 compilation specifying the appropriate generator `cmake -G <generator> ..`.

Further Information and Documentation
------
For further information and documentation please check out the [neic-glass3 Wiki](https://github.com/usgs/neic-glass3/wiki), [neic-glass Documentation Site](https://usgs.github.io/neic-glass3/), and the [Examples Directory](https://github.com/usgs/neic-glass3/tree/master/examples).

File bug reports, feature requests and questions using
[GitHub Issues](https://github.com/usgs/neic-glass3/issues)

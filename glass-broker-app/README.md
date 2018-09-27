# glass-broker-app

**glass-broker-app** is an application that uses the glasscore libraries and the rest
of the glass3 infrastructure to generate seismic event detections based on
an input dataset of picks, correlations, and detections.

glass-broker-app reads it's input data from various kafka input topics
accessed via the hazdevbroker library.

glass-broker-app writes it's output detections to various kafka output topics
accessed via the hazdevbroker library.

glass-broker-app uses the environment variable `GLASS_LOG` to define the
location to write log files

## Building

To build **glass-broker-app**, set the `BUILD_GLASS-BROKER-APP` option equal
to true (1) in the CMake command or GUI.

The `LIBRDKAFKA_C_LIB`, `LIBRDKAFKA_CPP_LIB`, and `LIBRDKAFKA_PATH` option
variables will also need to be set in the CMake command or GUI.

## Configuration

An example configuration for **glass-broker-app** is available in the [glass-broker-app params directory](https://github.com/usgs/neic-glass3/tree/master/glass-broker-app/params)

## Running

To run **glass-broker-app**, use the following command: `glass-broker-app <configfile> [logname] [noconsole]` where `<configfile>` is the required path the the glass.d configuration file, `[logname]` is an optional command that when present specifies the log file name and enables logging, and `[noconsole]` is an optional string specifying that glass-broker-app should not write messages to the console.

For more information, please see the [glass-broker-app DOxygen documentation](https://usgs.github.io/neic-glass3/glass-broker-app/html/glass-broker-app_8cpp.html)

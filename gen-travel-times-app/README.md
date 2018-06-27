# gen-traveltimes-app

**Gen-travel-times-app** is an application that uses the glasscore traveltime
libraries to generate the traveltime lookup files (.trv) used by neic-glass3 from a
model file.  Please note that this application is currently not optimized, and
is **extremely** slow.

## Building

To build **gen-travel-times-app**, set the `BUILD_GEN-TRAVELTMES-APP` option equal
to true (1) in the CMake command or GUI.

## Configuration

An example configuration for **gen-travel-times-app** is available in the [gen-travel-times-app params directory](https://github.com/usgs/neic-glass3/tree/master/gen-travel-times-app/params)

## Running

To run **gen-travel-times-app**, use the following command: `gen-travel-times-app <configfile> [logname]` where `<configfile>` is the required path the the gen-travel-times.d configuration file and `[logname]` is an optional command that when present specifies the log file name and enables logging.

For more information, please see the [gen-travel-times-app DOxygen documentation](https://usgs.github.io/neic-glass3/gen-travel-times-app/html/gen-travel-times-app_8cpp.html)

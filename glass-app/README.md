# glass-app

**glass-app** is an application that uses the glasscore libraries and the rest
of the glass3 infrastructure to generate seismic event detections based on
an input dataset of picks, correlations, and detections.

glass-app reads it's input data from files stored in a configured filesystem
directory

glass-app writes it's output detections to files in a configured filesystem
directory

glass-app uses the environment variable `GLASS_LOG` to define the
location to write log files

## Building

To build **glass-app**, set the `BUILD_GLASS-APP` option equal
to true (1) in the CMake command or GUI.

## Configuration

An example configuration for **glass-app** is available in the [glass-app params directory](https://github.com/usgs/neic-glass3/tree/master/glass-app/params)

## Running

To run **glass-app**, use the following command: `glass-app <configfile> [logname] [noconsole]` where `<configfile>` is the required path the the glass.d configuration file, `[logname]` is an optional command that when present specifies the log file name and enables logging, and `[noconsole]` is an optional string specifying that glass-app should not write messages to the console.

For more information, please see the [glass-app DOxygen documentation](https://usgs.github.io/neic-glass3/html/glass-app_8cpp.html)

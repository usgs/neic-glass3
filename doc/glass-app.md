# glass-app

glass-app is an implementation of the neic-glass3 libraries that reads input from a file system directory, and writes output to a file system directory, with a static stationlist and configuration.

## Building

To build glass-app, set the `BUILD_GLASS-APP` option equal to true (1) in the cmake command or GUI.

## Configuration

An example configuration for glass-app is available in the [glass-app params directory](https://github.com/usgs/neic-glass3/tree/master/glass-app/params)

### Glass-App

```json
{
    "Configuration":"glass-app",
    "LogLevel":"debug",
    "ConfigDirectory":"./params",
    "InitializeFile":"initialize.d",
    "StationList":"stationlist.d",
    "GridFiles":[
        "ak_grid.d",
    ],
    "InputConfig":"input.d",
    "OutputConfig":"output.d"
}
```

* **LogLevel** - Sets the minimum logging level, trace, debug, info, warning, error, or criticalerror
* **ConfigDirectory** - The path to directory containing the other glass subcomponent configuration files
* **InitializeFile** - Configuration file containing the neic-glass3 Algorithm configuration
* **StationList** - File containing the initial neic-glass3 station list
* **GridFiles** - One or more files defining detection grids used by neic-glass3
* **InputConfig** - Configuration file containing the file input configuration
* **OutputConfig** - Configuration file containing the file output configuration

### input.d

```json
{
    "Configuration":"GlassInput",
    "InputDirectory":"./input",
    "ErrorDirectory":"./error",
    "ArchiveDirectory":"./archive",
    "Format":"gpick",
    "QueueMaxSize":1000,
    "ShutdownWhenNoData":true,
    "ShutdownWait":300,
    "DefaultAgencyID":"US",
    "DefaultAuthor":"glassConverter"
}
```

* **InputDirectory** - The directory to read input files from
* **ErrorDirectory** - The optional directory to archive erroneous input files to.
* **ArchiveDirectory** - The optional directory to archive input files to.
* **Format** - The format to accept. glass-app currently understands the gpick, jsonpick, jsonhypo, and ccdata (dat) formats.  Note that the only way to use multiple inputs (Picks, Correlations, and Detections at the same time)
* **QueueMaxSize** - The maximum size of the input queue
* **ShutdownWhenNoData** - Optional Flag indicating whether to shut down when there is no more input data
* **ShutdownWait** - The time in seconds to wait before shutting down due to there being no input data
* **DefaultAgencyID** - The default agency identifier to use when converting data to json
* **DefaultAuthor** - The default author to use when converting data to json

### Output

```json
{
    "Configuration":"GlassOutput",
    "PublicationTimes":[20,180],
    "PublishOnExpiration":true,
    "OutputDirectory":"./output",
    "OutputFormat":"json",
    "TimeStampFileName":true,
    "OutputAgencyID":"US",
    "OutputAuthor":"glass"
}
```

* **PublicationTimes** - The time(s), in seconds since the detections was first found, to publish 
* **PublishOnExpiration** - Flag indicating whether to always publish a final version of a detection when it expires out of glass
* **OutputDirectory** - The directory to write output to
* **OutputFormat** - The format to write output in for now, the only format is json
* **TimeStampFileName** - Optional flag to define whether to timestamp output file names, defaults to true
* **OutputAgencyID** - The agency identifier to use when generating output data
* **OutputAuthor** - The author to use when generating output data

### neic-glass3 Algorithm

For neic-glass3 algorithmic configuration, see [neic-glass3 Configuration](https://github.com/usgs/neic-glass3/blob/master/doc/GlassConfiguration.md).

## Documentation

Further documentation of the glass-app software is available [here](https://usgs.github.io/neic-glass3/glass-app/html/)

## Running

To run glass-app, use the following command: `glass-app <configfile> [logname] [noconsole]` where `<configfile>` is the required path the the glass.d configuration file, `[logname]` is an optional command that defining an alternate name for the glass-app log file, and `[noconsole]` is an optional command specifying that glass-app should not write messages to the console.

glass-app uses the environment variable GLASS_LOG to define the location to write log files
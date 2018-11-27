# gen-travel-times-app

gen-travel-times-app is an application that uses the glasscore traveltime libraries to generate the traveltime lookup files (.trv) used by neic-glass3 from a model file.  Please note that this application is currently not optimized, and is **extremely** slow.

## Building

To build gen-travel-times-app, set the `BUILD_GEN-TRAVELTMES-APP` option equal to true (1) in the cmake command or GUI.

## Configuration

An example configuration for gen-travel-times-app is available in the [gen-travel-times-app params directory](https://github.com/usgs/neic-glass3/tree/master/gen-travel-times-app/params)

### gen-travel-times.d

```json
{
    "Configuration": "gen-travel-times-app",
    "FileExtension": ".trv",
    "OutputPath": "./",
    "Model": "./params/ak135_mod.d",
    "Branches": [
        {
            "Cmd": "GenerateTraveltime",
            "Branch": "P",
            "Rays": [
                "Pup",
                "P",
                "Pdiff"
            ],
            "DeltaTimeWarp": {
                "MinimumDistance": 0.0,
                "MaximumDistance": 360.0,
                "SlopeDecayConstant": 0.10,
                "SlopeZero": 0.05,
                "SlopeInfinite": 1.0
            },
            "DepthTimeWarp": {
                "MinimumDepth": -10.0,
                "MaximumDepth": 800.0,
                "SlopeDecayConstant": 0.10,
                "SlopeZero": 1.0,
                "SlopeInfinite": 10.0
            }
        }
    ]
}
```

* **FileExtension** - The file extension to use for travel time files
* **OutputPath** - The output directory to write travel time files to
* **Model** - The earth model file to use
* **Branches** - The list of travel time files to generate
* **Branch** - The branch name for the travel time file
* **Rays** - The rays (phases) to use in generating the file
* **DeltaTimeWarp** - The distance warp for this travel time file
* **MinimumDistance** - Start of warp in degrees
* **MaximumDistance** - End of warp in degrees
* **SlopeDecayConstant** - The decay exponent for the warp
* **SlopeZero** - The warp slope value at minimum
* **SlopeInfinite** - The warp slope value at maximum
* **DepthTimeWarp** - The depth warp for this travel time file
* **MinimumDepth** - Start of warp in kilometers
* **MaximumDepth** - End of warp in kilometers
* **SlopeDecayConstant** - The decay exponent for the warp
* **SlopeZero** - The warp slope value at minimum
* **SlopeInfinite** - The warp slope value at maximum

## Documentation

Further documentation of the gen-traveltimes-app software is available [here](https://usgs.github.io/neic-glass3/gen-travel-times-app/html/)

## Running

To run gen-travel-times-app, use the following command: `gen-travel-times-app <configfile> [logname]` where `<configfile>` is the required path the the gen-travel-times.d configuration fileand `[logname]` is an optional command that when present specifies the log file name and enables logging.

gen-travel-times-app uses the environment variable GLASS_LOG to define the location to write log files
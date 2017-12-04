# gen-travel-times.d
# Configuration file for the glass parameters
{
  "Configuration": "gen-travel-times-app",
  "FileExtension": ".trv",
  "OutputPath": "./",
  "Model": "./params/ak135_mod.d",
  "Branches":
  [
    {
      "Cmd": "GenerateTraveltime",
      "Branch": "P",
      "Rays": ["Pup", "P", "Pdiff"],
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
# End of gen-travel-times.d

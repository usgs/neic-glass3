# gen-travel-times.d
# Configuration file for the glass parameters
{
  "Configuration": "gen-travel-times-app",
  
  # The file extension to use for travel time files
  "FileExtension": ".trv",
  
  # The output directory to write travel time files to
  "OutputPath": "./",
  
  # The earth model to use
  "Model": "./params/ak135_mod.d",
  
  # The list of travel time files to generate
  "Branches":
  [
    {
      "Cmd": "GenerateTraveltime",
      
      # Branch name
      "Branch": "P",
      
      # The rays (phases) to use in generating the file
      "Rays": ["Pup", "P", "Pdiff"],
      # The distance warp for this file
      "DeltaTimeWarp": {
        "MinimumDistance": 0.0, # Start of warp
        "MaximumDistance": 360.0, # End of warp
        "SlopeDecayConstant": 0.10, # the decay exponent
        "SlopeZero": 0.05, # the slope value at minimum
        "SlopeInfinite": 1.0 # the slope value at maximum
      },
      # The Depth warp for this file
      "DepthTimeWarp": {
        "MinimumDepth": -10.0, # Start of warp
        "MaximumDepth": 800.0, # End of warp
        "SlopeDecayConstant": 0.10, # the decay exponent
        "SlopeZero": 1.0, # the slope value at minimum
        "SlopeInfinite": 10.0 # the slope value at maximum
      }
    }
  ]
}
# End of gen-travel-times.d

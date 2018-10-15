# initialize.d
# Configuration file for the glass parameters
{
  "Cmd": "Initialize",
  "MaximumNumberOfPicks": 10000,
  "MaximumNumberOfPicksPerSite": 40,
  "MaximumNumberOfCorrelations": 1000,
  "MaximumNumberOfHypos": 500,
  "PickDuplicateWindow": 2.5,
  "NumberOfNucleationThreads": 15,
  "NumberOfHypoThreads": 15,
  "NumberOfWebThreads": 0,  # 0 because glass-app does not do site lookups.
  "SiteHoursWithoutPicking": -1,  # Disabled because glass-app does not do site lookups.
  "SiteLookupInterval": -1,  # Disabled because glass-app does not do site lookups.
  "SiteMaximumPicksPerHour": -1,  # Disabled because glass-app does not do site lookups.
  "Params": {
      "NucleationStackThreshold": 5.0, # 0.5,
      "NucleationDataCountThreshold": 10,
      "AssociationStandardDeviationCutoff": 3.0,
      "PruningStandardDeviationCutoff": 3.0,
      "PickAffinityExponentialFactor": 2.5,
      "DistanceCutoffFactor": 5.0,
      "DistanceCutoffRatio": 0.8,
      "DistanceCutoffMinimum": 30.0,
      "HypoProcessCountLimit": 25,
      "CorrelationTimeWindow": 2.5,
      "CorrelationDistanceWindow": 0.5,
      "CorrelationCancelAge": 900,
      "BeamMatchingAzimuthWindow": 22.5,
      "ReportingStackThreshold": 0.5,
      "ReportingDataThreshold":5,
      "EventFragmentDepthThreshold": 550.0,
      "EventFragmentAzimuthThreshold": 270.0,
      "HypocenterTimeWindow": 30,
      "HypocenterDistanceWindow": 3
  },
  "DefaultNucleationPhase": {
      "PhaseName": "P",
      "TravFile": "./global_example/P.trv"
  },
  "AssociationPhases": [
      {
          "PhaseName": "P",
          "Range": [ 0, 0, 120, 180 ],
          "TravFile": "./global_example/P.trv"
      },
      {
          "PhaseName": "S",
          "Assoc": [ 5, 90 ],
          "TravFile": "./global_example/S.trv"
      },
      {
          "PhaseName": "PcP",
          "Range": [ 0 0, 45, 50 ],
          "TravFile": "./global_example/PcP.trv"
      },
      {
          "PhaseName": "PP",
          "Range": [ 90, 95, 175, 180 ],
          "TravFile": "./global_example/PP.trv"
      },
      {
          "PhaseName": "PKPab",
          "Range": [ 130, 132, 178, 180 ],
          "TravFile": "./global_example/PKPab.trv"
      },
      {
          "PhaseName": "PKPdf",
          "Range": [ 100, 114, 176, 180 ],
          "TravFile": "./global_example/PKPdf.trv"
      }
  ]
}
# End of initialize.d

# glassinitialize.d
# Configuration file for the glass parameters
{
  "Cmd": "Initialize",
  "MaximumNumberOfPicks": 10000,
  "MaximumNumberOfPicksPerSite": 30,
  "MaximumNumberOfCorrelations": 1000,
  "MaximumNumberOfHypos": 500,
  "PickDuplicateWindow": 2.5,
  "NumberOfNucleationThreads": 3,
  "NumberOfHypoThreads":3,
  "NumberOfWebThreads": 1,
  "SiteHoursWithoutPicking": -1,
  "SiteLookupInterval": -1,
  "SiteMaximumPicksPerHour": -1,
  "Params": {
      "NucleationStackThreshold": 0.5,
      "NucleationDataThreshold": 10,
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
      "ReportingDataThreshold":5
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
          "Assoc": [ 20, 60 ],
          "TravFile": "./global_example/PcP.trv"
      },
      {
          "PhaseName": "PP",
          "Assoc": [ 90, 180 ],
          "TravFile": "./global_example/PP.trv"
      },
      {
          "PhaseName": "PKPab",
          "Assoc": [ 130, 180 ],
          "TravFile": "./global_example/PKPab.trv"
      },
      {
          "PhaseName": "PKPdf",
          "Assoc": [ 100, 180 ],
          "TravFile": "./global_example/PKPdf.trv"
      }
  ]
}
# End of glassinitialize.d

# initialize.d
# Configuration file for the glass parameters
{
  "Cmd": "Initialize",
  "MaximumNumberOfPicks": 500,
  "MaximumNumberOfPicksPerSite": 20,
  "MaximumNumberOfCorrelations": 10,
  "MaximumNumberOfHypos": 100,
  "PickDuplicateWindow": .1,
  "NumberOfNucleationThreads": 3,
  "NumberOfHypoThreads": 3,
  "NumberOfWebThreads": 1,
  "SiteHoursWithoutPicking": -1,
  "SiteLookupInterval": -1,
  "SiteMaximumPicksPerHour": -1,
  "Params": {
      "NucleationStackThreshold": 6.5,
      "NucleationDataThreshold": 10,
      "AssociationStandardDeviationCutoff": 3.0,
      "PruningStandardDeviationCutoff": 3.0,
      "PickAffinityExponentialFactor": 1.0,
      "DistanceCutoffFactor": 5.0,
      "DistanceCutoffPercentage": 0.8,
      "DistanceCutoffMinimum": 30.0,
      "HypoProcessCountLimit": 25,
      "CorrelationTimeWindow": 2.5,
      "CorrelationDistanceWindow": 0.5,
      "CorrelationCancelAge": 900,
      "BeamMatchingAzimuthWindow": 22.5,
      "ReportingStackThreshold": 2.0,
      "ReportingDataThreshold":2
  },
  "DefaultNucleationPhase": {
      "PhaseName": "P",
      "TravFile": "./local_example/P.trv"
  },
  "AssociationPhases": [
      {
          "PhaseName": "P",
          "Range": [ 0, 0, 1.3, 1.3 ],
          "TravFile": "./local_example/P.trv"
      },
      {
          "PhaseName": "S",
          "Range": [ 0, 0, 1.3, 1.3 ],
          "TravFile": "./local_example/S.trv"
      }
  ]
}
# End of initialize.d

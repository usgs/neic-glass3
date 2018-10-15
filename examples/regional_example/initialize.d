# glassinitialize.d
# Configuration file for the glass parameters
{
  "Cmd": "Initialize",
  "MaximumNumberOfHypos": 300,
  "MaximumNumberOfPicks": 2500,
  "MaximumNumberOfPicksPerSite": 30,
  "PickDuplicateWindow": 10.0,
  "NumberOfNucleationThreads": 5,
  "NumberOfHypoThreads": 5,
  "Params": {
      "NucleationStackThreshold": 6.5,
      "NucleationDataCountThreshold": 10,
      "AssociationStandardDeviationCutoff": 5.0,
      "PruningStandardDeviationCutoff": 3.0,
      "DistanceCutoffFactor": 2.0,
      "DistanceCutoffRatio": 0.8,
      "DistanceCutoffMinimum": 2.0,
      "HypoProcessCountLimit": 25,
      "ReportingDataThreshold":10
  },
  "DefaultNucleationPhase": {
      "PhaseName": "P",
      "TravFile": "./regional_example/P.trv"
  },
  "AssociationPhases": [
      {
          "PhaseName": "P",
          "Range": [ 0., 0., 120., 180. ],
          "TravFile": "./regional_example/P.trv"
      },
      {
          "PhaseName": "S",
          "Range": [ 0., 0., 120., 180. ],
          "TravFile": "./regional_example/S.trv"
      }
  ]
}
# End of glassinitialize.d

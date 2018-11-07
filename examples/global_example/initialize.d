# initialize.d
# Configuration file for the glass parameters
{
  "Cmd": "Initialize",
  "MaximumNumberOfPicks": 10000,
  "MaximumNumberOfPicksPerSite": 40,
  "MaximumNumberOfCorrelations": 1000,
  "MaximumNumberOfHypos": 500,
  "PickDuplicateWindow": 10,
  "NumberOfNucleationThreads": 15,
  "NumberOfHypoThreads": 15,
  "Params": {
      "NucleationStackThreshold": 5.0, 
      "NucleationDataCountThreshold": 10,
      "AssociationStandardDeviationCutoff": 5.0,
      "PruningStandardDeviationCutoff": 5.0,
      "PickAffinityExponentialFactor": 2.5,
      "DistanceCutoffFactor": 5.0,
      "DistanceCutoffRatio": 0.8,
      "DistanceCutoffMinimum": 30.0,
      "HypoProcessCountLimit": 15,
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

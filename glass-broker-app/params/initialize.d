# initialize.d
# Configuration file for the glass parameters
{
  "Cmd": "Initialize",
  "MaximumNumberOfPicks": 10000,
  "MaximumNumberOfPicksPerSite": 30,
  "MaximumNumberOfCorrelations": 1000,
  "MaximumNumberOfHypos": 250,
  "PickDuplicateWindow": 2.5,
  "NumberOfNucleationThreads": 5,
  "NumberOfHypoThreads": 3,
  "NumberOfWebThreads": 1,
  "SiteHoursWithoutPicking": 6,
  "SiteLookupInterval": 6,
  "SiteMaximumPicksPerHour": 200,
  "Params": {
      "NucleationStackThreshold": 0.5,
      "NucleationDataCountThreshold": 10,
      "AssociationStandardDeviationCutoff": 5.0,
      "PruningStandardDeviationCutoff": 5.0,
      "PickAffinityExponentialFactor": 2.5,
      "DistanceCutoffFactor": 5.0,
      "DistanceCutoffRatio": 0.8,
      "DistanceCutoffMinimum": 30.0,
      "HypoProcessCountLimit": 25,
      "CorrelationTimeWindow": 2.5,
      "CorrelationDistanceWindow": 0.5,
      "CorrelationCancelAge": 900,
      "BeamMatchingAzimuthWindow" : 22.5,
      "ReportingStackThreshold": 0.5,
      "ReportingDataThreshold":5,
      "EventFragmentDepthThreshold": 550.0,
      "EventFragmentAzimuthThreshold": 270.0,
      "HypocenterTimeWindow": 30,
      "HypocenterDistanceWindow": 3
  },
  "DefaultNucleationPhase": {
      "PhaseName": "P",
      "TravFile": "./P.trv"
  },
  "AssociationPhases": [
      {
          "PhaseName": "P",
          "Range": [ 0, 0, 120, 180 ],
          "TravFile": "./P.trv"
      },
      {
          "PhaseName": "S",
          "Assoc": [ 10, 90 ],
          "TravFile": "./S.trv"
      },
      {
          "PhaseName": "PcP",
          "Range": [ 0, 0, 45 , 50],
          "TravFile": "./PcP.trv"
      },
      {
          "PhaseName": "PP",
          "Range": [ 90, 95, 175, 180 ],
          "TravFile": "./PP.trv"
      },
      {
          "PhaseName": "PKPab",
          "Range": [ 130, 132, 178, 180 ],
          "TravFile": "./PKPab.trv"
      },
      {
          "PhaseName": "PKPdf",
          "Range": [ 100, 114, 176, 180 ],
          "TravFile": "./PKPdf.trv"
      }
  ]
}
# End of initialize.d

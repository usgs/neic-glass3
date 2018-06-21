# initialize.d
# Configuration file for the glass parameters
{
  "Cmd": "Initialize",
  "HypoMax": 100,
  "PickMax": 500,
  "SitePickMax": 20,
  "CorrelationMax": 10,
  "Track": false,
  "PickDuplicateWindow": .1,
  "NumNucleationThreads": 3,
  "NumHypoThreads": 3,
  "NumWebThreads": 1,
  "WebBackgroundUpdate": false,
  "Params": {
      "Thresh": 6.5,
      "Nucleate": 10,
      "sdAssociate": 3.,
      "sdCut": 3.,
      "expAffinity": 1.,
      "avgDelta": 0.0,
      "avgSigma": 0.75,
      "dCutFactor": 5.0,
      "dCutPercentage": 0.8,
      "dCutMin": 30.0,
      "iCycleLimit": 25,
      "CorrelationTimeWindow": 2.5,
      "CorrelationDistanceWindow": 0.5,
      "CorrelationCancelAge": 900,
      "BeamMatchingAzimuthWindow": 22.5,
      "ReportThresh": 2.,
      "ReportCut":2.
  },
  "OutputFormat":"Event",
  "DefaultNucleationPhase": {
      "PhaseName": "P",
      "TravFile": "./local_example/P.trv"
  },
  "TestLocator": true,
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

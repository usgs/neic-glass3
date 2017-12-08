# glassinitialize.d
# Configuration file for the glass parameters
{
  "Cmd": "Initialize",
  "HypoMax": 500,
  "PickMax": 10000,
  "SitePickMax": 30,
  "CorrelationMax": 1000,
  "Track": false,
  "PickDuplicateWindow": 2.5,
  "NumNucleationThreads": 3,
  "NumHypoThreads":3,
  "WebBackgroundUpdate": true,
  "Params": {
      "Thresh": 0.5,
      "Nucleate": 10,
      "sdAssociate": 3.0,
      "expAffinity": 2.5,
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
      "ReportThresh": 0.5,
      "ReportCut":5
  },
  "OutputFormat":"Event",
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

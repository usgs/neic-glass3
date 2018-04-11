# initialize.d
# Configuration file for the glass parameters
{
  "Cmd": "Initialize",
  "HypoMax": 250,
  "PickMax": 10000,
  "SitePickMax": 30,
  "CorrelationMax": 1000,
  "PickDuplicateWindow": 2.5,
  "NumNucleationThreads": 5,
  "NumHypoThreads": 5,
  "WebBackgroundUpdate": true,
  "Params": {
      "Thresh": 0.5,
      "Nucleate": 10,
      "sdAssociate": 5.0,
      "avgDelta": 0.0,
      "avgSigma": 0.75,
      "dCutFactor": 5.0,
      "dCutPercentage": 0.8,
      "dCutMin": 30.0,
      "iCycleLimit": 25,
      "CorrelationTimeWindow": 2.5,
      "CorrelationDistanceWindow": 0.5,
      "CorrelationCancelAge": 900,
      "BeamMatchingAzimuthWindow" : 22.5,
      "ReportThresh": 0.5,
      "ReportCut":5
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

# culled_global_grid.d
# Configuration file for the glass global grid
{
  "Cmd": "Global",
  "Name": "GlobalCulled",
  "IncludeNetworks": ["IU", "US", "II", "CU", "G", "GE", "IM", "IC", "GT", "C", "AT", "MX" ]
  "NodeResolution": 100.0,
  "NumStationsPerNode": 40,
  "NucleationDataCountThreshold": 7,
  "NucleationStackThreshold": 6.9,
  "DepthLayers": [
      10.0,
      50.0,
      100.0,
      300.0,
      500.0,
      750.0
  ],
  "SaveGrid":false,
  "AzimuthGapTaper": 270.0,
  "ZoneStatsFile": "./global_example/qa_zonestats.txt",
  "DepthResolution": 125.0,
  "NucleationPhases":
  {
    "Phase1":
    {
	    "PhaseName": "P",
	    "TravFile": "./global_example/P.trv"
    }
  }
}
# End of culled_global_grid.d

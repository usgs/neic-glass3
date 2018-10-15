# culled_global_grid.d
# Configuration file for the glass global grid
{
  "Cmd": "Global",
  "Name": "GlobalCulled",
  "IncludeNetworks": ["IU", "US", "II", "CU", "G", "GE", "IM", "IC", "GT", "C", "AU", "MX", "AT"],
  "NodeResolution": 100.0,
  "NumStationsPerNode": 30,
  "NucleationDataCountThreshold": 5,
  "DepthLayers": [
      10.0,
      50.0,
      100.0,
      300.0,
      500.0,
      750.0
  ],
  "SaveGrid":false,
  "UpdateGrid":false, # False because glass-app does not do site lookups.
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

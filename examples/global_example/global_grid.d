# global_grid.d
# Configuration file for the glass global grid
{
  "Cmd": "Global",
  "Name": "Global",
  "NodeResolution": 100.0,
  "NumStationsPerNode": 24,
  "NucleationDataCountThreshold": 6,
  "DepthLayers": [
      10.0,
      30.0,
      50.0,
      100.0,
      200.0,
      300.0,
      400.0,
      500.0,
      600.0,
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
# End of global_grid.d

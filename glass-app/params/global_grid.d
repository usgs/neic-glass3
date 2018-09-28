# global_grid.d
# Configuration file for the glass global grid
{
  "Cmd": "Global",
  "Name": "Global",
  "NodeResolution": 100.0,
  "NumStationsPerNode": 40,
  "NucleationDataCountThreshold": 8,
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
  "NucleationPhases":{
	    "Phase1": {
      		"PhaseName": "P",
      		"TravFile": "./P.trv"
  		}
  }
}
# End of global_grid.d

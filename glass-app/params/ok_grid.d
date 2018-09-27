# ok_grid.d
# Configuration file for the Oklahoma glass grid
{
	"Cmd": "Grid",
	"Name": "Oklahoma",
	"NodeResolution": 25.0,
	"NumberOfRows": 51,
	"NumberOfColumns": 51,
	"CenterLatitude": 36.0,
	"CenterLongitude": -97.5,
	"DepthLayers": [ 10.0 ],
	"NumStationsPerNode": 10,
	"NucleationDataCountThreshold": 6,
	"NucleationStackThreshold": 5.0,
	"UpdateGrid":false, # False because glass-app does not do site lookups.
	"SaveGrid":false,
	"NucleationPhases":{
	    "Phase1": {
      		"PhaseName": "P",
      		"TravFile": "./P.trv"
  		}
	}
}
# End of ok_grid.d

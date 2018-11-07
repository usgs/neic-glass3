# ak_grid.d
# Configuration file for the US glass grid
{
	"Cmd": "Grid",
	"Name": "Alaska",
	"NucleationPhases":{
	    "Phase1": {
      		"PhaseName": "P",
      		"TravFile": "./global_example/P.trv"
  		}
	},
	"NodeResolution": 50.0,
	"NumberOfRows": 31,
	"NumberOfColumns": 41,
	"CenterLatitude": 65.0,
	"CenterLongitude": -147.0,
	"DepthLayers": [ 10.0 , 50.],
	"NumStationsPerNode": 35,
	"NucleationDataCountThreshold": 6,
	"NucleationStackThreshold": 5.9,
	"MaximumDepth": 200.,
	"AzimuthGapTaper":180.,
	"SaveGrid":true
}
# End of ak_grid.d

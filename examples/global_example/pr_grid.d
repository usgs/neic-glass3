# pr_grid.d
# Configuration file for the Hawaii glass grid
{
	"Cmd": "Grid",
	"Name": "Puerto Rico",
	"NodeResolution": 2.0,
	"NumberOfRows": 80,
	"NumberOfColumns": 80,
	"CenterLatitude": 18.23,
	"CenterLongitude": -66.51,
	"DepthLayers": [ 1., 50.],
	"NumStationsPerNode": 14,
	"NucleationDataCountThreshold": 6,
	"NucleationStackThreshold": 5.9,
	"SaveGrid":true,
	"AzimuthGapTaper":270.,
	"NucleationPhases":{
	    "Phase1": {
      		"PhaseName": "P",
      		"TravFile": "./global_example/P.trv"
  		}
	}
}
# End of pr_rid.d

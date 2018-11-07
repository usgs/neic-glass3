# hi_grid.d
# Configuration file for the Hawaii glass grid
{
	"Cmd": "Grid",
	"Name": "Hawaii",
	"NodeResolution": 2.0,
	"NumberOfRows": 80,
	"NumberOfColumns": 80,
	"CenterLatitude": 19.62,
	"CenterLongitude": -155.51,
	"DepthLayers": [ 1.],
	"NumStationsPerNode": 14,
	"NucleationDataCountThreshold": 6,
	"NucleationStackThreshold": 5.9,
	"SaveGrid":true,
	"AzimuthGapTaper":180.,
	"NucleationPhases":{
	    "Phase1": {
      		"PhaseName": "P",
      		"TravFile": "./global_example/P.trv"
  		}
	}
}
# End of hi_grid.d

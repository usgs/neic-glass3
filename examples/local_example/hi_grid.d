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
	"DepthLayers": [ 1.0, 5.0, 10.0],
	"NumStationsPerNode": 15,
	"NucleationDataThreshold": 5,
	"NucleationStackThreshold": 4.8,
	"SaveGrid":true,
	"AzimuthGapTaper":180.0,
	"NucleationPhases":{
	    "Phase1": {
      		"PhaseName": "P",
      		"TravFile": "./local_example/P.trv"
  		}
	}
}

# End of hi_grid.d

# us_grid.d
# Configuration file for the US glass grid
{
	"Cmd": "Grid",
	"Name": "ContinentalUS",
	"NucleationPhases":{
	    "Phase1": {
      		"PhaseName": "P",
      		"TravFile": "./global_example/P.trv"
  		}
	},
	"NodeResolution": 25.0,
	"NumberOfRows": 140,
	"NumberOfColumns": 264,
	"CenterLatitude": 37.5,
	"CenterLongitude": -97.0,
	"DepthLayers": [ 7.5 ],
	"NumStationsPerNode": 35,
	"NucleationDataCountThreshold": 6,
	"NucleationStackThreshold": 5.9,
	"MaximumDepth": 30.,
	"AzimuthGapTaper":180.,
	"SaveGrid":true
}
# End of us_grid.d

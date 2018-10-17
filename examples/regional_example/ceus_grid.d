# ceus_grid.d
# Configuration file for the CEUS glass grid
{
	"Cmd": "Grid",
	"Name": "CEUS",
	"NucleationPhases":{
	    "Phase1": {
      		"PhaseName": "P",
      		"TravFile": "./regional_example/P.trv"
  		},
  		"Phase2": {
      		"PhaseName": "S",
      		"TravFile": "./regional_example/S.trv"
  		}
	},
	"NodeResolution": 12.5,
	"NumberOfRows": 220,
	"NumberOfColumns": 360,
	"CenterLatitude": 37.5,
	"CenterLongitude": -90.0,
	"DepthLayers": [1.0, 10.0],
	"NumStationsPerNode": 25,
	"NucleationDataCountThreshold": 8,
	"NucleationStackThreshold": 7.6,
	"SaveGrid":false,
	"AzimuthalGapTaper":180.0,
	"MaximumDepth": 30.0
}
# End of ceus_grid.d

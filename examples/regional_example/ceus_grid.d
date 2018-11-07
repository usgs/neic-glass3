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
	"DepthLayers": [7.],
	"NumStationsPerNode": 18,
	"NucleationDataCountThreshold": 7,
	"NucleationStackThreshold": 6.7,
	"SaveGrid":true,
	"AzimuthGapTaper":270.,
    "MaximumDepth": 30.
}
# End of ceus_grid.d

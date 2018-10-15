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
	"NumberOfRows": 280,
	"NumberOfColumns": 528,
	"CenterLatitude": 37.5,
	"CenterLongitude": -97.0,
	"DepthLayers": [1.],
	"NumStationsPerNode": 25,
	"NucleationDataCountThreshold": 8,
	"NucleationStackThreshold": 7.6,
	"SaveGrid":true,
	"AzimuthalGapTaper":180.,
	"MaximumDepth": 30.
}
# End of ceus_grid.d

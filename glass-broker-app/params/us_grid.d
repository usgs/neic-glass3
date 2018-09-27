# us_grid.d
# Configuration file for the US glass grid
{
	"Cmd": "Grid",
	"Name": "ContinentalUS",
	"NodeResolution": 50.0,
	"NumberOfRows": 70,
	"NumberOfColumns": 132,
	"CenterLatitude": 37.5,
	"CenterLongitude": -97.0,
	"DepthLayers": [ 10.0 ],
	"NumStationsPerNode": 16,
	"NucleationDataCountThreshold": 7,
	"NucleationStackThreshold": 0.5,
	"UpdateGrid":true,
	"SaveGrid":false,
	"NucleationPhases":{
	    "Phase1": {
      		"PhaseName": "P",
      		"TravFile": "./P.trv"
  		},
	    "Phase2": {
      		"PhaseName": "S",
      		"TravFile": "./S.trv"
  		}
	}
}
# End of us_grid.d

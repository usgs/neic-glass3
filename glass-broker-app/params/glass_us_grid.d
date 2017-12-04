# glass_us_grid.d
# Configuration file for the US glass grid
{
	"Cmd": "Grid",
	"Name": "ContinentalUS",
	"NucleationPhases":{
	    "Phase1": {
      		"PhaseName": "P",
      		"TravFile": "./P.trv"
  		},
	    "Phase2": {
      		"PhaseName": "S",
      		"TravFile": "./S.trv"
  		}
	},
	"Resolution": 50.0,
	"Rows": 70,
	"Cols": 132,
	"Lat": 37.5,
	"Lon": -97.0,
	"Z": [ 10.0 ],
	"Detect": 16,
	"Nucleate": 7,
	"Thresh": 0.5,
	"SaveGrid":true,
	"Update":true
}
# End of glass_us_grid.d

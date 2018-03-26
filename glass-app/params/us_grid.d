# us_grid.d
# Configuration file for the US glass grid
{
	"Cmd": "Grid",
	"Name": "ContinentalUS",
	"Resolution": 50.0,
	"Rows": 70,
	"Cols": 132,
	"Lat": 37.5,
	"Lon": -97.0,
	"Z": [ 10.0 ],
	"Detect": 16,
	"Nucleate": 7,
	"Thresh": 5.5,
	"Update":true,
	"SaveGrid":false,
	"NucleationPhases":{
	    "Phase1": {
      		"PhaseName": "P",
      		"TravFile": "./P.trv"
  		}
	}
}
# End of us_grid.d

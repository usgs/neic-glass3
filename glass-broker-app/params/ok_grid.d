# ok_grid.d
# Configuration file for the Oklahoma glass grid
{
	"Cmd": "Grid",
	"Name": "Oklahoma",
	"Resolution": 25.0,
	"Rows": 51,
	"Cols": 51,
	"Lat": 36.0,
	"Lon": -97.5,
	"Z": [ 10.0 ],
	"Detect": 10,
	"Nucleate": 6,
	"Thresh": 5.0,
	"Update":true,
	"SaveGrid":false,
	"NucleationPhases":{
	    "Phase1": {
      		"PhaseName": "P",
      		"TravFile": "./P.trv"
  		}
	}
}
# End of ok_grid.d

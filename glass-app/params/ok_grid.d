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
	"Detect": 12,
	"Nucleate": 7,
	"Thresh": 0.5,
	"Update":false, # False because glass-app does not do site lookups.
	"SaveGrid":false,
	"NucleationPhases":{
	    "Phase1": {
      		"PhaseName": "P",
      		"TravFile": "./P.trv"
  		}
	}
}
# End of ok_grid.d

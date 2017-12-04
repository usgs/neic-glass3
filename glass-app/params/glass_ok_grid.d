# glass_ok_grid.d
# Configuration file for the Oklahoma glass grid
{
	"Cmd": "Grid",
	"Name": "Oklahoma",
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
	"Resolution": 25.0,
	"Rows": 51,
	"Cols": 51,
	"Lat": 36.0,
	"Lon": -97.5,
	"Z": [ 10.0 ],
	"Detect": 10,
	"Nucleate": 6,
	"Thresh": 0.5,
	"SaveGrid":true
}
# End of glass_ok_grid.d

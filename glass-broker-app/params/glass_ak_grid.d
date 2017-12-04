# glass_ak_grid.d
# Configuration file for the alaska glass grid
{
	"Cmd": "Grid",
	"Name": "Alaska",
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
	"Rows": 31,
	"Cols": 41,
	"Lat": 65.0,
	"Lon": -147.0,
	"Z": [ 10.0 ],
	"Detect": 12,
	"Nucleate": 6,
	"Thresh": 0.5,
	"SaveGrid":true,
	"Update":true
}
# End of glass_ak_grid.d

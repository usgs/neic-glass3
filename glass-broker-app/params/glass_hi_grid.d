# glass_hi_grid.d
# Configuration file for the Hawaii glass grid
{
	"Cmd": "Grid",
	"Name": "Hawaii",
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
	"Rows": 11,
	"Cols": 11,
	"Lat": 19.62,
	"Lon": -155.51,
	"Z": [ 10.0 ],
	"Detect": 10,
	"Nucleate": 6,
	"Thresh": 0.5,
	"SaveGrid":true,
	"Update":true
}
# End of glass_hi_grid.d

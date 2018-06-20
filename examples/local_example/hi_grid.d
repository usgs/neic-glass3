# hi_grid.d
# Configuration file for the Hawaii glass grid
{
	"Cmd": "Grid",
	"Name": "Hawaii",
	"Resolution": 2.0,
	"Rows": 80,
	"Cols": 80,
	"Lat": 19.62,
	"Lon": -155.51,
	"Z": [ 1., 5., 10.],
	"Detect": 15,
	"Nucleate": 5,
	"Thresh": 4.8,
	"SaveGrid":true,
	"AzimuthGapTaper":180.,
	"NucleationPhases":{
	    "Phase1": {
      		"PhaseName": "P",
      		"TravFile": "C:\\hydra\\glass-broker-app\\P.trv"
  		}
	}
}

# End of hi_grid.d

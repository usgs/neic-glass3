# culled_global_grid.d
# Configuration file for the glass global grid
{
  "Cmd": "Global",
  "Name": "CulledGlobal",
  "Nets": ["IU", "US", "II", "CU", "G", "GE", "IM", "IC", "GT", "C", "AU", "MX", "AT", "JP", "NZ"],
  "Resolution": 100.0,
  "Detect": 40,
  "Nucleate": 8,
  "Z": [
      10.0,
      30.0,
      50.0,
      100.0,
      200.0,
      300.0,
      400.0,
      500.0,
      600.0,
      750.0
  ],
  "SaveGrid":false,
	"Update":false, # False because glass-app does not do site lookups.
  "NucleationPhases":{
	    "Phase1": {
      		"PhaseName": "P",
      		"TravFile": "./P.trv"
  		}
  }
}
# End of culled_global_grid.d

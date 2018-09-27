# culled_global_grid.d
# Configuration file for the glass global grid
{
  "Cmd": "Global",
  "Name": "CulledGlobal",
  "IncludeNetworks": ["IU", "US", "II", "CU", "G", "GE", "IM", "IC", "GT", "C", "AU", "MX", "AT", "JP", "NZ"],
  "NodeResolution": 100.0,
  "NumStationsPerNode": 40,
  "NucleationDataCountThreshold": 8,
  "DepthLayers": [
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
	"UpdateGrid":true,
  "NucleationPhases":{
	    "Phase1": {
      		"PhaseName": "P",
      		"TravFile": "./P.trv"
  		}
  }
}
# End of culled_global_grid.d

# glass_global_grid.d
# Configuration file for the glass global grid
{
  "Cmd": "Global",
  "Name": "GlobalCulled",
  "Nets": ["IU", "US", "II", "CU", "G", "GE", "IM", "IC", "GT", "C", "AU", "MX", "AT"],
  "Resolution": 100.0,
  "Detect": 30,
  "Nucleate": 5,
  "Z": [
      10.0,
      50.0,
      100.0,
      300.0,
      500.0,
      750.0
  ],
  "SaveGrid":false,
  "NucleationPhases":
  {
    "Phase1":
    {
	     "PhaseName": "P",
	      "TravFile": "./global_example/P.trv"
    },
    "Phase2":
    {
	     "PhaseName": "S",
	     "TravFile": "./global_example/S.trv"
    }
  }
}
# End of glass_global_grid.d

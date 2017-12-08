# glass_global_grid.d
# Configuration file for the glass global grid
{
  "Cmd": "Global",
  "Name": "Global",
  "Resolution": 100.0,
  "Detect": 24,
  "Nucleate": 6,
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

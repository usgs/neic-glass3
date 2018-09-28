# glass.d
# glass configuration file
{
	# this config is for glass
	"Configuration":"glass-app",

	# Set this logging level
	# trace, debug, info, warning, error, criticalerror
	"LogLevel":"debug",

	# Use this directory for the other glass
	# subcomponent configuration files
	"ConfigDirectory":"./params",

	# Association thread configuration
	# The file containing the configuration
	# to initialize glass.
	"InitializeFile":"initialize.d",

	# The file containing the station list.
	"StationList":"stationlist.d",

	# List of files containing the configuration
	# to define 1 or more global/regional/local grids
	"GridFiles":[
		"global_grid.d",
		"culled_global_grid.d",
		"us_grid.d",
		"ok_grid.d"
	],

	# The file containing the configuration
	# for the input thread.
	"InputConfig":"input.d",

	# The file containing the configuration
	# for the output thread.
	"OutputConfig":"output.d"
}
# End of glass.d

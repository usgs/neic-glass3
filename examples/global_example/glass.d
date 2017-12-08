# glass.d
# glass configuration file
{
	# this config is for glass
	"Cmd":"Glass",

	# Set this logging level
	# trace, debug, info, warning, error, criticalerror
	"LogLevel":"debug",

	# Use this directory for the other glass
	# subcomponent configuration files
	"ConfigDirectory":"./params",

	# Association thread configuration
	# The file containing the configuration
	# to initialize glass.
	"InitializeFile":"glass_initialize.d",

	# The file containing the station list.
	"StationList":"2015_8_14_stationlist.json",

	# List of files containing the configuration
	# to define 1 or more global/regional/local grids
	"GridFiles":[
		"glass_global_grid.d",
		"glass_globalculled_grid.d"
	],

	# The file containing the configuration
	# for the input thread.
	"InputConfig":"glass_input.d",

	# The file containing the configuration
	# for the output thread.
	"OutputConfig":"glass_output.d"
}
# End of glass.d

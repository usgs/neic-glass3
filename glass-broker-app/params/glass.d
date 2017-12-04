# glass.d
# glass configuration file
{
	# this config is for the glass broker app
	"Configuration":"glass-broker-app",

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

	# The file containing the initial station list.
	"StationList":"stationlist.d",

	# List of files containing the configuration
	# to define 1 or more regional/local grids
	"GridFiles":[
		"glass_ak_grid.d",
		"glass_aleutians_grid.d",
		"glass_chile_grid.d",
		"glass_east_pacific_rise_grid.d",
		"glass_eurasia_grid.d",
		"glass_hi_grid.d",
		"glass_indonesia1_grid.d",
		"glass_indonesia2_grid.d",
		"glass_japan1_grid.d",
		"glass_japan2_grid.d",
		"glass_juandefuca_grid.d",
		"glass_kuril_grid.d"
		"glass_north_atlantic_grid.d",
		"glass_ok_grid.d",
		"glass_south_atlantic_grid.d",
		"glass_tonga1_grid.d",
		"glass_tonga2_grid.d",
		"glass_us_grid.d",
		"glass_global_grid.d"
	],

	# The file containing the configuration
	# for the input thread.
	"InputConfig":"glass_input.d",

	# The file containing the configuration
	# for the output thread.
	"OutputConfig":"glass_output.d"
}
# End of glass.d

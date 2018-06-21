# input.d
# Configuration file for the glass input component
{
	# this config is for glass
	"Cmd":"GlassInput",

	# the directory to read input from
	"InputDirectory":"./local_example/input",

	# Whether to shut down when there is no more input data
	"ShutdownWhenNoData":true,

	# The time in seconds to wait before shutting down due to no data
	"ShutdownWait":300,

	# the formats glass will accept
	# glass currently understands the gpick, jsonpick,
	# jsondetect, and ccdata (dat) formats
	"Formats":["jsonpick"],

	# the maximum size of the input queue
	"QueueMaxSize":1000,

	# The default source to use when converting data to json
	"DefaultAgencyID":"US",
	"DefaultAuthor":"glassConverter"
}
# End of input.d

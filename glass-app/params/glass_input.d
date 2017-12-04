# glassinput.d
# Configuration file for the glass input component
{
	# this config is for glass
	"Cmd":"GlassInput",

	# the directory to read input from
	"InputDirectory":"./input",

	# the directory to write errors out to
	"ErrorDirectory":"./error",

	# the directory to archive input
	"ArchiveDirectory":"./archive",

	# the formats glass will accept
	# glass currently understands the gpick, jsonpick,
	# jsonhypo, and ccdata (dat) formats
	"Formats":["gpick","jsonpick","jsonhypo","jsoncorrelation","dat"],

	# the maximum size of the input queue
	"QueueMaxSize":1000,

	# Whether to shut down when there is no more input data
	"ShutdownWhenNoData":true,

	# The time in seconds to wait before shutting down due to no data
	"ShutdownWait":300,

	# The default source to use when converting data to json
	"DefaultAgencyID":"US",
	"DefaultAuthor":"glassConverter"
}
# End of glassinput.d

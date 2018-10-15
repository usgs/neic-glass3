# input.d
# Configuration file for the glass input component
{
	# this config is for glass
	"Configuration":"GlassInput",

	# the directory to read input from
	"InputDirectory":"./global_example/input",

	# the directory to archive input
	"ArchiveDirectory":"./global_example/archive",

	# Whether to shut down when there is no more input data
	"ShutdownWhenNoData":true,

	# The time in seconds to wait before shutting down due to no data
	"ShutdownWait":600,

	# the formats glass will accept
	# glass currently understands the gpick, jsonpick,
	# jsonhypo, and ccdata (dat) formats
	"Format":"gpick",

	# the maximum size of the input queue
	"QueueMaxSize":1000,

	# The default source to use when converting data to json
	"DefaultAgencyID":"US",
	"DefaultAuthor":"Glass3GlobalExample"
}
# End of input.d

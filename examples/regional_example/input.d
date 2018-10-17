# input.d
# Configuration file for the glass input component
{
	# this configuration is for glass broker input
	"Configuration":"GlassInput",

	# the directory to read input from
	"InputDirectory":"./input",

	# the directory to write errors out to
	"ErrorDirectory":"./error",

	# the directory to archive input
	"ArchiveDirectory":"./archive",

	# Whether to shut down when there is no more input data
	"ShutdownWhenNoData":true,

	# The time in seconds to wait before shutting down due to no data
	"ShutdownWait":120,

	# the formats glass will accept
	# glass currently understands the gpick, jsonpick,
	# jsonhypo, and ccdata (dat) formats
	"Format":"jsonpick",

	# the maximum size of the input queue
	"QueueMaxSize":1000,

	# The default source to use when converting data to json
	"DefaultAgencyID":"US",
	"DefaultAuthor":"Glass3RegionalExample"
}
# End of input.d

# inputtest.d
# Configuration file for the glass input component
{
	# this config is for glass
	"Cmd":"GlassInput",

	# the directory to read input from
	"InputDirectory":"./testdata/inputtests",

	# the directory to write errors out to
	"ErrorDirectory":"./testdata/inputtests/error",

	# the directory to archive input
	"ArchiveDirectory":"./testdata/inputtests/archive",

	# the formats glass will accept
	# glass currently understands the gpick, jsonpick,
	# jsonhypo, and ccdata (dat) formats
	"Formats":["gpick","jsonpick","jsondetect","jsoncorl","dat"],

	# the maximum size of the input queue
	"QueueMaxSize":1000,

	# The default source to use when converting data to json
	"DefaultAgencyID":"US",
	"DefaultAuthor":"glassConverter"
}
# End of inputtest.d

# output.d
# Configuration file for the glass input component
{
	# this config is for glass
	"Cmd":"GlassOutput",

	# the times in seconds to publish events
	"PublicationTimes":[20,180],

	# the directory to write output to
	"OutputDirectory":"./output",

	# the format to write output in
	# for now, the only format is json
	"OutputFormat":"json",

	# optional flag to define whether to timestamp
	# output file names, defaults to true.
	"TimeStampFileName":true,

	# The  source to use when outputing data
	"OutputAgencyID":"US",
	"OutputAuthor":"glass"
}
# End of glassoutput.d

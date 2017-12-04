# glassoutput.d 
# Configuration file for the glass input component
{
	# this config is for glass 
	"Cmd":"GlassOutput",

	# the times in seconds to publish events
	"PublicationTimes":[3, 6],

	# the directory to write output to
	"OutputDirectory":"./testdata/outputtests/output",

	# the format to write output in
	# for now, the only format is json
	"OutputFormat":"json",

	# whether to timestamp output file names
	"TimeStampFileName":false,

	# The  source to use when outputing data
	"OutputAgencyID":"US",
	"OutputAuthor":"glass"
}
# End of glassoutput.d
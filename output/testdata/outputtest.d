# outputtest.d 
# Configuration file for the glass output component
{
	# this config is for glass 
	"Cmd":"GlassOutput",

	# the times in seconds after which to publish event detections, multiple
	# entries indicate multiple publications.
	"PublicationTimes":[3,6],

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
# End of outputtest.d
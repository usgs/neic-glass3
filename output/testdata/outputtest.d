# glassoutput.d 
# Configuration file for the glass input component
{
	# this config is for glass 
	"Cmd":"GlassOutput",

	# the delay between when core glass declares a detection
	# and when the detection is written out to file
	# (acts as a filter to reduce false detections)
	"PublicationDelay":[3,6],

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
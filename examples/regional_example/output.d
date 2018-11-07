# output.d
# Configuration file for the glass output component
{
	# this configuration is for glass broker output
	"Configuration":"GlassOutput",

	# the times in seconds to publish events
    "PublicationTimes":[60,300],

	# Whether to publish an event on expiration from glass
	"PublishOnExpiration":false,

	# the directory to write output to
	"OutputDirectory":"./regional_example/output"

	# the format to write output in
	# for now, the only format is json
	"OutputFormat":"json",

	# optional flag to define whether to timestamp
	# output file names, defaults to true.
	"TimeStampFileName":true,

	# The  source to use when outputing data
	"OutputAgencyID":"US",
	"OutputAuthor":"Glass3RegionalExample"
}
# End of output.d

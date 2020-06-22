# output.d
# Configuration file for the glass input component
{
	# this config is for glass
	"Configuration":"GlassOutput",

	# the times in seconds to publish events
    "PublicationTimes":[60,300],

	# Whether to publish an event on expiration from glass
	"PublishOnExpiration":true,

	# Whether to immediately publish a quake that hits 
	# this bayes threshold.
	"ImmediatePublicationThreshold": 30.0,

	# the directory to write output to
	"OutputDirectory":"./global_example/output",

	# the format to write output in
	# for now, the only format is json
	"OutputFormat":"json",

	# optional flag to define whether to timestamp
	# output file names, defaults to true.
	"TimeStampFileName":true,

	# The  source to use when outputing data
	"OutputAgencyID":"US",
	"OutputAuthor":"Glass3GlobalExample"
}
# End of output.d

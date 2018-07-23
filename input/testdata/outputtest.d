# outputtest.d
# Configuration file for the glass output unit tests
{
	# this config is for glass
	"Cmd":"GlassOutput",

	# the times in seconds after which to publish event detections, multiple
	# entries indicate multiple publications.
	"PublicationTimes":[3,6],

	# Whether to publish an event on expiration from glass
	"PublishOnExpiration":true,

	# The optional delay between requesting a current sitelist from glasscore
	# for station list creating in seconds
	"SiteListDelay":72,

	# The optional file name of the stationlist to create from the sitelist from
	# glasscore
	"StationFile":"./testdata/stationlist.d",

	# The  source to use when outputting data
	"OutputAgencyID":"US",
	"OutputAuthor":"glass"
}
# End of outputtest.d
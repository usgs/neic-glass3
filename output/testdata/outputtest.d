# outputtest.d
# Configuration file for the glass output unit tests
{
	# this config is for glass 
	"Configuration":"GlassOutput",

	# the times in seconds after which to publish event detections, multiple
	# entries indicate multiple publications.
	"PublicationTimes":[3,6],

	# Whether to publish an event on expiration from glass
	"PublishOnExpiration":true,

	# The optional delay between requesting a current sitelist from glasscore
	# for station list creating in seconds
	"SiteListRequestInterval":72,

	# The  source to use when outputting data
	"OutputAgencyID":"US",
	"OutputAuthor":"glass"
}
# End of outputtest.d
# glassoutput.d
# Configuration file for the glass output component
{
	# this configuration is for glass broker output
	"Configuration":"glass-broker-app-output",

	# the times in seconds to publish events
	"PublicationTimes":[60,300],

	# the broker to use
	"HazdevBrokerConfig": {
		"Type":"ProducerConfig",
		"Properties":{
		  "client.id":"glass3Default",
			"group.id":"0",
			"metadata.broker.list":"<HazDev Brokers>",
			"retries":"0"
		}
	},

	# the topic the producer will produce to
	"OutputTopic":"Glass3-Default",

	# optional station lookup topic
	"StationRequestTopic":"Station-Lookup",

	# The optional delay between requesting a current sitelist from glasscore
	# for station list creating in seconds
	"SiteListDelay":7200,

	# The optional file name of the stationlist to create from the sitelist from
	# glasscore
	"StationFile":"./params/stationlist.d",

	# The  source to use when outputting data
	"OutputAgencyID":"US",
	"OutputAuthor":"glass3"
}
# End of glassoutput.d

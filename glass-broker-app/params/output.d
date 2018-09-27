# output.d
# Configuration file for the glass output component
{
	# this configuration is for glass broker output
	"Configuration":"GlassOutput",

	# whether to publish a final update when glass
	# is done with the event
	"PublishOnExpiration":false,

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
	"OutputTopics":[
		{	"TopicName":"OK",
			"TopLatitude":38.0,
			"LeftLongitude":-101.0,
			"BottomLatitude":33.0,
			"RightLongitude":-94.0
		},
		{	"TopicName":"CEUS",
			"TopLatitude":50.0,
			"LeftLongitude":-105.0,
			"BottomLatitude":25.0,
			"RightLongitude":-67.0
		},
		{	"TopicName":"DefaultWorld"
		}
	],

	# optional station lookup topic
	"StationRequestTopic":"Station-Lookup",

	# The optional interval between requesting a current sitelist from glasscore
	# for station list creating in seconds
	"SiteListRequestInterval":7200,

	# The optional file name of the stationlist to create from the sitelist from
	# glasscore
	"StationFileName":"./params/stationlist.d",

	# The  source to use when outputting data
	"OutputAgencyID":"US",
	"OutputAuthor":"glass3"
}
# End of output.d

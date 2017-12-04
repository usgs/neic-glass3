# glassinput.d
# Configuration file for the glass input component
{
	# this configuration is for glass broker input
	"Configuration":"glass-broker-app-input",

	# the broker to use
	"HazdevBrokerConfig": {
		"Type":"ConsumerConfig",
		"Properties":{
			"client.id":"glass3Default",
			"group.id":"1",
			"metadata.broker.list":"<HazDev Brokers>",
			"enable.auto.commit":"false"
		}
	},

	# the default topic configuration
	"HazdevBrokerTopicConfig": {
		"Type":"TopicConfig",
		"Properties":{
			"auto.commit.enable":"false",
			"auto.offset.reset":"latest"
		}
	},

	# the topics the consumer will subscribe to
	"Topics":["Dev-RayPicker-1", "Station-Data"],

	# the maximum size of the input queue
	"QueueMaxSize":1000,

	# The default source to use when converting data to json
	"DefaultAgencyID":"US",
	"DefaultAuthor":"glass3"
}
# End of glassinput.d

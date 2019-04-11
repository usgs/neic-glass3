# glass-broker-app

glass-broker-app is an implementation of the neic-glass3 libraries that reads
input, station, and configuration updates from
[HazDev Broker](https://github.com/usgs/hazdev-broker) topics, and writes output
and station information requests to HazDev Broker topics.

## Building

To build glass-broker-app, set the `BUILD_GLASS-BROKER-APP` option equal to
true (1) in the cmake command or GUI. Building glass-broker-app requires
dependencies, such as [hazdev-broker](https://github.com/usgs/hazdev-broker)
and [librdkafka](https://github.com/edenhill/librdkafka).

## Configuration

An example configuration for glass-broker-app is available in the
[glass-broker-app params directory](https://github.com/usgs/neic-glass3/tree/master/glass-broker-app/params)

### glass.d

```json
{
    "Configuration":"glass-broker-app",
    "LogLevel":"debug",
    "ConfigDirectory":"./params",
    "StationList":"stationlist.d",
    "InitializeFile":"initialize.d",
    "GridFiles":[
        "ak_grid.d",
    ],
    "InputConfig":"input.d",
    "OutputConfig":"output.d"
}
```

* **LogLevel** - Sets the minimum logging level, trace, debug, info, warning, error, or criticalerror
* **ConfigDirectory** - The path to directory containing the other glass subcomponent configuration files
* **InitializeFile** - Configuration file containing the neic-glass3 Algorithm configuration
* **StationList** - File containing the initial neic-glass3 station list
* **GridFiles** - One or more files defining detection grids used by neic-glass3
* **InputConfig** - Configuration file containing the broker input configuration
* **OutputConfig** - Configuration file containing the broker output configuration

### input.d

```json
{
    "Configuration":"GlassInput",
    "HazdevBrokerConfig": {
        "Type":"ConsumerConfig",
        "Properties":{
            "client.id":"glass3Default",
            "group.id":"1",
            "metadata.broker.list":"<HazDev Brokers>",
            "enable.auto.commit":"false"
        }
    },
    "HazdevBrokerTopicConfig": {
        "Type":"TopicConfig",
        "Properties":{
            "auto.commit.enable":"false",
            "auto.offset.reset":"latest"
        }
    },
    "Topics":["Dev-RayPicker-1", "Station-Data"],
    "HeartbeatDirectory":"./",
    "BrokerHeartbeatInterval":300,
    "QueueMaxSize":1000,
    "DefaultAgencyID":"US",
    "DefaultAuthor":"glassConverter"
}
```

* **HazdevBrokerConfig** - The HazDev Broker configuration to use for input, see [HazDev-Broker](https://github.com/usgs/hazdev-broker)
* **HazdevBrokerTopicConfig** - The HazDev Broker topic configuration to use, see [HazDev-Broker](https://github.com/usgs/hazdev-broker)
* **Topics** - The HazDev Broker topic(s) to receive input data from
* **HeartbeatDirectory** - An optional key defining where HazDev Broker heartbeat files should be written, if not defined, heartbeat files will not be written.
* **BrokerHeartbeatInterval** - An optional key defining the interval in seconds to expect HazDev Broker heartbeats, if not defined, heatbeats are not expected.
* **QueueMaxSize** - The maximum size of the input queue
* **DefaultAgencyID** - The default agency identifier to use when converting data to json
* **DefaultAuthor** - The default author to use when converting data to json

### output.d

```json
{
    "Configuration":"GlassOutput",
    "PublishOnExpiration":false,
    "PublicationTimes":[20,180],
    "HazdevBrokerConfig": {
        "Type":"ProducerConfig",
        "Properties":{
          "client.id":"glass3Default",
            "group.id":"0",
            "metadata.broker.list":"<HazDev Brokers>",
            "retries":"0"
        }
    },
    "OutputTopics":[
        {
            "TopicName":"OK",
            "TopLatitude":38.0,
            "LeftLongitude":-101.0,
            "BottomLatitude":33.0,
            "RightLongitude":-94.0
        },
        {    "TopicName":"DefaultWorld"
        }
    ],
    "StationRequestTopic":"Station-Lookup",
    "SiteListDelay":7200,
    "StationFile":"./params/stationlist.d",
    "OutputAgencyID":"US",
    "OutputAuthor":"glass"
}
```

* **PublishOnExpiration** - Flag indicating whether to always publish a final version of a detection when it expires out of glass
* **PublicationTimes** - The time(s), in seconds since the detections was first found, to publish 
* **HazdevBrokerConfig** - The HazDev Broker configuration to use for output, see [HazDev-Broker](https://github.com/usgs/hazdev-broker)
* **OutputTopics** - The list of HazDev Broker topics to write output to.
* **TopicName** -
* **TopLatitude** -
* **LeftLongitude** -
* **BottomLatitude** -
* **RightLongitude** -
* **StationRequestTopic** - Optional HazDev Broker topic to request station information.
* **SiteListDelay** - Optional delay between writing updated station files to disk
* **StationFile** - Optional file name of updated station file
* **OutputAgencyID** - The agency identifier to use when generating output data
* **OutputAuthor** - The author to use when generating output data

### neic-glass3 Algorithm

For neic-glass3 algorithmic configuration, see [GLASS 3 Configuration](https://github.com/usgs/neic-glass3/blob/master/doc/GlassConfiguration.md).

## Running

To run glass-broker-app, use the following command: `glass-broker-app <configfile> [logname] [noconsole]` where `<configfile>` is the required path to the glass.d configuration file, `[logname]` is an optional command that defining an alternate name for the glass-broker-app log file, and `[noconsole]` is an optional command specifying that glass-broker-app should not write messages to the console.

glass-broker-app uses the environment variable GLASS_LOG to define the location to write log files
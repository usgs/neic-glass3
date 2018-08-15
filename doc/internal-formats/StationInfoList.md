# StationInfoList Message Specification

## Description

StationInfoList is a message used by the glasscore library. StationInfoList
represents a list of detection formats [StationInfo](https://github.com/usgs/earthquake-detection-formats/blob/master/format-docs/StationInfo.md)
objects used to populate glasscore's internal station list (SiteList class).
The output library can periodically generate the StationInfoList message to
preserve glasscore's internal station list through restarts.

The StationInfoList message uses the [JSON standard](http://www.json.org).

## Output
```json
    {
      "Type"        : "StationInfoList",
      "StationList" : [
        StationInfo, ...]
    }
```

## Glossary
* Type - A string that identifies this message as a StationInfoList.
* StationList - An array of [StationInfo](https://github.com/usgs/earthquake-detection-formats/blob/master/format-docs/StationInfo.md)
objects representing the list of stations

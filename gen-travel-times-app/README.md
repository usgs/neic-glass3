# gen-travel-times-app

A java program used to generate glass3 traveltimes from the NEIC java [travel time](https://github.com/usgs/neic-traveltime) package.

Dependencies
------
* gen-travel-times-app was written in Oracle Java 1.8
* gen-travel-times-app is built with [Gradle](https://gradle.org/), a build.gradle file and gradlew are included in this project
* gen-travel-times-app depends on the [neic-traveltime](https://github.com/usgs/neic-traveltime) package. A copy of this package is included with this package.

Building
------
The steps to get and build gen-travel-times-app.jar using gradle are as follows:

1. Clone neic-glass3.
2. Open a command window and change directories to /neic-glass3/gen-travel-times-app/
3. To build the jar file, run the command `./gradlew build`

Using
-----
Once you are able to build the gen-travel-times-app jar, simply include the jar
file in your application, or call using the App class.

A set of model files used by the gen-travel-times-app is stored in the models/ directory.

Example configuration files are included in `/neic-glass3/gen-travel-times-app/src/main/resources/`

To run this example, run from `/neic-glass3/gen-travel-times-app/` run the command: `java -jar build/libs/gen-travel-times-app-0.1.0.jar --configFile=src/main/resources/config.json`
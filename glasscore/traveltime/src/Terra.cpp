#include "Terra.h"
#include <logger.h>
#include <json.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace traveltime {

// ---------------------------------------------------------CTerra
CTerra::CTerra() {
	clear();
}

// ---------------------------------------------------------CTerra
CTerra::CTerra(std::string filename) {
	load(filename);
}

// ---------------------------------------------------------~CTerra
CTerra::~CTerra() {
}

// ---------------------------------------------------------clear
void CTerra::clear() {
	nLayer = 0;
	dEarthRadius = 0.0;
	iOuterDiscontinuity = -1;
	iInnerDiscontinuity = -1;
	nDiscontinuities = 0;
}

// ---------------------------------------------------------load
bool CTerra::load(std::string filename) {
	// null check
	if (filename == "") {
		glass3::util::Logger::log("error",
								"CTerra::load: Empty filename provided.");
		return (false);
	}

	// reset
	clear();

	// file handle
	FILE *earthModelFile;

	// file buffer
	char buf[4096];
	double dFileLayerRadii[MAXLAYERS];
	double dFileLayerPVel[MAXLAYERS];
	double dFileLayerSVel[MAXLAYERS];

	// Remember the path
	sModelFilePath = filename;

	// open the model file
	earthModelFile = fopen(filename.c_str(), "rt");

	// make sure we loaded a valid file
	if (!earthModelFile) {
		glass3::util::Logger::log(
				"error",
				"CTerra::load: Unable to open earth model file: " + filename);
		return (false);
	}

	// loop through file until
	while (true) {
		// get the next line from the file
		char * line = fgets(buf, 4000, earthModelFile);

		// make sure we got a line
		// (also ends the loop when we get to the end of the file)
		if (!line) {
			break;
		}

		// parse the line into an array
		json::Array arr = parse(line);

		// make sure the line parsed
		if (arr.size() < 1) {
			break;
		}

		// get the first sting in the line
		std::string str = (std::string) arr[0];

		// lines that start with # are comments, skip
		if (str == "#") {
			continue;
		}

		// make sure we've got enough entries on the line
		// we should have 3
		if (arr.size() < 3) {
			break;
		}

		// first string is the radius of the layer
		str = (std::string) arr[0];
		dFileLayerRadii[nLayer] = atof(str.c_str());

		// second string is the P velocity
		str = (std::string) arr[1];
		dFileLayerPVel[nLayer] = atof(str.c_str());

		// third string is the S velocity
		str = (std::string) arr[2];
		dFileLayerSVel[nLayer] = atof(str.c_str());
		nLayer++;
	}

	// close the model file
	fclose(earthModelFile);

	// Assume max depth is center of planet
	dEarthRadius = dFileLayerRadii[nLayer - 1];

	// Invert so are arranged with increasing radius.
	for (int i = 0; i < nLayer; i++) {
		dLayerRadii[i] = dEarthRadius - dFileLayerRadii[nLayer - i - 1];
		dLayerPVel[i] = dFileLayerPVel[nLayer - i - 1];
		dLayerSVel[i] = dFileLayerSVel[nLayer - i - 1];
	}

	// for each layer
	for (int i = 0; i < nLayer - 1; i++) {
		// is this a discontinuity
		if ((dLayerRadii[i] > (dLayerRadii[i + 1] - 0.001))
				&& (dLayerRadii[i] < (dLayerRadii[i + 1] + 0.001))) {
			// save this discontinuity
			iDiscontinuityIndexes[nDiscontinuities++] = i;

			// is this the inner core discontinuity
			if ((dLayerSVel[i] > 0.01) && (dLayerSVel[i + 1] < 0.01)) {
				iInnerDiscontinuity = i;
			}

			// is this the outer core discontinuity
			if ((dLayerSVel[i] < 0.01) && (dLayerSVel[i + 1] > 0.01)) {
				iOuterDiscontinuity = i;
			}
		}
	}

	glass3::util::Logger::log(
			"debug",
			"CTerra::load: Loaded earth model file: " + filename);

	return (true);
}

// ---------------------------------------------------------parse
json::Array CTerra::parse(const char *line) {
	// NOTE: Why are we using a json array!?
	// switch to vector or something that doesn't require
	// an external library
	json::Array arr;
	char str[1000];

	// set up to search for the start of a string
	int state = 0;
	int strLength = 0;

	// For each character in the line
	for (int i = 0; i < static_cast<int>(strlen(line)); i++) {
		char c = line[i];
		switch (state) {
			case 0:
				// Looking for first non-blank
				if (c == ' ' || c == '\t' || c == ',' || c == '\n'
						|| c == '\r') {
					continue;
				}

				// copy first character
				str[0] = c;

				// init string length to 1
				strLength = 1;

				// found the start of the string,
				// setup to search for the end
				state = 1;
				break;

			case 1:
				// Looking for end of string
				if (c == ' ' || c == '\t' || c == ',' || c == '\n'
						|| c == '\r') {
					// found end of string
					// null terminate
					str[strLength] = 0;

					// add string to array
					arr.push_back(std::string(str));

					// reset for next string
					// set up to search for the start of a string
					strLength = 0;
					state = 0;
				} else {
					// not at end, copy next character
					str[strLength++] = c;
				}
				break;
		}
	}

	// if we have a valid string
	if (strLength) {
		// null terminate
		str[strLength] = 0;

		// add string to array
		arr.push_back(std::string(str));
	}

	// return the array of strings
	return (arr);
}

// ---------------------------------------------------------P
double CTerra::P(double radius) {
	// Interpolate velocity for P, currently linear only
	return (interpolateVelocity(radius, dLayerPVel));
}

// ---------------------------------------------------------S
double CTerra::S(double radius) {
	// Interpolate velocity for S, currently linear only
	return (interpolateVelocity(radius, dLayerSVel));
}

// ---------------------------------------------------------interpolateVelocity
double CTerra::interpolateVelocity(double radius, double *layerVelocity) {
	double velocity;
	int lowerIndex = -1;
	int upperIndex = nLayer;

	// use a binary search to find the layer index for the given
	// earth radius.  The found index is stored in the lower index
	while (upperIndex - lowerIndex > 1) {
		// compute the current middle index
		// right shift bits by one (effectively divide by 2)
		int middleIndex = (upperIndex + lowerIndex) >> 1;

		if (radius > dLayerRadii[middleIndex]) {
			// if the given radius is greater than layer radius of
			// the middle index, set the lower index to the current
			// middle index
			lowerIndex = middleIndex;
		} else {
			// if the given radius is less than layer radius of
			// the middle index, set the upper index to the current
			// middle index
			upperIndex = middleIndex;
		}
	}

	// lower index can't be less than 0
	if (lowerIndex < 0) {
		lowerIndex = 0;
	}

	// lower index also can't be greater than the number of layers
	if (lowerIndex > nLayer - 2) {
		lowerIndex = nLayer - 2;
	}

	// compute velocity at the identified layer
	velocity =
			layerVelocity[lowerIndex]
					+ (radius - dLayerRadii[lowerIndex])
							* (layerVelocity[lowerIndex + 1]
									- layerVelocity[lowerIndex])
							/ (dLayerRadii[lowerIndex + 1]
									- dLayerRadii[lowerIndex]);

	return (velocity);
}

// ---------------------------------------------------------calculateTurnRadius
double CTerra::calculateTurnRadius(int lowerIndex, int upperIndex,
									double *layerVelocity, double rayParam) {
	int myLowerIndex = lowerIndex;
	int myUpperIndex = upperIndex;
	int middleIndex;

	// use a binary search to find the layer index for the given
	// earth radius.  The found index is stored in the lower index
	while (myUpperIndex - myLowerIndex > 1) {
		// compute the current middle index
		middleIndex = (myUpperIndex + myLowerIndex) >> 1;

		// compute the current ray parameter
		double currentRayParam = dLayerRadii[middleIndex]
				/ layerVelocity[middleIndex];

		if (rayParam > currentRayParam) {
			// if the given ray param is greater than the current ray param at
			// the middle index, set the lower index to the current
			// middle index
			myLowerIndex = middleIndex;
		} else {
			// if the given ray param is less than the current ray param at
			// the middle index, set the upper index to the current
			// middle index
			myUpperIndex = middleIndex;
		}
	}
	// lower index can't be less than 0
	if (myLowerIndex < 0) {
		myLowerIndex = 0;
	}

	// lower index also can't be greater than the number of layers
	if (myLowerIndex > nLayer - 2) {
		myLowerIndex = nLayer - 2;
	}

	// get radii and velocities for calculation
	double radius = dLayerRadii[myLowerIndex];
	double radius2 = dLayerRadii[myLowerIndex + 1];
	double velocity = layerVelocity[myLowerIndex];
	double velocity2 = layerVelocity[myLowerIndex + 1];

	// Messy formula for radius because velocity not rayparam is linearly
	// interpolated.
	double turnRadius = rayParam
			* (radius * (velocity2 - velocity) - velocity * (radius2 - radius))
			/ (rayParam * (velocity2 - velocity) - (radius2 - radius));

	return (turnRadius);
}

// ---------------------------------------------------------evaluateFunction
double CTerra::evaluateFunction(int functionIndex, double earthRadius,
								double rayParam) {
	double velocity;
	double result = 0.0;

	switch (functionIndex) {
		case FUN_TEST:
			// test evaluateFunction
			result = 1.0 / sqrt(earthRadius);
			// result = sin(earthRadius);
			// result = earthRadius*earthRadius;
			break;

		case FUN_P_TIME:
			// evaluate the time function for P
			velocity = P(earthRadius);
			result = 1.0
					/ sqrt(1.0 / velocity / velocity
							- rayParam * rayParam / earthRadius / earthRadius)
					/ velocity / velocity;
			break;

		case FUN_P_DELTA:
			// evaluate the distance function for P
			velocity = P(earthRadius);
			result = rayParam
					/ sqrt(1.0 / velocity / velocity
							- rayParam * rayParam / earthRadius / earthRadius)
					/ earthRadius / earthRadius;
			break;

		case FUN_P_TAU:
			// evaluate the Tau function for P
			velocity = P(earthRadius);
			result = sqrt(
					1.0 / velocity / velocity
							- rayParam * rayParam / earthRadius / earthRadius);
			break;

		case FUN_S_TIME:
			// evaluate the time function for S
			velocity = S(earthRadius);
			result = 1.0
					/ sqrt(1.0 / velocity / velocity
							- rayParam * rayParam / earthRadius / earthRadius)
					/ velocity / velocity;
			break;

		case FUN_S_DELTA:
			// evaluate the distance function for S
			velocity = S(earthRadius);
			result = rayParam
					/ sqrt(1.0 / velocity / velocity
							- rayParam * rayParam / earthRadius / earthRadius)
					/ earthRadius / earthRadius;
			break;

		case FUN_S_TAU:
			// evaluate the tau function for S
			velocity = S(earthRadius);
			result = sqrt(
					1.0 / velocity / velocity
							- rayParam * rayParam / earthRadius / earthRadius);
			break;
	}

	return (result);
}

// ---------------------------------------------------------integrateRaySegment
double CTerra::integrateRaySegment(int functionIndex, double startingRadius,
									double endingRadius, double rayParam) {
	double myStartingRadius = startingRadius;
	double myEndingRadius = endingRadius;
	double result = 0.0;

	// for the number of seismic discontinuities
	for (int i = 0; i < nDiscontinuities; i++) {
		// get the current discontinuities
		int currentBreak = iDiscontinuityIndexes[i];

		// continue if the starting radius is less or equal to the
		// current discontinuity radius
		if (dLayerRadii[currentBreak] <= myStartingRadius) {
			continue;
		}

		// compute if the ending radius is greater than the
		// current discontinuity radius
		if (dLayerRadii[currentBreak] < myEndingRadius) {
			// compute the current function integral and add to the result
			result += rombergIntegration(functionIndex, myStartingRadius,
											dLayerRadii[currentBreak],
											rayParam);

			// move to the next earth radius
			myStartingRadius = dLayerRadii[currentBreak + 1];
		}
	}
	if (myEndingRadius > myStartingRadius) {
		result += rombergIntegration(functionIndex, myStartingRadius,
										myEndingRadius, rayParam);
	}

	return (result);
}

// ---------------------------------------------------------rombergIntegration
double CTerra::rombergIntegration(int functionIndex, double startingRadius,
									double endingRadius, double rayParam) {
	double accuracy = 1.0e-6;
	double stepSize = 2;
	double lowerLimit = startingRadius;
	double upperLimit = endingRadius;
	double rows[ROMB_MAX + 1];
	int rowIndex = 0;
	double prevInt = 0;
	int countLimit = 1;

	rows[0] = 0;
	upperLimit -= lowerLimit;

	// loop until we've hit the maximum number of steps
	for (int step = 0; step < ROMB_MAX; ++step) {
		double currentStep = stepSize / 2 - 1;
		double currentSum = 0.0;

		for (int count = 0; count < countLimit; count++) {
			double t = 1 - currentStep * currentStep;
			double currentLimit = currentStep + t * currentStep / 2;
			currentLimit = (currentLimit * upperLimit + upperLimit) / 2
					+ lowerLimit;
			currentSum += t
					* evaluateFunction(functionIndex, currentLimit, rayParam);
			currentStep += stepSize;
		}

		// compute trapezoidal steps
		currentStep = 4;
		double previousRow = rows[0];
		rows[0] = (rows[0] + stepSize * currentSum) / 2.0;
		for (rowIndex = 0; rowIndex <= step; rowIndex++) {
			double nextRow = rows[rowIndex + 1];
			rows[rowIndex + 1] = (currentStep * rows[rowIndex] - previousRow)
					/ (currentStep - 1);
			previousRow = nextRow;
			currentStep *= 4;
		}

		// break out of loop if we're accurate enough
		if (fabs(rows[rowIndex] - prevInt)
				< accuracy * fabs(rows[rowIndex]) * 16) {
			break;
		}

		// save this trapezoidal step for next loop
		prevInt = rows[rowIndex];

		// increase lim for next loop
		// left shift bits by one
		// (effectively multiply by 2)
		countLimit <<= 1;

		// decrease step size for next loop
		stepSize /= 2.0;
	}

	// compute final value from the last trapezoidal step and
	// upper limit
	double value = rows[rowIndex] * upperLimit * 3 / 4;

	return (value);
}
}  // namespace traveltime

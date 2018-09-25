#include "GenTrv.h"
#include <logger.h>
#include <glassmath.h>
#include <json.h>
#include <cmath>
#include <cstdio>
#include <string>
#include "Spline.h"
#include "Terra.h"
#include "Ray.h"
#include "TimeWarp.h"

namespace traveltime {

// ---------------------------------------------------------CGenTrv
CGenTrv::CGenTrv() {
	clear();
	m_OutputPath = "";
	m_FileExtension = "";
	bSetup = false;
	pDistanceWarp = NULL;
	pDepthWarp = NULL;
	pTerra = NULL;
	pRay = NULL;
}

// ---------------------------------------------------------CGenTrv
CGenTrv::CGenTrv(std::string modelFile, std::string outputPath,
					std::string fileExtension) {
	clear();
	m_OutputPath = "";
	m_FileExtension = "";
	bSetup = false;
	pDistanceWarp = NULL;
	pDepthWarp = NULL;
	pTerra = NULL;
	pRay = NULL;

	setup(modelFile, outputPath, fileExtension);
}

// ---------------------------------------------------------~CGenTrv
CGenTrv::~CGenTrv() {
	if (pTerra != NULL) {
		delete (pTerra);
	}
	pTerra = NULL;

	if (pRay != NULL) {
		delete (pRay);
	}
	pRay = NULL;
	m_OutputPath = "";
	m_FileExtension = "";
	bSetup = false;
}

// ---------------------------------------------------------clear
void CGenTrv::clear() {
	nDistanceWarp = 0;
	nDepthWarp = 0;
	nRays = 0;
	rayParameter = 0;
	dDepthDistance = 0;
	dTravelTime = 0;
	Phase = "";
	vRays.clear();

	if (pDistanceWarp != NULL) {
		delete (pDistanceWarp);
	}
	pDistanceWarp = NULL;

	if (pDepthWarp != NULL) {
		delete (pDepthWarp);
	}
	pDepthWarp = NULL;
}

// ---------------------------------------------------------setup
bool CGenTrv::setup(std::string modelFile, std::string outputPath,
					std::string fileExtension) {
	if (bSetup == true) {
		return(true);
	}

	// create earth structure class
	pTerra = new CTerra();

	// load earth model
	if (pTerra->load(modelFile.c_str())) {
		glass3::util::Logger::log(
				"debug",
				"CGenTrv::setup: Terra nLayer: "
						+ std::to_string(pTerra->nLayer) + " dEarthRadius: "
						+ std::to_string(pTerra->dEarthRadius));
	} else {
		glass3::util::Logger::log(
				"error",
				"CGenTrv::setup: Unable to load Model: " + modelFile);
		return (false);
	}

	// create ray path class
	pRay = new CRay();
	pRay->pTerra = pTerra;

	m_OutputPath = outputPath;
	m_FileExtension = fileExtension;
	bSetup = true;

	return(true);
}

// ---------------------------------------------------------generate
bool CGenTrv::generate(json::Object *com) {
	if (bSetup == false) {
		glass3::util::Logger::log("error",
										"CGenTrv::generate: Call setup before "
										"generate.");
		return(false);
	}

	// null check json
	if (com == NULL) {
		glass3::util::Logger::log("error",
								"CGenTrv::generate: NULL json communication.");
		return (false);
	}

	clear();

	// check for a command
	if (com->HasKey("Cmd")
			&& ((*com)["Cmd"].GetType() == json::ValueType::StringVal)) {
		std::string command = (*com)["Cmd"].ToString();

		if (command != "GenerateTraveltime") {
			glass3::util::Logger::log(
					"warning",
					"CGenTrv::generate: Non-Correlation message "
					"passed in.");
			return (false);
		}
	} else {
		// no command or type
		glass3::util::Logger::log("error",
								"CGenTrv::generate: Missing required Cmd Key.");
		return (false);
	}

	// definition variables
	std::string branchName = "";
	nRays = 0;

	// branch name
	if (com->HasKey("Branch")
			&& ((*com)["Branch"].GetType() == json::ValueType::StringVal)) {
		branchName = (*com)["Branch"].ToString();
		glass3::util::Logger::log(
				"info",
				"CGenTrv::generate: Using Branch: " + branchName);
	} else {
		glass3::util::Logger::log(
				"error",
				"CGenTrv::generate: Missing required Branch Key.");
		return (false);
	}

	// get phases
	if ((com->HasKey("Rays"))
			&& ((*com)["Rays"].GetType() == json::ValueType::ArrayVal)) {
		// get the array of phase entries
		json::Array rays = (*com)["Rays"].ToArray();

		// for each phase in the array
		for (auto val : rays) {
			// make sure the phase is an object
			if (val.GetType() != json::ValueType::StringVal) {
				continue;
			}

			std::string ray = val.ToString();

			if (ray == "") {
				continue;
			}

			// add phase to list
			nRays++;
			vRays.push_back(ray);

			glass3::util::Logger::log("info",
									"CGenTrv::generate: Using ray: " + ray);
		}
	} else {
		glass3::util::Logger::log(
				"error",
				"CGenTrv::generate: Missing required Phases key");
		return (false);
	}

	// pDistanceWarp
	if ((com->HasKey("DeltaTimeWarp"))
			&& ((*com)["DeltaTimeWarp"].GetType() == json::ValueType::ObjectVal)) {
		// get object
		json::Object delta = (*com)["DeltaTimeWarp"].ToObject();

		// timewarp variables
		double gridMin = 0;
		double gridMax = 0;
		double decayConst = 0;
		double slopeZero = 0;
		double slopeInf = 0;

		// gridMin
		if ((delta.HasKey("MinimumDistance"))
				&& (delta["MinimumDistance"].GetType()
						== json::ValueType::DoubleVal)) {
			gridMin = delta["MinimumDistance"].ToDouble();

			glass3::util::Logger::log(
					"info",
					"CGenTrv::generate: Using DeltaTimeWarp MinimumDistance: "
							+ std::to_string(gridMin));
		} else {
			glass3::util::Logger::log(
					"error",
					"CGenTrv::generate: Missing required DeltaTimeWarp MinimumDistance key.");
			return (false);
		}

		// gridMax
		if ((delta.HasKey("MaximumDistance"))
				&& (delta["MaximumDistance"].GetType()
						== json::ValueType::DoubleVal)) {
			gridMax = delta["MaximumDistance"].ToDouble();

			glass3::util::Logger::log(
					"info",
					"CGenTrv::generate: Using DeltaTimeWarp MaximumDistance: "
							+ std::to_string(gridMax));
		} else {
			glass3::util::Logger::log(
					"error",
					"CGenTrv::generate: Missing required DeltaTimeWarp MaximumDistance key.");
			return (false);
		}

		// decayConst
		if ((delta.HasKey("SlopeDecayConstant"))
				&& (delta["SlopeDecayConstant"].GetType()
						== json::ValueType::DoubleVal)) {
			decayConst = delta["SlopeDecayConstant"].ToDouble();

			glass3::util::Logger::log(
					"info",
					"CGenTrv::generate: Using DeltaTimeWarp SlopeDecayConstant: "
							+ std::to_string(decayConst));
		} else {
			glass3::util::Logger::log(
					"error",
					"CGenTrv::generate: Missing required DeltaTimeWarp "
					"SlopeDecayConstant key.");
			return (false);
		}

		// slopeZero
		if ((delta.HasKey("SlopeZero"))
				&& (delta["SlopeZero"].GetType() == json::ValueType::DoubleVal)) {
			slopeZero = delta["SlopeZero"].ToDouble();

			glass3::util::Logger::log(
					"info",
					"CGenTrv::generate: Using DeltaTimeWarp SlopeZero: "
							+ std::to_string(slopeZero));
		} else {
			glass3::util::Logger::log(
					"error",
					"CGenTrv::generate: Missing required DeltaTimeWarp "
					"SlopeZero key.");
			return (false);
		}

		// slopeInf
		if ((delta.HasKey("SlopeInfinite"))
				&& (delta["SlopeInfinite"].GetType()
						== json::ValueType::DoubleVal)) {
			slopeInf = delta["SlopeInfinite"].ToDouble();

			glass3::util::Logger::log(
					"info",
					"CGenTrv::generate: Using DeltaTimeWarp SlopeInfinite: "
							+ std::to_string(slopeInf));
		} else {
			glass3::util::Logger::log(
					"error",
					"CGenTrv::generate: Missing required DeltaTimeWarp "
					"SlopeInfinite key.");
			return (false);
		}

		// create distance timewarp
		pDistanceWarp = new CTimeWarp(gridMin, gridMax, decayConst, slopeZero,
										slopeInf);
		nDistanceWarp = pDistanceWarp->grid(gridMax);

		glass3::util::Logger::log(
				"debug",
				"CGenTrv::generate: pDistanceWarp: minDistance: "
						+ std::to_string(
								pDistanceWarp->value(
										pDistanceWarp->grid(gridMin)))
						+ " -> maxDistance:"
						+ std::to_string(
								pDistanceWarp->value(
										pDistanceWarp->grid(gridMax))));
	} else {
		glass3::util::Logger::log(
				"error",
				"CGenTrv::generate: Missing required DeltaTimeWarp key");
		return (false);
	}

	// pDepthWarp
	if ((com->HasKey("DepthTimeWarp"))
			&& ((*com)["DepthTimeWarp"].GetType() == json::ValueType::ObjectVal)) {
		// get object
		json::Object delta = (*com)["DepthTimeWarp"].ToObject();

		// timewarp variables
		double gridMin = 0;
		double gridMax = 0;
		double decayConst = 0;
		double slopeZero = 0;
		double slopeInf = 0;

		// gridMin
		if ((delta.HasKey("MinimumDepth"))
				&& (delta["MinimumDepth"].GetType()
						== json::ValueType::DoubleVal)) {
			gridMin = delta["MinimumDepth"].ToDouble();

			glass3::util::Logger::log(
					"info",
					"CGenTrv::generate: Using DepthTimeWarp MinimumDepth: "
							+ std::to_string(gridMin));
		} else {
			glass3::util::Logger::log(
					"error",
					"CGenTrv::generate: Missing required DepthTimeWarp "
					"MinimumDepth key.");
			return (false);
		}

		// gridMax
		if ((delta.HasKey("MaximumDepth"))
				&& (delta["MaximumDepth"].GetType()
						== json::ValueType::DoubleVal)) {
			gridMax = delta["MaximumDepth"].ToDouble();

			glass3::util::Logger::log(
					"info",
					"CGenTrv::generate: Using DepthTimeWarp MaximumDepth: "
							+ std::to_string(gridMax));
		} else {
			glass3::util::Logger::log(
					"error",
					"CGenTrv::generate: Missing required DepthTimeWarp "
					"MaximumDepth key.");
			return (false);
		}

		// decayConst
		if ((delta.HasKey("SlopeDecayConstant"))
				&& (delta["SlopeDecayConstant"].GetType()
						== json::ValueType::DoubleVal)) {
			decayConst = delta["SlopeDecayConstant"].ToDouble();

			glass3::util::Logger::log(
					"info",
					"CGenTrv::generate: Using DepthTimeWarp SlopeDecayConstant: "
							+ std::to_string(decayConst));
		} else {
			glass3::util::Logger::log(
					"error",
					"CGenTrv::generate: Missing required DepthTimeWarp "
					"SlopeDecayConstant key.");
			return (false);
		}

		// slopeZero
		if ((delta.HasKey("SlopeZero"))
				&& (delta["SlopeZero"].GetType() == json::ValueType::DoubleVal)) {
			slopeZero = delta["SlopeZero"].ToDouble();

			glass3::util::Logger::log(
					"info",
					"CGenTrv::generate: Using DepthTimeWarp SlopeZero: "
							+ std::to_string(slopeZero));
		} else {
			glass3::util::Logger::log(
					"error",
					"CGenTrv::generate: Missing required DepthTimeWarp SlopeZero key.");
			return (false);
		}

		// slopeInf
		if ((delta.HasKey("SlopeInfinite"))
				&& (delta["SlopeInfinite"].GetType()
						== json::ValueType::DoubleVal)) {
			slopeInf = delta["SlopeInfinite"].ToDouble();

			glass3::util::Logger::log(
					"info",
					"CGenTrv::generate: Using DepthTimeWarp SlopeInfinite: "
							+ std::to_string(slopeInf));
		} else {
			glass3::util::Logger::log(
					"error",
					"CGenTrv::generate: Missing required DepthTimeWarp SlopeInfinite key.");
			return (false);
		}

		// create depth timewarp
		pDepthWarp = new CTimeWarp(gridMin, gridMax, decayConst, slopeZero,
									slopeInf);
		nDepthWarp = pDepthWarp->grid(gridMax);

		glass3::util::Logger::log(
				"debug",
				"CGenTrv::generate: pDepthWarp: minDepth: "
						+ std::to_string(
								pDepthWarp->value(pDepthWarp->grid(gridMin)))
						+ " -> maxDepth:"
						+ std::to_string(
								pDepthWarp->value(pDepthWarp->grid(gridMax))));
	} else {
		glass3::util::Logger::log(
				"error",
				"CGenTrv::generate: Missing required DepthTimeWarp key");
		return (false);
	}

	glass3::util::Logger::log(
			"debug",
			"CGenTrv::generate: nDistanceWarp: " + std::to_string(nDistanceWarp)
					+ " nDepthWarp: " + std::to_string(nDepthWarp));

	// Unit tests
	// UnitTest("Pup", 1, 250, 34.99);
	// UnitTest("P", 30, 50, 363.82);
	// UnitTest("P", 50, 250, 509.04);
	// UnitTest("P", 90, 100, 768.24);

	glass3::util::Logger::log("info",
							"CGenTrv::generate: Generating depth layers.");

	// set up for interpolation
	double *travelTimeArray = new double[nDistanceWarp * nDepthWarp];
	double *depthDistanceArray = new double[nDistanceWarp * nDepthWarp];
	char *phaseArray = new char[nDistanceWarp * nDepthWarp];

	// generate interpolation grid (depth, time)
	// for each depth
	for (int iDepth = 0; iDepth < nDepthWarp; iDepth++) {
		// generate row
		Row(iDepth, &travelTimeArray[iDepth * nDistanceWarp],
			&depthDistanceArray[iDepth * nDistanceWarp],
			&phaseArray[iDepth * nDistanceWarp]);

		// init min/max for this depth
		int minDelta = -1;
		int maxDelta = -1;

		// get travel times for this depth
		double *tDistTravelTime = &travelTimeArray[iDepth * nDistanceWarp];

		// for each distance at this depth
		for (int iDelta = 0; iDelta < nDistanceWarp; iDelta++) {
			glass3::util::Logger::log(
					"debug",
					"CGenTrv::generate: iDelta: " + std::to_string(iDelta)
							+ " distance: "
							+ std::to_string(pDistanceWarp->value(iDelta)));

			// check to see if there is a valid travel time at this
			// distance for this depth
			if (tDistTravelTime[iDelta] > 0.0) {
				// only set minimum distance if there is not one already
				if (minDelta < 0) {
					minDelta = iDelta;
				}

				// set maximum distance
				maxDelta = iDelta;
			}
		}

		// check to see if there was a valid travel time for this depth
		if (maxDelta < 0) {
			glass3::util::Logger::log(
					"debug",
					"CGenTrv::generate: iDepth:" + std::to_string(iDepth)
							+ " depth:"
							+ std::to_string(pDepthWarp->value(iDepth))
							+ " No Travel Times for this depth.");

			continue;
		}
	}

	// Create output file
	// Compute header values
	// type is a trav file
	std::string fileType = "TRAV";

	// set endian
	int16_t endianType = 1;

	// copy branch name
	char branch[16];
	int branchNameSize = branchName.size();
	for (int i = 0; i < 16; i++) {
		if (i < branchNameSize) {
			branch[i] = branchName[i];
		} else {
			// null terminate
			branch[i] = 0;
		}
	}

	// generate phase list
	std::string phaseList = "";
	for (int i = 0; i < nRays; i++) {
		if (i < 0) {
			// comma delimit
			phaseList += ",";
		}
		phaseList += vRays[i];
	}

	// copy phase list
	char phases[64];
	int phaseListSize = phaseList.size();
	for (int i = 0; i < 64; i++) {
		if (i < phaseListSize) {
			phases[i] = phaseList[i];
		} else {
			// null terminate
			phases[i] = 0;
		}
	}

	// open file
	std::string outFileName = m_OutputPath + branchName + m_FileExtension;

	glass3::util::Logger::log("info",
							"CGenTrv::generate: writing file: " + outFileName);

	FILE *outFile = fopen(outFileName.c_str(), "wb");

	// write header values
	fwrite(&fileType, 1, 4, outFile);
	fwrite(&endianType, 1, 2, outFile);
	fwrite(branch, 1, 16, outFile);
	fwrite(phases, 1, 64, outFile);

	// Output distance warp parameters
	fwrite(&nDistanceWarp, 1, 4, outFile);
	fwrite(&pDistanceWarp->dGridMinimum, 1, 8, outFile);
	fwrite(&pDistanceWarp->dGridMaximum, 1, 8, outFile);
	fwrite(&pDistanceWarp->dDecayConstant, 1, 8, outFile);
	fwrite(&pDistanceWarp->dSlopeZero, 1, 8, outFile);
	fwrite(&pDistanceWarp->dSlopeInfinity, 1, 8, outFile);

	// Output depth warp paramters
	fwrite(&nDepthWarp, 1, 4, outFile);
	fwrite(&pDepthWarp->dGridMinimum, 1, 8, outFile);
	fwrite(&pDepthWarp->dGridMaximum, 1, 8, outFile);
	fwrite(&pDepthWarp->dDecayConstant, 1, 8, outFile);
	fwrite(&pDepthWarp->dSlopeZero, 1, 8, outFile);
	fwrite(&pDepthWarp->dSlopeInfinity, 1, 8, outFile);

	// Output interpolation grids
	fwrite(travelTimeArray, 1, 8 * nDistanceWarp * nDepthWarp, outFile);
	fwrite(depthDistanceArray, 1, 8 * nDistanceWarp * nDepthWarp, outFile);
	fwrite(phaseArray, 1, nDistanceWarp * nDepthWarp, outFile);

	// done with file
	fclose(outFile);

	glass3::util::Logger::log("info",
							"CGenTrv::generate: file writing complete.");

	// cleanup
	delete[] (travelTimeArray);
	delete[] (depthDistanceArray);
	delete[] (phaseArray);

	return (true);
}

// ---------------------------------------------------------Row
int CGenTrv::Row(int iDepth, double *travelTimeArray,
					double *depthDistanceArray, char *phaseArray) {
	glass3::util::Logger::log(
			"debug",
			"CGenTrv::Row: iDepth: " + std::to_string(iDepth) + +" depth:"
					+ std::to_string(pDepthWarp->value(iDepth)));

	// array to hold ray parameters
	// NOTE: This doesn't appear to be used by anything
	double *rayParameters = new double[nDistanceWarp];

	// init travelTimeArray
	for (int i = 0; i < nDistanceWarp; i++) {
		travelTimeArray[i] = -10.0;
	}

	// get current depth
	double depth = pDepthWarp->value(iDepth);

	// for each phase
	for (int phaseIndex = 0; phaseIndex < nRays; phaseIndex++) {
		// get the current phase
		std::string phase = vRays[phaseIndex];

		glass3::util::Logger::log("debug",
								"CGenTrv::Row: Phase:" + phase);

		// for each distance at this depth
		for (int distanceIndex = 0; distanceIndex < nDistanceWarp;
				distanceIndex++) {
			// get the current distance
			double distance = pDistanceWarp->value(distanceIndex);

			// generate traveltime for this phase / distance / depth
			if (T(phase, distance, depth)) {
				// check to see if the travel time array has either an invalid
				// travel time OR the new travel time is earlier than the travel
				// time stored in the array
				if ((travelTimeArray[distanceIndex] < 0.0)
						|| (dTravelTime < travelTimeArray[distanceIndex])) {
					// set arrays to new time/distance/ray/phase
					travelTimeArray[distanceIndex] = dTravelTime;
					depthDistanceArray[distanceIndex] = dDepthDistance;
					rayParameters[distanceIndex] = rayParameter;

					// NOTE: we're putting an int into a char array?!?!
					phaseArray[distanceIndex] = phaseIndex;
				}
			}
		}
	}

	// Patch holes (discontinuities?)
	int holeStart = -1;
	int holeEnd;
	int state = 0;
	int nHoles = 0;
	for (int distanceIndex = 0; distanceIndex < nDistanceWarp;
			distanceIndex++) {
		switch (state) {
			case 0:
				// Looking for first good travel time value
				if (travelTimeArray[distanceIndex] > 0) {
					// found it, start looking for last good
					state = 1;
				}
				break;
			case 1:
				// Looking for last good travel time value
				if (travelTimeArray[distanceIndex] < 0) {
					// found hole
					holeStart = distanceIndex - 1;

					// look for end of hole
					state = 2;
				}
				break;
			case 2:
				// Looking for end of the hole (next good travel time value)
				if (travelTimeArray[distanceIndex] > 0) {
					// start patching
					holeEnd = distanceIndex;

					// compute the travel time difference between the start
					// and the end of the hole
					double travelTimeDifference = travelTimeArray[holeEnd]
							- travelTimeArray[holeStart];

					// for each index in the hole
					for (int j = holeStart + 1; j < holeEnd; j++) {
						// we've found a hole
						nHoles++;

						double a = (travelTimeArray[holeEnd]
								- travelTimeArray[j]) / travelTimeDifference;
						double b = (travelTimeArray[j]
								- travelTimeArray[holeStart])
								/ travelTimeDifference;

						// compute new travel time for this index
						travelTimeArray[j] = a * travelTimeArray[holeStart]
								+ b * travelTimeArray[holeEnd];

						// compute new distance for this index
						depthDistanceArray[j] = a
								* depthDistanceArray[holeStart]
								+ b * depthDistanceArray[holeEnd];

						// compute new ray parameter for this index
						rayParameters[j] = a * rayParameters[holeStart]
								+ b * rayParameters[holeEnd];
					}

					// looking for another hole
					state = 1;
				}
				break;
		}
	}

	glass3::util::Logger::log(
			"debug",
			"CGenTrv::Row: Patched: " + std::to_string(nHoles) + " holes.");

	// cleanup
	if (rayParameters) {
		delete[] (rayParameters);
	}

	return (nHoles);
}

// ---------------------------------------------------------UnitTest
/* void CGenTrv::UnitTest(std::string phase, double delta, double depth, double
 tcorr) {
 // Calculate integrated times to compare against AK135
 // tables to ensure that there are no huge problems
 // in the accuracy of the traveltime branch calculations.
 // This test should probably be extended to be more comprehensive
 double travelTime = T(phase, delta, depth);
 printf("%s D:%.2f pDepthWarp:%.2f : %.2f - %.2f = %.2f\n", phase.c_str(),
 delta, depth, travelTime,
 tcorr, travelTime - tcorr);
 }
 */

// ---------------------------------------------------------T
bool CGenTrv::T(std::string phase, double delta, double depth) {
	// delta in degrees, depth in km
	// This routine uses reciprocity to calculate travel
	// times when source is above station

	// make sure distance not less than minimum
	if (fabs(delta) < 0.01) {
		delta = 0.01;
	}

	// get earth radius from earth model
	double earthRadius = pTerra->dEarthRadius;

	// init travel time and ray parameter
	double rayParam = 0.0;
	dTravelTime = -10.0;
	double travelTime = -10.0;

	// set up radius
	double radius = earthRadius;
	double sourceRadius = earthRadius - depth;

	// check to see if source is above earth radius
	if (sourceRadius > radius) {
		double rtmp = sourceRadius;
		sourceRadius = radius;
		radius = rtmp;
	}

	// init travel time ray parameters object
	pRay->setPhase(phase.c_str());
	pRay->setDepth(earthRadius - sourceRadius);
	pRay->setupRayParam();

	// get travel time and ray parameter
	travelTime = pRay->travel((DEG2RAD * delta), radius, &rayParam);

	// check for valid travel time
	if (travelTime < 0) {
		// no travel time
		return (false);
	}

	if ((dTravelTime < 0) || (travelTime < dTravelTime)) {
		// set up radius
		radius = earthRadius + 1;
		sourceRadius = earthRadius - depth;

		// check to see if source is above earth radius
		if (sourceRadius > radius) {
			double rtmp = sourceRadius;
			sourceRadius = radius;
			radius = rtmp;
		}

		// init travel time ray parameters object
		pRay->setDepth(earthRadius - sourceRadius);
		pRay->setupRayParam();

		// get travel time and ray parameter
		double tdif = pRay->travel((DEG2RAD * delta), radius);

		// check for valid travel time
		if (tdif < 0) {
			// no traveltime
			return (false);
		}

		// fill in values
		dTravelTime = travelTime;
		dDepthDistance = tdif - dTravelTime;
		rayParameter = rayParam;
		Phase = phase;

		// have a travel time
		return (true);
	}

	// no traveltime
	return (false);
}
}  // namespace traveltime

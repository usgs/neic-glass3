#include "TravelTime.h"
#include <geo.h>
#include <logger.h>
#include <string>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "TimeWarp.h"

namespace traveltime {

// ---------------------------------------------------------CTravelTime
CTravelTime::CTravelTime() {
	pDistanceWarp = NULL;
	pDepthWarp = NULL;
	pTravelTimeArray = NULL;
	pDepthDistanceArray = NULL;
	pPhaseArray = NULL;

	clear();
}

// ---------------------------------------------------------CTravelTime
CTravelTime::CTravelTime(const CTravelTime &travelTime) {
	pDistanceWarp = NULL;
	pDepthWarp = NULL;
	pTravelTimeArray = NULL;
	pDepthDistanceArray = NULL;
	pPhaseArray = NULL;

	clear();

	nDistanceWarp = travelTime.nDistanceWarp;
	nDepthWarp = travelTime.nDepthWarp;
	dDepth = travelTime.dDepth;
	dDelta = travelTime.dDelta;
	sPhase = travelTime.sPhase;

	// null check these?
	pDistanceWarp = new CTimeWarp(*travelTime.pDistanceWarp);
	pDepthWarp = new CTimeWarp(*travelTime.pDepthWarp);

	pTravelTimeArray = new double[nDistanceWarp * nDepthWarp];
	pDepthDistanceArray = new double[nDistanceWarp * nDepthWarp];
	pPhaseArray = new char[nDistanceWarp * nDepthWarp];

	for (int i = 0; i < (nDistanceWarp * nDepthWarp); i++) {
		pTravelTimeArray[i] = travelTime.pTravelTimeArray[i];
		pDepthDistanceArray[i] = travelTime.pDepthDistanceArray[i];
		pPhaseArray[i] = travelTime.pPhaseArray[i];
	}
}

// ---------------------------------------------------------~CTravelTime
CTravelTime::~CTravelTime() {
	clear();
}

// ---------------------------------------------------------clear
void CTravelTime::clear() {
	nDistanceWarp = 0;
	nDepthWarp = 0;
	dDepth = 0;
	dDelta = 0;

	if (pDistanceWarp) {
		delete (pDistanceWarp);
	}
	pDistanceWarp = NULL;

	if (pDepthWarp) {
		delete (pDepthWarp);
	}
	pDepthWarp = NULL;

	if (pTravelTimeArray) {
		delete (pTravelTimeArray);
	}
	pTravelTimeArray = NULL;

	if (pDepthDistanceArray) {
		delete (pDepthDistanceArray);
	}
	pDepthDistanceArray = NULL;

	if (pPhaseArray) {
		delete (pPhaseArray);
	}
	pPhaseArray = NULL;
}

// ---------------------------------------------------------Setup
bool CTravelTime::setup(std::string phase, std::string file) {
	// nullcheck
	if (phase == "") {
		glass3::util::Logger::log("error",
								"CTravelTime::Setup: empty phase provided");
		return (false);
	} else {
		sPhase = phase;
	}

	// generate file name if not specified
	if (file == "") {
		file = phase + ".trv";
	}

	glass3::util::Logger::log(
			"debug",
			"CTravelTime::Setup: phase:" + phase + " file:" + file);

	// open file
	FILE *inFile = fopen(file.c_str(), "rb");
	if (!inFile) {
		glass3::util::Logger::log("debug",
								"CTravelTime::Setup: Cannot open file:" + file);
		return (false);
	}

	// header
	// read file type
	char fileType[8];
	fread(fileType, 1, 4, inFile);
	fileType[4] = 0;

	// check file type
	if (strcmp(fileType, "TRAV") != 0) {
		glass3::util::Logger::log(
				"debug",
				"CTravelTime::Setup: File is not .trv file:" + file);

		fclose(inFile);

		return (false);
	}

	// read endian
	int16_t endianType;
	fread(&endianType, 1, 2, inFile);

	// read branch
	char branch[16];
	fread(branch, 1, 16, inFile);

	// read phase list
	char phaseList[64];
	fread(phaseList, 1, 64, inFile);

	// read distance warp
	double vlow = 0;
	double vhigh = 0;
	double alpha = 0;
	double bzero = 0;
	double binf = 0;
	nDistanceWarp = 0;
	fread(&nDistanceWarp, 1, 4, inFile);
	fread(&vlow, 1, 8, inFile);
	fread(&vhigh, 1, 8, inFile);
	fread(&alpha, 1, 8, inFile);
	fread(&bzero, 1, 8, inFile);
	fread(&binf, 1, 8, inFile);

	char sLog[1024];
	snprintf(sLog, sizeof(sLog),
				"CTravelTime::Setup: pDistanceWarp %d %.2f %.2f %.2f %.2f %.2f",
				nDistanceWarp, vlow, vhigh, alpha, bzero, binf);
	glass3::util::Logger::log(sLog);

	// create distance warp
	pDistanceWarp = new CTimeWarp(vlow, vhigh, alpha, bzero, binf);

	// read depth warp
	vlow = 0;
	vhigh = 0;
	alpha = 0;
	bzero = 0;
	binf = 0;
	nDepthWarp = 0;
	fread(&nDepthWarp, 1, 4, inFile);
	fread(&vlow, 1, 8, inFile);
	fread(&vhigh, 1, 8, inFile);
	fread(&alpha, 1, 8, inFile);
	fread(&bzero, 1, 8, inFile);
	fread(&binf, 1, 8, inFile);

	snprintf(sLog, sizeof(sLog),
				"CTravelTime::Setup: pDepthWarp %d %.2f %.2f %.2f %.2f %.2f",
				nDepthWarp, vlow, vhigh, alpha, bzero, binf);
	glass3::util::Logger::log(sLog);

	// create depth warp
	pDepthWarp = new CTimeWarp(vlow, vhigh, alpha, bzero, binf);

	// create interpolation grids
	pTravelTimeArray = new double[nDistanceWarp * nDepthWarp];
	pDepthDistanceArray = new double[nDistanceWarp * nDepthWarp];
	pPhaseArray = new char[nDistanceWarp * nDepthWarp];

	// read interpolation grids
	fread(pTravelTimeArray, 1, 8 * nDistanceWarp * nDepthWarp, inFile);
	fread(pDepthDistanceArray, 1, 8 * nDistanceWarp * nDepthWarp, inFile);
	fread(pPhaseArray, 1, nDistanceWarp * nDepthWarp, inFile);

	// done with file
	fclose(inFile);

	return (true);
}

// ---------------------------------------------------------setOrigin
void CTravelTime::setOrigin(double lat, double lon, double depth) {
	geoOrg.setGeographic(lat, lon, EARTHRADIUSKM - depth);
	dDepth = depth;
}

// ---------------------------------------------------------setOrigin
void CTravelTime::setOrigin(const glass3::util::Geo &geoOrigin) {
	geoOrg = geoOrigin;
	// ditch dDepth in favor or
	dDepth = EARTHRADIUSKM - geoOrg.m_dGeocentricRadius;
}

// ---------------------------------------------------------T
double CTravelTime::T(glass3::util::Geo *geo) {
	// Calculate travel time given CGeo
	dDelta = RAD2DEG * geoOrg.delta(geo);

	return (T(dDelta));
}

// ---------------------------------------------------------T

double CTravelTime::T(double delta) {
	// Calculate travel time given delta in degrees
	double depth = pDepthWarp->grid(dDepth);
	double distance = pDistanceWarp->grid(delta);

	// compute travel time using bilinear interpolation
	double travelTime = bilinear(distance, depth);
	dDelta = delta;

	return (travelTime);
}

// ---------------------------------------------------------T
double CTravelTime::T(int deltaIndex, int depthIndex) {
	// bounds checks
	if ((deltaIndex < 0) || (deltaIndex >= nDistanceWarp)) {
		return (-1.0);
	}
	if ((depthIndex < 0) || (depthIndex >= nDepthWarp)) {
		return (-1.0);
	}

	// get traveltime from travel time array
	double travelTime =
			pTravelTimeArray[depthIndex * nDistanceWarp + deltaIndex];

	return (travelTime);
}

// ---------------------------------------------------------Bilinear
double CTravelTime::bilinear(double distance, double depth) {
	double interpolationGrid[2][2];
	double travelTime;
	int startingDelta = static_cast<int>(distance);
	int startingDepth = static_cast<int>(depth);
	bool error = false;

	// generate interpolation grid
	for (int i = 0; i < 2; i++) {
		// calculate distance index
		int deltaIndex = startingDelta + i;

		for (int j = 0; j < 2; j++) {
			// calculate depth index
			int depthIndex = startingDepth + j;

			// get current travel time from travel time array
			double time = T(deltaIndex, depthIndex);

			// check current travel time
			if (time < 0.0) {
				error = true;
			}

			// store travel time in interpolation grid
			interpolationGrid[i][j] = time;
		}

		double s = distance - floor(distance);
		double t = depth - floor(depth);

		// compute overall travel time by interpolating grid
		travelTime = interpolationGrid[0][0] * (1.0f - s) * (1.0f - t)
				+ interpolationGrid[0][1] * (1.0f - s) * t
				+ interpolationGrid[1][0] * s * (1.0f - t)
				+ interpolationGrid[1][1] * s * t;
	}

	// check if we had errors
	if (error) {
		// no traveltime
		return (-1.0);
	}

	return (travelTime);
}
}  // namespace traveltime

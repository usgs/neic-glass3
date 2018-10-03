#include "TravelTime.h"
#include <geo.h>
#include <logger.h>
#include <string>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "TimeWarp.h"

namespace traveltime {

// constants
constexpr double CTravelTime::k_dTravelTimeInvalid;

// ---------------------------------------------------------CTravelTime
CTravelTime::CTravelTime() {
	m_pDistanceWarp = NULL;
	m_pDepthWarp = NULL;
	m_pTravelTimeArray = NULL;
	m_pDepthDistanceArray = NULL;
	m_pPhaseArray = NULL;

	clear();
}

// ---------------------------------------------------------CTravelTime
CTravelTime::CTravelTime(const CTravelTime &travelTime) {
	m_pDistanceWarp = NULL;
	m_pDepthWarp = NULL;
	m_pTravelTimeArray = NULL;
	m_pDepthDistanceArray = NULL;
	m_pPhaseArray = NULL;

	clear();

	m_iNumDistanceWarp = travelTime.m_iNumDistanceWarp;
	m_iNumDepthWarp = travelTime.m_iNumDepthWarp;
	m_dDepth = travelTime.m_dDepth;
	m_dDelta = travelTime.m_dDelta;
	m_sPhase = travelTime.m_sPhase;

	// null check these?
	m_pDistanceWarp = new CTimeWarp(*travelTime.m_pDistanceWarp);
	m_pDepthWarp = new CTimeWarp(*travelTime.m_pDepthWarp);

	m_pTravelTimeArray = new double[m_iNumDistanceWarp * m_iNumDepthWarp];
	m_pDepthDistanceArray = new double[m_iNumDistanceWarp * m_iNumDepthWarp];
	m_pPhaseArray = new char[m_iNumDistanceWarp * m_iNumDepthWarp];

	for (int i = 0; i < (m_iNumDistanceWarp * m_iNumDepthWarp); i++) {
		m_pTravelTimeArray[i] = travelTime.m_pTravelTimeArray[i];
		m_pDepthDistanceArray[i] = travelTime.m_pDepthDistanceArray[i];
		m_pPhaseArray[i] = travelTime.m_pPhaseArray[i];
	}
}

// ---------------------------------------------------------~CTravelTime
CTravelTime::~CTravelTime() {
	clear();
}

// ---------------------------------------------------------clear
void CTravelTime::clear() {
	m_iNumDistanceWarp = 0;
	m_iNumDepthWarp = 0;
	m_dDepth = 0;
	m_dDelta = 0;

	if (m_pDistanceWarp) {
		delete (m_pDistanceWarp);
	}
	m_pDistanceWarp = NULL;

	if (m_pDepthWarp) {
		delete (m_pDepthWarp);
	}
	m_pDepthWarp = NULL;

	if (m_pTravelTimeArray) {
		delete (m_pTravelTimeArray);
	}
	m_pTravelTimeArray = NULL;

	if (m_pDepthDistanceArray) {
		delete (m_pDepthDistanceArray);
	}
	m_pDepthDistanceArray = NULL;

	if (m_pPhaseArray) {
		delete (m_pPhaseArray);
	}
	m_pPhaseArray = NULL;
}

// ---------------------------------------------------------Setup
bool CTravelTime::setup(std::string phase, std::string file) {
	// nullcheck
	if (phase == "") {
		glass3::util::Logger::log("error",
									"CTravelTime::Setup: empty phase provided");
		return (false);
	} else {
		m_sPhase = phase;
	}

	// generate file name if not specified
	if (file == "") {
		file = phase + ".trv";
	}

	glass3::util::Logger::log(
			"debug", "CTravelTime::Setup: phase:" + phase + " file:" + file);

	// open file
	FILE *inFile = fopen(file.c_str(), "rb");
	if (!inFile) {
		glass3::util::Logger::log(
				"debug", "CTravelTime::Setup: Cannot open file:" + file);
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
				"debug", "CTravelTime::Setup: File is not .trv file:" + file);

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
	m_iNumDistanceWarp = 0;
	fread(&m_iNumDistanceWarp, 1, 4, inFile);
	fread(&vlow, 1, 8, inFile);
	fread(&vhigh, 1, 8, inFile);
	fread(&alpha, 1, 8, inFile);
	fread(&bzero, 1, 8, inFile);
	fread(&binf, 1, 8, inFile);

	char sLog[1024];
	snprintf(sLog, sizeof(sLog),
				"CTravelTime::Setup: pDistanceWarp %d %.2f %.2f %.2f %.2f %.2f",
				m_iNumDistanceWarp, vlow, vhigh, alpha, bzero, binf);
	glass3::util::Logger::log(sLog);

	// create distance warp
	m_pDistanceWarp = new CTimeWarp(vlow, vhigh, alpha, bzero, binf);

	// read depth warp
	vlow = 0;
	vhigh = 0;
	alpha = 0;
	bzero = 0;
	binf = 0;
	m_iNumDepthWarp = 0;
	fread(&m_iNumDepthWarp, 1, 4, inFile);
	fread(&vlow, 1, 8, inFile);
	fread(&vhigh, 1, 8, inFile);
	fread(&alpha, 1, 8, inFile);
	fread(&bzero, 1, 8, inFile);
	fread(&binf, 1, 8, inFile);

	snprintf(sLog, sizeof(sLog),
				"CTravelTime::Setup: pDepthWarp %d %.2f %.2f %.2f %.2f %.2f",
				m_iNumDepthWarp, vlow, vhigh, alpha, bzero, binf);
	glass3::util::Logger::log(sLog);

	// create depth warp
	m_pDepthWarp = new CTimeWarp(vlow, vhigh, alpha, bzero, binf);

	// create interpolation grids
	m_pTravelTimeArray = new double[m_iNumDistanceWarp * m_iNumDepthWarp];
	m_pDepthDistanceArray = new double[m_iNumDistanceWarp * m_iNumDepthWarp];
	m_pPhaseArray = new char[m_iNumDistanceWarp * m_iNumDepthWarp];

	// read interpolation grids
	fread(m_pTravelTimeArray, 1, 8 * m_iNumDistanceWarp * m_iNumDepthWarp,
			inFile);
	fread(m_pDepthDistanceArray, 1, 8 * m_iNumDistanceWarp * m_iNumDepthWarp,
			inFile);
	fread(m_pPhaseArray, 1, m_iNumDistanceWarp * m_iNumDepthWarp, inFile);

	// done with file
	fclose(inFile);

	return (true);
}

// ---------------------------------------------------------setOrigin
void CTravelTime::setTTOrigin(double lat, double lon, double depth) {
	m_geoTTOrigin.setGeographic(lat, lon,
								glass3::util::Geo::k_EarthRadiusKm - depth);
	m_dDepth = depth;
}

// ---------------------------------------------------------setOrigin
void CTravelTime::setTTOrigin(const glass3::util::Geo &geoOrigin) {
	m_geoTTOrigin = geoOrigin;
	// ditch dDepth in favor or
	m_dDepth = glass3::util::Geo::k_EarthRadiusKm
			- m_geoTTOrigin.m_dGeocentricRadius;
}

// ---------------------------------------------------------T
double CTravelTime::T(glass3::util::Geo *geo) {
	// Calculate travel time given CGeo
	m_dDelta = glass3::util::GlassMath::k_RadiansToDegrees
			* m_geoTTOrigin.delta(geo);

	return (T(m_dDelta));
}

// ---------------------------------------------------------T

double CTravelTime::T(double delta) {
	// Calculate travel time given delta in degrees
	double depth = m_pDepthWarp->calculateGridPoint(m_dDepth);
	double distance = m_pDistanceWarp->calculateGridPoint(delta);

	// compute travel time using bilinear interpolation
	double travelTime = bilinear(distance, depth);
	m_dDelta = delta;

	return (travelTime);
}

// ---------------------------------------------------------T
double CTravelTime::T(int deltaIndex, int depthIndex) {
	// bounds checks
	if ((deltaIndex < 0) || (deltaIndex >= m_iNumDistanceWarp)) {
		return (k_dTravelTimeInvalid);
	}
	if ((depthIndex < 0) || (depthIndex >= m_iNumDepthWarp)) {
		return (k_dTravelTimeInvalid);
	}

	// get traveltime from travel time array
	double travelTime = m_pTravelTimeArray[depthIndex * m_iNumDistanceWarp
			+ deltaIndex];

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
		return (k_dTravelTimeInvalid);
	}

	return (travelTime);
}
}  // namespace traveltime

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
const std::string CTravelTime::k_dPhaseInvalid = ""; // NOLINT

// ---------------------------------------------------------CTravelTime
CTravelTime::CTravelTime() {
	m_pTravelTimeArray = NULL;
	m_pDepthDistanceArray = NULL;
	m_pPhaseArray = NULL;

	clear();
}

// ---------------------------------------------------------CTravelTime
CTravelTime::CTravelTime(const CTravelTime &travelTime) {
	m_pTravelTimeArray = NULL;
	m_pDepthDistanceArray = NULL;
	m_pPhaseArray = NULL;

	clear();

	m_iNumDistances = travelTime.m_iNumDistances;
	m_dMinimumDistance = travelTime.m_dMinimumDistance;
	m_dMaximumDistance = travelTime.m_dMaximumDistance;

	m_iNumDepths = travelTime.m_iNumDepths;
	m_dMinimumDepth = travelTime.m_dMinimumDepth;
	m_dMaximumDepth = travelTime.m_dMaximumDepth;

	m_dDepth = travelTime.m_dDepth;
	m_dDelta = travelTime.m_dDelta;
	m_sPhase = travelTime.m_sPhase;

	// null check these?
	m_pTravelTimeArray = new double[m_iNumDistances * m_iNumDepths];
	m_pDepthDistanceArray = new double[m_iNumDistances * m_iNumDepths];
	m_pPhaseArray = new char[m_iNumDistances * m_iNumDepths];

	for (int i = 0; i < (m_iNumDistances * m_iNumDepths); i++) {
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
	m_iNumDistances = 0;
	m_dMinimumDistance = 0;
	m_dMaximumDistance = 0;

	m_iNumDepths = 0;
	m_dMinimumDepth = 0;
	m_dMaximumDepth = 0;

	m_dDepth = 0;
	m_dDelta = 0;
	m_sPhase = CTravelTime::k_dPhaseInvalid;

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
	if (phase == CTravelTime::k_dPhaseInvalid) {
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

	// read num distances
	m_iNumDistances = 0;
	fread(&m_iNumDistances, sizeof(m_iNumDistances), 1, inFile);

	// read the minimum distance
	float minDist = 0;
	fread(&minDist, sizeof(minDist), 1, inFile);
	m_dMinimumDistance = static_cast<double>(minDist);

	// read the maximum distance
	float maxDist = 0;
	fread(&maxDist, sizeof(maxDist), 1, inFile);
	m_dMaximumDistance = static_cast<double>(maxDist);

	// read num depths
	m_iNumDepths = 0;
	fread(&m_iNumDepths, sizeof(m_iNumDepths), 1, inFile);

	// read the minimum depth
	float minDepth = 0;
	fread(&minDepth, sizeof(minDepth), 1, inFile);
	m_dMinimumDepth = static_cast<double>(minDepth);

	// read the maximum depth
	float maxDepth = 0;
	fread(&maxDepth, sizeof(maxDepth), 1, inFile);
	m_dMaximumDepth = static_cast<double>(maxDepth);

	// create interpolation grids
	m_pTravelTimeArray = new double[m_iNumDistances * m_iNumDepths];
	m_pDepthDistanceArray = new double[m_iNumDistances * m_iNumDepths];
	m_pPhaseArray = new char[m_iNumDistances * m_iNumDepths];

	// read interpolation grids
	fread(m_pTravelTimeArray, 1, 8 * m_iNumDistances * m_iNumDepths,
			inFile);
	fread(m_pDepthDistanceArray, 1, 8 * m_iNumDistances * m_iNumDepths,
			inFile);
	fread(m_pPhaseArray, 1, m_iNumDistances * m_iNumDepths, inFile);

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
	// bounds checks
	if(delta < m_dMinimumDistance || delta > m_dMaximumDistance) {
    return (k_dTravelTimeInvalid);
	}
	if(m_dDepth < m_dMinimumDepth || m_dDepth > m_dMaximumDepth) {
    return (k_dTravelTimeInvalid);
	}

	m_dDelta = delta;

	// compute distance and depth points
	double depthStep = (m_dMaximumDepth - m_dMinimumDepth) /
		static_cast<double>(m_iNumDepths);
	double depthPoint = floor((m_dDepth / depthStep) - 1);

	double distanceStep = (m_dMaximumDistance - m_dMinimumDistance) /
		static_cast<double>(m_iNumDistances);
	double distancePoint = floor((delta / distanceStep) - 1);

	// compute travel time using bilinear interpolation
	double travelTime = bilinear(distancePoint, depthPoint);

	return (travelTime);
}

// ---------------------------------------------------------T
double CTravelTime::T(int deltaIndex, int depthIndex) {
	// bounds checks
	if ((deltaIndex < 0) || (deltaIndex >= m_iNumDistances)) {
		return (k_dTravelTimeInvalid);
	}
	if ((depthIndex < 0) || (depthIndex >= m_iNumDepths)) {
		return (k_dTravelTimeInvalid);
	}

	// get traveltime from travel time array
	double travelTime = m_pTravelTimeArray[depthIndex * m_iNumDistances
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

#include "TravelTime.h"
#include <geo.h>
#include <logger.h>
#include <string>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace traveltime {

// constants
constexpr double CTravelTime::k_dTravelTimeInvalid;
const std::string CTravelTime::k_dPhaseInvalid = ""; // NOLINT

// ---------------------------------------------------------CTravelTime
CTravelTime::CTravelTime() {
	m_pTravelTimeArray = NULL;

	clear();
}

// ---------------------------------------------------------CTravelTime
CTravelTime::CTravelTime(const CTravelTime &travelTime) {
	m_pTravelTimeArray = NULL;

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

	// null check?
	m_pTravelTimeArray = new double[m_iNumDistances * m_iNumDepths];

	for (int i = 0; i < (m_iNumDistances * m_iNumDepths); i++) {
		m_pTravelTimeArray[i] = travelTime.m_pTravelTimeArray[i];
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
	char fileType[5];
	fread(fileType, sizeof(char), 5, inFile);

	// check file type
	if (strcmp(fileType, "TRAV") != 0) {
		glass3::util::Logger::log(
				"debug", "CTravelTime::Setup: File is not .trv file:" + file);

		fclose(inFile);

		return (false);
	}

	// read branch
	char branch[17];
	fread(branch, sizeof(char), 17, inFile);

	// read phase list
	char phaseList[65];
	fread(phaseList, sizeof(char), 65, inFile);

	// read num distances
	m_iNumDistances = 0;
	fread(&m_iNumDistances, sizeof(int), 1, inFile);

	// read the minimum distance
	m_dMinimumDistance = 0;
	fread(&m_dMinimumDistance, sizeof(double), 1, inFile);

	// read the maximum distance
	m_dMaximumDistance = 0;
	fread(&m_dMaximumDistance, sizeof(double), 1, inFile);

	// read num depths
	m_iNumDepths = 0;
	fread(&m_iNumDepths, sizeof(int), 1, inFile);

	// read the minimum depth
	m_dMinimumDepth = 0;
	fread(&m_dMinimumDepth, sizeof(double), 1, inFile);
	// read the maximum depth
	m_dMaximumDepth = 0;
	fread(&m_dMaximumDepth, sizeof(double), 1, inFile);

	if ((m_iNumDistances <= 0) || (m_iNumDepths <= 0)) {
		glass3::util::Logger::log("error",
											"CTravelTime::Setup: Invalid data read from input file");
		fclose(inFile);
		return (false);
	}

	// create interpolation array
	m_pTravelTimeArray = new double[m_iNumDistances * m_iNumDepths];

	// read interpolation array
	fread(m_pTravelTimeArray, 1, sizeof(double) * m_iNumDistances * m_iNumDepths,
			inFile);

	// done with file
	fclose(inFile);

	glass3::util::Logger::log(
		"debug",
		"CTravelTime::Setup: Read: Branch Name |" + std::string(branch)
			+ "| Phase List |" + std::string(phaseList)
			+ "| Num Dist: " + std::to_string(m_iNumDistances)
			+ ", Min Dist: " + std::to_string(m_dMinimumDistance)
			+ ", Max Dist: " + std::to_string(m_dMaximumDistance)
			+ ", Num Depth: " + std::to_string(m_iNumDepths)
			+ ", Min Depth: " + std::to_string(m_dMinimumDepth)
			+ ", Max Depth: " + std::to_string(m_dMaximumDepth));

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

	// compute distance and depth interpolation points
	double depthStep = (m_dMaximumDepth - m_dMinimumDepth) /
		static_cast<double>(m_iNumDepths);
	double depthIndex = floor((m_dDepth / depthStep));

	// bounds check
	if (depthIndex < 0) {
		depthIndex = 0;
	} else if (depthIndex > (m_iNumDepths - 1)) {
		depthIndex = m_iNumDepths - 1;
	}

	/*printf("m_dDepth: %f, ", m_dDepth);
	printf("m_dMinimumDepth: %f, ", m_dMinimumDepth);
	printf("m_dMaximumDepth: %f, ", m_dMaximumDepth);
	printf("m_iNumDepths: %d, ", m_iNumDepths);
	printf("depthStep: %f, ", depthStep);
	printf("depthIndex: %f, ", depthIndex);*/

	double distanceStep = (m_dMaximumDistance - m_dMinimumDistance)
		/ static_cast<double>(m_iNumDistances);
	double distanceIndex = floor((m_dDelta / distanceStep));

	// bounds check
	if (distanceIndex < 0) {
		distanceIndex = 0;
	} else if (distanceIndex > (m_iNumDistances - 1)) {
		distanceIndex = m_iNumDistances - 1;
	}

	/*printf("m_dDelta: %f, ", m_dDelta);
	printf("m_dMinimumDistance: %f, ", m_dMinimumDistance);
	printf("m_dMaximumDistance: %f, ", m_dMaximumDistance);
	printf("m_iNumDistances: %d, ", m_iNumDistances);
	printf("distanceStep: %f, ", distanceStep);
	printf("distanceIndex: %f, ", distanceIndex);*/

	// compute travel time using bilinear interpolation
	double travelTime = bilinear(distanceIndex, depthIndex);
	// printf("travelTime: %f\n", travelTime);

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

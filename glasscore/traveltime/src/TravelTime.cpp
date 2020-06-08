#include "TravelTime.h"
#include <geo.h>
#include <logger.h>
#include <string>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>

namespace traveltime {

// constants
constexpr double CTravelTime::k_dTravelTimeInvalid;
const std::string CTravelTime::k_dPhaseInvalid = ""; // NOLINT

// ---------------------------------------------------------CTravelTime
CTravelTime::CTravelTime(bool useForLocations, bool publishable) {
	m_pTravelTimeArray = NULL;

	clear();

	m_bUseForLocations = useForLocations;
	m_bPublishable = publishable;
}

// ---------------------------------------------------------CTravelTime
CTravelTime::CTravelTime(const CTravelTime &travelTime) {
	m_pTravelTimeArray = NULL;

	clear();

	m_iNumDistances = travelTime.m_iNumDistances;
	m_dMinimumDistance = travelTime.m_dMinimumDistance;
	m_dMaximumDistance = travelTime.m_dMaximumDistance;
	m_dDistanceStep = travelTime.m_dDistanceStep;

	m_iNumDepths = travelTime.m_iNumDepths;
	m_dMinimumDepth = travelTime.m_dMinimumDepth;
	m_dMaximumDepth = travelTime.m_dMaximumDepth;
	m_dDepthStep = travelTime.m_dDepthStep;

	m_dDepth = travelTime.m_dDepth;
	m_dDelta = travelTime.m_dDelta;
	m_sPhase = travelTime.m_sPhase;

	m_bUseForLocations = travelTime.m_bUseForLocations;
	m_bPublishable = travelTime.m_bPublishable;

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
	m_dDistanceStep = 0;

	m_iNumDepths = 0;
	m_dMinimumDepth = 0;
	m_dMaximumDepth = 0;
	m_dDepthStep = 0;

	m_dDepth = 0;
	m_dDelta = 0;
	m_sPhase = CTravelTime::k_dPhaseInvalid;

	m_bUseForLocations = true;
	m_bPublishable = true;

	if (m_pTravelTimeArray) {
		delete (m_pTravelTimeArray);
	}
	m_pTravelTimeArray = NULL;
}

// -----------------------------------------------------writeToFile
void CTravelTime::writeToFile(std::string fileName, double depth) {
	if (fileName == "") {
		return;
	}

	// bounds check depth
	if (depth < m_dMinimumDepth) {
		return;
	} else if (depth > m_dMaximumDepth) {
		return;
	}

	glass3::util::Logger::log("info",
									"CTravelTime::writeToFile: writing: " + fileName);

	// set the depth
	m_dDepth = depth;

	// compute distance step
	double distanceStep = (m_dMaximumDistance - m_dMinimumDistance)
		/ static_cast<double>(m_iNumDistances);

	// init to minimum distance
	double aDistance = m_dMinimumDistance;

	glass3::util::Logger::log("info",
									"CTravelTime::writeToFile: Depth: " + std::to_string(depth)
									+ " NumDist: " + std::to_string(m_iNumDistances)
									+ " MinDist: " + std::to_string(m_dMinimumDistance)
									+ " MaxDist: " + std::to_string(m_dMaximumDistance)
									+ " StartDist: " + std::to_string(aDistance));

	int count = 0;
	std::string fileString = "Distance,Time\n";
	for (int i = 0; i < m_iNumDistances; i++) {
		// get the travel time for this distance
		double aTime = T(aDistance);

		// make sure we got a valid time
		if (aTime > 0) {
			count++;
			// add this distance/time to the file string
			// as csv
			fileString += std::to_string(aDistance) + ","
				+ std::to_string(aTime) + "\n";
		}

		// update to the next distance
		aDistance += distanceStep;
	}

	glass3::util::Logger::log("info",
									"CTravelTime::writeToFile: Generated " + std::to_string(count)
									+ " points. ");

	// now, write the file to disk
	std::ofstream outfile;
	outfile.open(fileName, std::ios::out);
	outfile << fileString;

	// done
	outfile.close();
}

// ---------------------------------------------------------setup
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

	// A travel time file is a binary file with a header and the travel time
	//     interpolation array
	// The file consists of the following:
	// <SOF>
	// <FileType> - 4 ascii characters plus null termination - 5 bytes
	// <BranchName> - 16 ascii characters plus null termination - 17 bytes
	// <PhaseList> - 64 ascii characters plus null termination - 65 bytes
	// <numberOfDistancePoints> - 1 int value, 4 bytes
	// <minimumDistance> - 1 double value, 8 bytes
	// <maximumDistance> - 1 double value, 8 bytes
	// <numberOfDepthPoints> - 1 int value, 4 bytes
	// <minimumDepth> - 1 double value, 8 bytes
	// <maximumDepth> - 1 double value, 8 bytes
	// <travelTimeInterpolationArray> -
	//    (numberOfDistancePoints * numberOfDepthPoints) double values,
	//    (numberOfDistancePoints * numberOfDepthPoints) * 8 bytes
	// <EOF>

	// header
	// read <FileType>
	char fileType[5];
	fread(fileType, sizeof(char), 5, inFile);

	// check file type
	if (strcmp(fileType, "TRAV") != 0) {
		glass3::util::Logger::log(
				"debug", "CTravelTime::Setup: File is not .trv file:" + file);

		fclose(inFile);

		return (false);
	}

	// read <BranchName>
	char branch[17];
	fread(branch, sizeof(char), 17, inFile);

	// read <PhaseList>
	char phaseList[65];
	fread(phaseList, sizeof(char), 65, inFile);

	// read <numberOfDistancePoints>
	m_iNumDistances = 0;
	fread(&m_iNumDistances, sizeof(int), 1, inFile);

	// read <minimumDistance>
	m_dMinimumDistance = 0;
	fread(&m_dMinimumDistance, sizeof(double), 1, inFile);

	// read <maximumDistance>
	m_dMaximumDistance = 0;
	fread(&m_dMaximumDistance, sizeof(double), 1, inFile);

	// read <numberOfDepthPoints>
	m_iNumDepths = 0;
	fread(&m_iNumDepths, sizeof(int), 1, inFile);

	// read <minimumDepth>
	m_dMinimumDepth = 0;
	fread(&m_dMinimumDepth, sizeof(double), 1, inFile);

	// read <maximumDepth>
	m_dMaximumDepth = 0;
	fread(&m_dMaximumDepth, sizeof(double), 1, inFile);

	// check for valid data
	if ((m_iNumDistances <= 0) || (m_iNumDepths <= 0)) {
		glass3::util::Logger::log("error",
											"CTravelTime::Setup: Invalid data read from input file");
		fclose(inFile);
		return (false);
	}

	// allocate travel time interpolation array
	m_pTravelTimeArray = new double[m_iNumDistances * m_iNumDepths];

	// read <travelTimeInterpolationArray>
	fread(m_pTravelTimeArray, 1, sizeof(double) * m_iNumDistances * m_iNumDepths,
			inFile);

	// done with file
	fclose(inFile);

	// compute steps
	m_dDistanceStep = (m_dMaximumDistance - m_dMinimumDistance)
		/ static_cast<double>(m_iNumDistances);
	m_dDepthStep = (m_dMaximumDepth - m_dMinimumDepth) /
		static_cast<double>(m_iNumDepths);

	glass3::util::Logger::log(
		"debug",
		"CTravelTime::Setup: Read: Branch Name |" + std::string(branch)
			+ "| Phase List |" + std::string(phaseList)
			+ "| Num Dist: " + std::to_string(m_iNumDistances)
			+ ", Min Dist: " + std::to_string(m_dMinimumDistance)
			+ ", Max Dist: " + std::to_string(m_dMaximumDistance)
			+ ", Dist Step: " + std::to_string(m_dDistanceStep)
			+ ", Num Depth: " + std::to_string(m_iNumDepths)
			+ ", Min Depth: " + std::to_string(m_dMinimumDepth)
			+ ", Max Depth: " + std::to_string(m_dMaximumDepth)
			+ ", Depth Step: " + std::to_string(m_dDepthStep));

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
	m_dDelta = delta;

	// bounds checks
	if((m_dDelta < m_dMinimumDistance) || (m_dDelta > m_dMaximumDistance)) {
    return (k_dTravelTimeInvalid);
	}
	if((m_dDepth < m_dMinimumDepth) || (m_dDepth > m_dMaximumDepth)) {
    return (k_dTravelTimeInvalid);
	}

	double inDistance = m_dDelta;
	double inDepth = m_dDepth;

	// calculate distance interpolation indexes and values
	int distanceIndex1 = getIndexFromDistance(inDistance);
	double distance1 = getDistanceFromIndex(distanceIndex1);
	int distanceIndex2 = distanceIndex1 + 1;
	double distance2 = getDistanceFromIndex(distanceIndex2);

	// calculate depth interpolation indexes and values
	int depthIndex1 = getIndexFromDepth(inDepth);
	double depth1 = getDepthFromIndex(depthIndex1);
	int depthIndex2 = depthIndex1 + 1;
	double depth2 = getDepthFromIndex(depthIndex2);

	// lookup travel time interpolation values from using the indexes
	double travelTime11 = T(distanceIndex1, depthIndex1);
	double travelTime12 = T(distanceIndex1, depthIndex2);
	double travelTime21 = T(distanceIndex2, depthIndex1);
	double travelTime22 = T(distanceIndex2, depthIndex2);

	// check travel time interpolation values
	if ((travelTime11 < 0) || (travelTime12 < 0)
		|| (travelTime21 < 0) || (travelTime22 < 0)) {
		// no traveltime
		return (k_dTravelTimeInvalid);
	}

	// get traveltime via bilinear interpolation using the values and
	// input distance/depth
	double outTravelTime = bilinearInterpolation(
		travelTime11, travelTime12, travelTime21, travelTime22,
		distance1, depth1, distance2, depth2,
		inDistance, inDepth);

	// check final travel time
	if (outTravelTime < 0) {
		// no traveltime
		return (k_dTravelTimeInvalid);
	}

	return (outTravelTime);
}

// ------------------------------------------------------getIndexFromDistance
int CTravelTime::getIndexFromDistance(double distance) {
	if (m_dDistanceStep < 0) {
		return (0);
	}

	// we need to convert from actual distance in degrees to an index point within
	// the distance range specified for this branch's traveltime interpolation
	// array so that the travel time can be computed using bilinear interpolation
	// of the travel time array using distance and distance index points.

	// we divide the integer equivelent of the actual value (using floor())
	// minus the start of the range by the actual step size
	int distanceIndex = static_cast<int>(floor(((distance - m_dMinimumDistance)
		/ m_dDistanceStep)));

	// bounds checks
	if (distanceIndex < 0) {
		return(0);
	} else if (distanceIndex > m_iNumDistances) {
		return(m_iNumDistances - 1);
	} else {
		return (distanceIndex);
	}
}

// ------------------------------------------------------getDistanceFromIndex
double CTravelTime::getDistanceFromIndex(int index) {
	if (m_dDistanceStep < 0) {
		return (0);
	}
	if (index < 0) {
		return (0);
	}

	double distance = (index * m_dDistanceStep) + m_dMinimumDistance;

	return distance;
}

// ------------------------------------------------------getIndexFromDepth
int CTravelTime::getIndexFromDepth(double depth) {
	if (m_dDepthStep < 0) {
		return (0);
	}

	// we need to convert from actual depth in kilometers to an index point within
	// the depth range specified for this branch's traveltime interpolation array
	// so that the travel time can be computed using bilinear interpolation of the
	// travel time array using distance and depth index points.

	// we divide the integer equivelent of the actual value (using floor())
	// minus the start of the range by the actual step size
	int depthIndex = static_cast<int>(floor(((depth - m_dMinimumDepth)
		/ m_dDepthStep)));

	// bounds checks
	if (depthIndex < 0) {
		return(0);
	} else if (depthIndex > m_iNumDepths) {
		return(m_iNumDepths - 1);
	} else {
		return (depthIndex);
	}
}

// ------------------------------------------------------getDepthFromIndex
double CTravelTime::getDepthFromIndex(int index) {
	if (m_dDepthStep < 0) {
		return (0);
	}
	if (index < 0) {
		return (0);
	}

	double depth = (index * m_dDepthStep) + m_dMinimumDepth;

	return(depth);
}

// -------------------------------------------------------bilinearInterpolation
double CTravelTime::bilinearInterpolation(double q_x1y1, double q_x1y2,
	double q_x2y1, double q_x2y2, double x1, double y1, double x2, double y2,
	double x, double y) {
	// check values to avoid div by 0
	if ((x1 == x2) || (y1 == y2)) {
		return(-1.0);
	}

	double x2x1 = x2 - x1;
	double y2y1 = y2 - y1;
	double x2x = x2 - x;
	double y2y = y2 - y;
	double yy1 = y - y1;
	double xx1 = x - x1;

	return 1.0 / (x2x1 * y2y1) * (
			q_x1y1 * x2x * y2y +
			q_x2y1 * xx1 * y2y +
			q_x1y2 * x2x * yy1 +
			q_x2y2 * xx1 * yy1);
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
}  // namespace traveltime

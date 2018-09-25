/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef TRAVELTIME_H
#define TRAVELTIME_H

#include <json.h>
#include <geo.h>
#include <memory>
#include <vector>
#include <string>

/**
 * \namespace traveltime
 * \brief traveltime namespace phase travel time classes and functions
 *
 * The traveltime namespace contains various classes and functions used
 * in the generation, calculation, and lookup of seismic phase travel
 * times and expected distances.
 */
namespace traveltime {

// forward declarations
class CRay;
class CTimeWarp;

/**
 * \brief travel time phase class
 *
 * The traveltime CTravelTime class is a class that loads,
 * parses, and generates travel time phase branch over a set
 * of valid depths for a given phase or phase class.
 * CTravelTime supports calculating travel times based on distance
 * or geographic location for the given phase or phase class.
 */
class CTravelTime {
 public:
	/**
	 * \brief CTravelTime constructor
	 *
	 * The constructor for the CTravelTime class.
	 */
	CTravelTime();

	/**
	 * \brief CTravelTime copy constructor
	 *
	 * The copy constructor for the CTravelTime class.
	 */
	CTravelTime(const CTravelTime &travelTime);

	/**
	 * \brief CTravelTime destructor
	 *
	 * The destructor for the CTravelTime class.
	 */
	~CTravelTime();

	/**
	 * \brief Load or generate branch data
	 *
	 * Attempts to load branch data (using Load()) for a phase using a given
	 * CRay and phase std::string.
	 *
	 * \param phase - A std::std::string representing the phase to use, default
	 * is "P"
	 * \param file - A std::std::string representing the file to load, default
	 * is ""
	 */
	bool setup(std::string phase = "P", std::string file = "");

	/**
	 * \brief CTravelTime clear function
	 */
	void clear();

	/**
	 * \brief Set current geographic location
	 *
	 * Set the current geographic location to the provided latitude,
	 * longitude, and depth.  This will set pTrv1, pTrv2, dA, and dB
	 * ephemeral values used for travel time calculations.
	 *
	 * \param lat - A double value representing the latitude of the
	 * desired geographic location.
	 * \param lon - A double value representing the longitude of the
	 * desired geographic location.
	 * \param depth - A double value representing the depth of the
	 * desired geographic location.
	 */
	void setOrigin(double lat, double lon, double depth);

	/**
	 * \brief Set current geographic location
	 *
	 * Set the current geographic location using the provided CGeo.
	 * This will set the source location used for source/receiver
	 * traveltime calculations.
	 *
	 * \param geoOrigin - A CGeo representing the lat/lon and surface depth
	 * (or elev) of the source, along with already computed concentric lat/lon
	 * and vector coordinates.
	 */
	void setOrigin(const glass3::util::Geo &geoOrigin);

	/**
	 * \brief Calculate travel time in seconds
	 *
	 * Calculate travel time in seconds given geographic location
	 *
	 * \param geo - A pointer to a glass3::util::Geo object representing the
	 * location to calculate the travel time from
	 * \return Returns the travel time in seconds, or -1.0 if there is
	 * no valid travel time
	 */
	double T(glass3::util::Geo *geo);

	/**
	 * \brief Calculate travel time in seconds
	 *
	 * Interpolate travel time in seconds given distance in degrees
	 *
	 * \param delta - A double value containing the distance in degrees
	 * to calculate travel time from
	 * \return Returns the travel time in seconds, or -1.0 if there is
	 * no valid travel time
	 */
	double T(double delta);

	/**
	 * \brief Lookup travel time in seconds
	 *
	 * Lookup travel time in seconds from travel time array given distance and
	 * depth indexes
	 *
	 * \param deltaIndex - An integer value containing distance index
	 * \param depthIndex - An integer value containing depth index
	 * \return Returns the travel time in seconds, or -1.0 if there is
	 * no valid travel time
	 */
	double T(int deltaIndex, int depthIndex);

	/**
	 * \brief Compute travel time in seconds via bilinear interpolation
	 *
	 * Compute a traveltime from travel time array via a bilinear interpolation
	 * using T() and the given distance and depth
	 *
	 * \param distance - A double value containing the distance to use
	 * \param depth - A double value  containing depth to use
	 * \return Returns the travel time in seconds, or -1.0 if there is
	 * no valid travel time
	 */
	double bilinear(double distance, double depth);

	/**
	 * \brief A pointer to the distance warp object used
	 */
	CTimeWarp *pDistanceWarp;

	/**
	 * \brief A pointer to the depth warp object used
	 */
	CTimeWarp *pDepthWarp;

	/**
	 * \brief An integer variable containing the grid index for the distance
	 * warp
	 */
	int nDistanceWarp;

	/**
	 * \brief An integer variable containing the grid index for the depth
	 * warp
	 */
	int nDepthWarp;

	/**
	 * \brief An array of double values containing the travel times indexed by
	 * depth and distance
	 */
	double *pTravelTimeArray;

	/**
	 * \brief An array of double values containing the distances indexed by
	 * depth
	 */
	double *pDepthDistanceArray;

	/**
	 * \brief An array of characters containing the phases
	 */
	char *pPhaseArray;

	/**
	 * \brief A std::std::string containing the name of the phase used for this
	 * CTravelTime
	 */
	std::string sPhase;

	/**
	 * \brief Delta in degrees used by caller to calculate distance
	 * dependent weights
	 */
	double dDelta;

	/**
	 * \brief Depth in km used by caller to calculate distance
	 * dependent weights
	 */
	double dDepth;

	/**
	 * \brief Ephemeral (temporary) glass3::util::Geo object containing current
	 * geographic location. Set by setOrigin()
	 */
	glass3::util::Geo geoOrg;
};
}  // namespace traveltime
#endif  // TRAVELTIME_H

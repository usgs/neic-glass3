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
	 * 
	 * \param useForLocations - A boolean flag indicating that this CTravelTime
	 * is usable for location
	 * \param minPublishable - A double value containing the minumum distance 
	 * in degrees to publish this phase.
	 * \param minPublishable - A double value containing the maximum distance 
	 * in degrees to publish this phase.
	 */
	explicit CTravelTime(bool useForLocations = true, double minPublishable = 0,
		double maxPublishable = 180);

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
	 * CRay and phase std::string and filename.
	 *
	 * \param phase - A std::std::string representing the phase to use, default
	 * is "P"
	 * \param file - A std::std::string representing the file to load, default
	 * is ""
	 */
	bool setup(std::string phase = "P", std::string file = "");

	/**
	 * \brief Write out travel times to file
	 * 
	 * This function prints the available travel time points for the travel
	 * time contained in this TravelTime at a given depth to a given file.
	 * 
	 * \param fileName - A std::string containing the file name to write to.
	 * \param depth - A double containing the depth to use
	 */
	void writeToFile(std::string fileName, double depth);

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
	void setTTOrigin(double lat, double lon, double depth);

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
	void setTTOrigin(const glass3::util::Geo &geoOrigin);

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
	 * \brief Compute bilinear interpolation
	 *
	 * Compute a bilinear interpolation from the provided values
	 *
	 * \param q_x1y1 - A double containing the interpolation value for (x1, y1)
	 * \param q_x1y2 - A double containing the interpolation value for (x1, y2)
	 * \param q_x2y1 - A double containing the interpolation value for (x2, y1)
	 * \param q_x2y2 - A double containing the interpolation value for (x2, y2)
	 * \param x1 - A double containing first x interpolation coordinate
	 * \param y1 - A double containing first y interpolation coordinate
	 * \param x2 - A double containing second x interpolation coordinate
	 * \param y2 - A double containing second y interpolation coordinate
	 * \param x - A double containing the given x coordinate
	 * \param y - A double containing the given y coordinate
	 * \return Returns a double containing the resulting bilinear interpolation
	 */
	double bilinearInterpolation(double q_x1y1, double q_x1y2, double q_x2y1,
		double q_x2y2, double x1, double y1, double x2, double y2, double x,
		double y);

	/**
	 * \brief Compute interpolation grid distance index
	 *
	 * Compute an interpolation grid index from the given distance
	 *
	 * \param distance - A double value containing the distance to use
	 * \return Returns the distance index
	 */
	int getIndexFromDistance(double distance);

	/**
	 * \brief Compute distance using interpolation grid index
	 *
	 * Compute a distance using the given interpolation grid index 
	 *
	 * \param index - An integer value containing the interpolation grid index to
	 * 	use
	 * \return Returns the distance
	 */
	double getDistanceFromIndex(int index);

	/**
	 * \brief Compute interpolation grid depth index
	 *
	 * Compute an interpolation grid index from the given depth
	 *
	 * \param depth - A double value containing the depth to use
	 * \return Returns the depth index
	 */
	int getIndexFromDepth(double depth);

	/**
	 * \brief Compute depth using interpolation grid index
	 *
	 * Compute a depth using the given interpolation grid index 
	 *
	 * \param index - An integer value containing the interpolation grid index to
	 * 	use
	 * \return Returns the depth
	 */
	double getDepthFromIndex(int index);


	/**
	 * \brief An integer variable containing the array index size for the distance
	 * array
	 */
	int m_iNumDistances;

	/**
	 * \brief A double variable containing the minimum distance of the depth 
	 * distance array
	 */
	double m_dMinimumDistance;

	/**
	 * \brief A double variable containing the maximum distance of the depth 
	 * distance array
	 */
	double m_dMaximumDistance;

	/**
	 * \brief A double variable containing the spacing, or step, between distance
	 * points in the depth distance array
	 */
	double m_dDistanceStep;

	/**
	 * \brief An integer variable containing the array index size for the depth
	 * array
	 */
	int m_iNumDepths;

	/**
	 * \brief A double variable containing the minimum depth of the depth 
	 * distance array
	 */
	double m_dMinimumDepth;

	/**
	 * \brief A double variable containing the maximum Depth of the depth 
	 * distance array
	 */
	double m_dMaximumDepth;

	/**
	 * \brief A double variable containing the spacing, or step, between depth
	 * points in the depth distance array
	 */
	double m_dDepthStep;

	/**
	 * \brief An array of double values containing the travel times indexed by
	 * depth and distance
	 */
	double * m_pTravelTimeArray;

	/**
	 * \brief A std::string containing the name of the phase used for this
	 * CTravelTime
	 */
	std::string m_sPhase;

	/**
	 * \brief A boolean flag indicating whether this CTravelTime should be used in 
	 * generating locations
	 */
	bool m_bUseForLocations;

	/**
	 * \brief A double value representing the minimum distance in degrees this  
	 * CTravelTime should be published in output messages
	 */
	double m_dMinDeltaPublishable;

	/**
	 * \brief A double value representing the maximum distance in degrees this  
	 * CTravelTime should be published in output messages
	 */
	double m_dMaxDeltaPublishable;

	/**
	 * \brief Delta in degrees used by caller to calculate distance
	 * dependent weights
	 */
	double m_dDelta;

	/**
	 * \brief Depth in km used by caller to calculate distance
	 * dependent weights
	 */
	double m_dDepth;

	/**
	 * \brief Ephemeral (temporary) glass3::util::Geo object containing current
	 * geographic location. Set by setOrigin()
	 */
	glass3::util::Geo m_geoTTOrigin;

	// constants
	/**
	 * \brief the value for an invalid travel time
	 */
	static constexpr double k_dTravelTimeInvalid = -1.0;

	/**
	 * \brief the string for an invalid phase name
	 */
	static const std::string k_dPhaseInvalid;
};
}  // namespace traveltime
#endif  // TRAVELTIME_H

/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef TTT_H
#define TTT_H
#include <string>
#include <mutex>
#include "Geo.h"
#include "TravelTime.h"
#include "Taper.h"

namespace traveltime {

#define MAX_TRAV 40  // Maximum number of supported travel time objects

class CRay;

/**
 * \brief travel time interface class
 *
 * The traveltime CTTT class is a class serves as the interface between
 * glass core and a set of phase specific CTravelTime objects.
 * CTTT supports calculating travel times based on distance
 * or geographic location for the phases in the set of CTravelTime objects.
 */
class CTTT {
 public:
	/**
	 * \brief CTTT constructor
	 *
	 * The constructor for the CTTT class.
	 */
	CTTT();

	/**
	 * \brief CTTT copy constructor
	 *
	 * The copy constructor for the CTTT class.
	 */
	CTTT(const CTTT &ttt);

	/**
	 * \brief CTTT destructor
	 *
	 * The destructor for the CTTT class.
	 */
	~CTTT();

	/**
	 * \brief CTTT clear function
	 */
	void clear();

	/**
	 * \brief Add phase to list to be calculated
	 *
	 * Creates a new CTravelTime object using the already set
	 * pRay and the provided phase name, and adds the object to the
	 * pTrv list.
	 *
	 * \param phase - A std::std::string containing the phase name to use
	 * \param weightRange - An array of 4 double values defining the phase
	 * taper weighting, mutually exclusive with assocRange
	 * \param assocRange - An array of 2 double values containing the
	 * association range, mutually exclusive with weightRange
	 * \param file - A std::std::string representing the file to load, default
	 * is ""
	 * \return Returns true if successful, false otherwise
	 */
	bool addPhase(std::string phase, double *weightRange = NULL,
					double *assocRange = NULL, std::string file = "");

	/**
	 * \brief Set hypocenter for calculations
	 *
	 * Set hypocenter for calculations using the provided latitude,
	 * longitude, and depth
	 *
	 * \param lat - A double value containing the latitude to use
	 * \param lon - A double value containing the longitude to use
	 * \param z - A double value containing the depth to use
	 */
	void setOrigin(double lat, double lon, double z);

	/**
	 * \brief Set current geographic location
	 *
	 * Set the current geographic location using the provided CGeo.
	 * This will set the source location used for source/receiver
	 * traveltime calculations.
	 *
	 * \param geoOrigin - A CGeo representing the lat/lon and surface depth (or
	 * elev) of the source, along with already computed concentric lat/lon and
	 * vector coordinates.
	 */
	void setOrigin(const glassutil::CGeo &geoOrigin);

	/**
	 * \brief Calculate travel time in seconds
	 *
	 * Calculate travel time in seconds given geographic location and
	 * the desired phase
	 *
	 * \param geo - A pointer to a glassutil::CGeo object representing the location
	 * to calculate the travel time from
	 * \param phase - A std::std::string containing the phase to use in calculating
	 * the travel time, default is "P"
	 * \return Returns the travel time in seconds, or -1.0 if there is
	 * no valid travel time
	 */
	double T(glassutil::CGeo *geo, std::string phase = "P");

	/**
	 * \brief Calculate travel time in seconds
	 *
	 * Calculate travel time in seconds given distance in degrees and the
	 * desired phase
	 *
	 * \param delta - A double value containing the distance in degrees
	 * to calculate travel time from
	 * \param phase - A std::std::string containing the phase to use in calculating
	 * the travel time, default is "P"
	 * \return Returns the travel time in seconds, or -1.0 if there is
	 * no valid travel time
	 */
	double T(double delta, std::string phase = "P");

	/**
	 * \brief Calculate travel time in seconds, setting depth
	 *
	 * Calculate travel time in seconds given distance in degrees and the
	 * desired phase
	 *
	 * \param delta - A double value containing the distance in degrees
	 * to calculate travel time from
	 * \param phase - A std::std::string containing the phase to use in calculating
	 * the travel time, default is "P"
	 * \param depth - A double containing specific depth
	 * \return Returns the travel time in seconds, or -1.0 if there is
	 * no valid travel time
	 */
	double Td(double delta, std::string phase = "P", double depth = 0.0);

	/**
	 * \brief Calculate best travel time in seconds
	 *
	 * Calculate best travel time in seconds given geographic location and
	 * the observed arrival time
	 *
	 * \param geo - A pointer to a glassutil::CGeo object representing the location
	 * to calculate the travel time from
	 * \param tobs - A double value containing the observed arrival time.
	 * \return Returns the travel time in seconds, or -1.0 if there is
	 * no valid travel time
	 */
	double T(glassutil::CGeo *geo, double tobs);

	/**
	 * \brief Print Travel Times to File
	 *
	 * Prints travel-times for a phase for testing purposes
	 */
	double testTravelTimes(std::string phase);

	/**
	 * \brief A temporary std::std::string variable containing the phase determined
	 * during the last call to T()
	 */
	std::string sPhase;

	/**
	 * \brief A temporary double variable containing the weight determined
	 * during the last call to T()
	 */
	double dWeight;

	/**
	 * \brief glassutil::CGeo object containing current
	 * geographic location of source(as in source/receiver). Set by setOrigin()
	 */
	glassutil::CGeo geoOrg;

	/**
	 * \brief An integer variable containing number of CTravelTime objects in
	 * pTrv
	 */
	int nTrv;

	/**
	 * \brief An array of pointers to CTravelTime objects for each phase
	 */
	CTravelTime *pTrv[MAX_TRAV];

	/**
	 * \brief An array of pointers taper objects that determine phase
	 * weights as a function of distance
	 */
	glassutil::CTaper *pTaper[MAX_TRAV];

	/**
	 * \brief An array of doubles containing the minimum values for association
	 */
	double dAssocMin[MAX_TRAV];

	/**
	 * \brief  An array of doubles containing the maximum values for association
	 */
	double dAssocMax[MAX_TRAV];

	// std::mutex m_TTTMutex;
};
}  // namespace traveltime
#endif  // TTT_H

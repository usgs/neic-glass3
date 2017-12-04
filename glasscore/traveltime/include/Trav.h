/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef TRAV_H
#define TRAV_H
#include <string>
#include <memory>

namespace traveltime {

#define MAX_POINTS 5000  // the maximum number of branch points for time or
                         // distance

// forward declarations
class CRay;
class CSpline;

/**
 * \brief traveltime branch class
 *
 * The traveltime CTrav class is a class that encapsulates
 * one or more 1D interpolated distance/time spline functions representing
 * seismic phase branches at a specific depth.
 *
 * The CTrav class uses the distance/time spline functions to calculate the
 * expected travel times and distances.
 * The spline functions are generated from the provided CRay parameters.
 *
 */
class CTrav {
 public:
	/**
	 * \brief CTrav constructor
	 *
	 * The constructor for the CTrav class.
	 */
	CTrav();

	/**
	 * \brief CTrav destructor
	 *
	 * The destructor for the CTrav class.
	 */
	~CTrav();

	/**
	 * \brief Generate branch and spline data
	 *
	 * Generate the branches and splines using a given CRay, phase string
	 * and depth.
	 *
	 * \param ray - A pointer to the CRay object to use
	 * \param phase - A const char* representing the phase string
	 * \param depth - A double variable representing depth in kilometers
	 * \return Returns true if the branch data and splines were generated,
	 * false otherwise.
	 */
	bool genBranch(CRay *ray, std::string phase, double depth);

	/**
	 * \brief Generate a branch for a phase
	 *
	 * Generate branch data for a phase using a given CRay and distance range.
	 *
	 * \param ray - A pointer to the CRay object to use
	 * \param startDistance - A double variable representing lower limit of the
	 * distance range
	 * \param endDistance - A double variable representing upper limit of the
	 * distance range
	 */
	void branch(CRay *ray, double startDistance, double endDistance);

	/**
	 * \brief Calculate travel time from distance
	 *
	 * Calculate travel time in seconds from a given distance in degrees
	 *
	 * \param delta - A double variable representing distance in degrees to
	 * calculate the travel time at
	 * \return Returns a double variable containing the travel time in seconds
	 */
	double T(double delta);

	/**
	 * \brief Calculate distance from travel time
	 *
	 * Calculate distance in degrees from a given travel time in seconds
	 *
	 * \param travelTime - A double variable representing travel time in seconds
	 * to calculate the distance from
	 * \return Returns a double variable containing the distance in degrees
	 */
	double D(double travelTime);

	/**
	 * \brief A std::string variable containing the phase or phase class
	 */
	std::string sPhase;

	/**
	 * \brief A double variable containing the depth from surface
	 */
	double dZ;

	/**
	 * \brief A pointer to a CSpline containing the generated time spline
	 */
	CSpline *tSpline;

	/**
	 * \brief A pointer to a CSpline containing the generated distance spline
	 */
	CSpline *dSpline;

	/**
	 * \brief A double variable containing the lower limit of the distance
	 * validity range in degrees
	 */
	double dDeg0;

	/**
	 * \brief A double variable containing the upper limit of the distance
	 * validity range in degrees
	 */
	double dDeg1;

	/**
	 * \brief A double variable containing the lower limit of the travel time
	 * validity range in seconds
	 */
	double dTrv0;

	/**
	 * \brief A double variable containing the upper limit of the travel time
	 * validity range in seconds
	 */
	double dTrv1;

	/**
	 * \brief An integer variable containing number of points used to generate
	 * the branches
	 */
	int nTrv;

	/**
	 * \brief A temporary array of distance (degrees) double values used
	 * in generating the branches
	 */
	double *dDeg;

	/**
	 * \brief A temporary array of travel time (seconds) double values
	 * used in generating the branches
	 */
	double *dTrv;
};
}  // namespace traveltime
#endif  // TRAV_H

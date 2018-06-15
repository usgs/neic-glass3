/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef TERRA_H
#define TERRA_H

#include <json.h>
#include <string>

namespace traveltime {

#define MAXLAYERS 200
#define ROMB_MAX 20

// It is significant that the S phase is 1 greater
// than the corresponding P phase.
#define FUN_TEST 1000
#define FUN_P_TIME	0
#define FUN_P_DELTA	2
#define FUN_P_TAU   4
#define FUN_S_TIME	1
#define FUN_S_DELTA 3
#define FUN_S_TAU   5

/**
 * \brief travel time earth structure model class
 *
 * The traveltime CTerra class is a class that loads,
 * parses, and contains the earth structure model.
 * The class also performs various travel time interpolation and
 * integration calculations.
 */
class CTerra {
 public:
	/**
	 * \brief CTerra constructor
	 *
	 * The constructor for the CTerra class.
	 */
	CTerra();

	/**
	 * \brief CTerra alternate constructor
	 *
	 * The alternate constructor for the CTerra class.
	 * Loads the earth structure model from a file.
	 * \param filename - A string containing the path and file name of the earth
	 * model file on disk.
	 */
	explicit CTerra(std::string filename);

	/**
	 * \brief CTerra destructor
	 *
	 * The destructor for the CTerra class.
	 */
	virtual ~CTerra();

	/**
	 * \brief CTerra clear function
	 */
	void clear();

	/**
	 * \brief Load earth model from file
	 *
	 * Loads the earth structure model from a file.
	 * \param filename - A string containing the path and file name of the earth
	 * model file on disk.
	 * \return Returns true if successful.
	 */
	bool load(std::string filename);

	/**
	 * \brief Parse a line from the file
	 *
	 * Parse a line from the earth model file
	 * \param line - A pointer character array containing the line
	 * to parse.
	 * \return Returns a json::Array of the values parsed
	 */
	json::Array parse(const char *line);

	/**
	 * \brief Interpolate P velocity
	 *
	 * Interpolate P velocity using the given radius
	 * currently linear only
	 * \param radius - A double value containing the earth radius
	 * to use
	 * \return Returns a double containing the P velocity.
	 */
	double P(double radius);

	/**
	 * \brief Interpolate S velocity
	 *
	 * Interpolate S velocity using the given radius
	 * currently linear only
	 * \param radius - A double value containing the earth radius
	 * to use
	 * \return Returns a double containing the S velocity.
	 */
	double S(double radius);

	/**
	 * \brief Interpolate Phase velocity
	 *
	 * Interpolate Phase velocity using the given radius and
	 * velocity table
	 * currently linear only
	 * \param radius - A double value containing the earth radius
	 * to use
	 * \param layerVelocity - A pointer to the array of double values containing
	 * the phase velocity in kilometers per second at each layer of the
	 * velocity model.
	 * \return Returns a double containing the velocity.
	 */
	double interpolateVelocity(double radius, double *layerVelocity);

	/**
	 * \brief Calculate ray bottoming radius
	 *
	 * Calculate bottoming radius for a ray with a given
	 * ray parameter between specified model index
	 * bounds. Out of range values cause interpolation from closest
	 * valid interval.
	 *  (This is an internal helper method for Tau().
	 *
	 * Assumes p monotonically increasing between lowerIndex and middleIndex.
	 * Out of range values cause interpolation from closest valid interval.
	 *
	 * \param lowerIndex - An integer value containing the lower model index
	 * \param upperIndex - An integer value containing the upper model index
	 * \param layerVelocity - A pointer to the array of double values containing
	 * the phase velocity in kilometers per second at each layer of the
	 * velocity model.
	 * \param rayParam - A double value containing the ray parameter.
	 * \return Returns a double containing the bottoming radius
	 */
	double calculateTurnRadius(int lowerIndex, int upperIndex,
								double *layerVelocity, double rayParam);

	/**
	 * \brief Evaluate function during integration
	 *
	 * Evaluate the specified function (time, delta, tau) during integration
	 *
	 * \param functionIndex - An integer value containing the index of the
	 * function to integrate
	 * \param earthRadius - A double value containing the earth radius
	 * to use
	 * \param rayParam -  A double value containing the ray parameter to use
	 * \return Returns a double containing the evaluated function value
	 */
	double evaluateFunction(int functionIndex, double earthRadius,
							double rayParam);

	/**
	 * \brief Compute the integral of a segment of the function
	 *
	 * Compute the integral of the function (time, delta, tau) over ray
	 * segment separated by parameter discontinuities
	 *
	 * \param functionIndex - An integer value containing the index of the
	 * function to integrate
	 * \param startingRadius - A double value containing the starting earth
	 * radius
	 * \param endingRadius - A double value containing the ending earth radius
	 * \param rayParam - A double value containing the ray parameter
	 * \return Returns a double containing the integral
	 */
	double integrateRaySegment(int functionIndex, double startingRadius,
								double endingRadius, double rayParam);

	/**
	 * \brief Compute the integral of the function
	 *
	 * Compute the integral of the function (time, delta, tau) between two
	 * earth radii using the Romberg integration formula
	 *
	 * Singularities allowed at end points
	 *
	 * \param functionIndex - An integer value containing the index of the
	 * function to integrate
	 * \param startingRadius - A double value containing the starting earth
	 * radius
	 * \param endingRadius - A double value containing the ending earth radius
	 * \param rayParam - a double value containing the ray parameter
	 * \return Returns a double containing the integral
	 */
	double rombergIntegration(int functionIndex, double startingRadius,
								double endingRadius, double rayParam);

	/**
	 * \brief A string containing the path to the model
	 * file.
	 */
	std::string sModelFilePath;

	/**
	 * \brief An integer containing the number of seismic
	 * discontinuities
	 */
	int nDiscontinuities;

	/**
	 * \brief An array of integer values containing indexes of each
	 * seismic discontinuity
	 */
	int iDiscontinuityIndexes[20];

	/**
	 * \brief An integer value containing the index of the
	 * inner core seismic discontinuity
	 */
	int iInnerDiscontinuity;

	/**
	 * \brief An integer value containing the index of the
	 * outer core seismic discontinuity
	 */
	int iOuterDiscontinuity;

	/**
	 * \brief A int64_t integer value containing the number of
	 * layers in the velocity model
	 */
	int64_t nLayer;

	/**
	 * \brief A double value containing the radius of the earth
	 * (at 0 depth)
	 */
	double dEarthRadius;

	/**
	 * \brief An array of double values containing the radius of the earth
	 * at each layer of the velocity model.  Ordered from the center of the
	 * earth out.
	 */
	double dLayerRadii[MAXLAYERS];

	/**
	 * \brief An array of double values containing P velocity in kilometers per
	 * second at each layer of the velocity model.  Ordered from the center of
	 * the earth out.
	 */
	double dLayerPVel[MAXLAYERS];

	/**
	 * \brief An array of double values containing S velocity in kilometers per
	 * second at each layer of the velocity model.  Ordered from the center of
	 * the earth out.
	 */
	double dLayerSVel[MAXLAYERS];
};
}  // namespace traveltime
#endif  // TERRA_H

/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef RAY_H
#define RAY_H

namespace traveltime {

/**
 * \brief enumeration of possible travel time ray phase indexes
 */
enum PhaseIndexes {
	RAY_Pup,
	RAY_P,
	RAY_Pdiff,
	RAY_PP,
	RAY_PPP,
	RAY_PKP,
	RAY_PKIKP,
	RAY_PKPab,
	RAY_PKPbc,
	RAY_PKPdf,
	RAY_PcP,
	RAY_Sup,
	RAY_S,
	RAY_Sdiff,
	RAY_SS,
	RAY_SSS
};

/**
 * \brief array containing possible travel time ray phase names
 */
static const char *PhaseIndexesValues[] = { "Pup", "P", "Pdiff", "PP", "PPP",
		"PKP", "PKIKP", "PKPab", "PKPbc", "PKPdf", "PcP", "Sup", "S", "Sdiff",
		"SS", "SSS" };

/**
 * \brief number of possible travel time ray phases
 */
static const int nPhase = 16;

class CTerra;
class CGeo;

/**
 * \brief traveltime ray path class
 *
 * The traveltime CRay class is a class that encapsulates
 * calculating travel time ray parameters, and using the ray
 * parameters to calculate expected travel times and distances.
 * The Ray parameters are calculated via inversions over a
 * provided CTerra earth structure model.
 *
 * Note that as written, CRay is NOT thread safe.
 */
class CRay {
 public:
	/**
	 * \brief CRay constructor
	 *
	 * The constructor for the CRay class.
	 */
	CRay();

	/**
	 * \brief CRay advanced constructor
	 *
	 * The advanced constructor for the CRay class.
	 *
	 * \param terra - A pointer to the CTerra for CRay to use
	 */
	explicit CRay(CTerra *terra);

	/**
	 * \brief CRay init method
	 *
	 * The initialize method for the CRay class.
	 *
	 * \param terra - A pointer to the CTerra for CRay to use
	 */
	void initialize(CTerra *terra);

	/**
	 * \brief CRay destructor
	 *
	 * The destructor for the CRay class.
	 */
	virtual ~CRay();

	/**
	 * \brief CRay clear function
	 */
	void clear();

	/**
	 * \brief Setup CRay class
	 *
	 * Setup CRay class based on the previously defined iPhaseIndex (via
	 * setPhase)
	 */
	void setupRayParam();

	/**
	 * \brief Set the phase for ray parameters
	 *
	 * Sets the iPhaseIndex for this CRay to the provided phase string
	 * \param phase - A const char* representing the phase string
	 * \return Returns the phase index as an integer if successful, -1 if not
	 */
	int setPhase(const char *phase);

	/**
	 * \brief Set the depth for calculations
	 *
	 * Set source depth prior to travel time or delta calculations
	 * \param depth - A double variable representing depth in kilometers
	 */
	void setDepth(double depth);

	/**
	 * \brief Calculate basic travel time from distance
	 *
	 * Calculate the basic travel time using the given distance
	 *
	 * NOTE Does not appear to be used
	 *
	 * \param delta - A double variable containing the distance in radians to
	 * use in calculating the travel time
	 * \return Returns the calculated travel time in seconds if successful, -1
	 * otherwise
	 */
	double travelBasic(double delta);

	/**
	 * \brief Calculate minimum travel time from distance
	 *
	 * Calculate minimum travel time from Tau curve for current branch using
	 * the given distance
	 *
	 * \param delta - A double variable containing the distance in radians to
	 * use in calculating the travel time
	 * \return Returns the calculated travel time in seconds if successful, -1
	 * otherwise
	 */
	double travel(double delta);

	/**
	 * \brief Calculate minimum travel time from distance and radius
	 *
	 * Calculate minimum travel time from Tau curve for current branch using
	 * the given distance and earth radius
	 *
	 * \param delta - A double variable containing the distance in radians to
	 * use in calculating the travel time
	 * \param earthRadius - A double variable containing the earth radius in
	 * kilometers
	 * \return Returns the calculated travel time in seconds if successful, -1
	 * otherwise
	 */
	double travel(double delta, double earthRadius);

	/**
	 * \brief Calculate minimum travel time from distance and radius
	 *
	 * Calculate minimum travel time from Tau curve for current branch using
	 * the given distance and earth radius, passing the ray parameter
	 *
	 * \param delta - A double variable containing the distance in radians to
	 * use in calculating the travel time
	 * \param earthRadius - A double variable containing the earth radius in
	 * kilometers
	 * \param rayParam - A pointer to a double variable to return the ray
	 * parameter.
	 * \return Returns the calculated travel time in seconds if successful, -1
	 * otherwise
	 */
	double travel(double delta, double earthRadius, double *rayParam);

	/**
	 * \brief Calculate minimum distance from travel time
	 *
	 * Calculate the minimum distance for current branch using
	 * the given travel time, passing the ray parameter
	 *
	 * NOTE Does not appear to be used
	 *
	 * \param time - A double variable containing the travel time in seconds to
	 * use in calculating the distance
	 * \param rayParam - A pointer to a double variable to return the ray
	 * parameter.
	 * \return Returns the calculated distance in radians if successful, -1
	 * otherwise if there is no such arrival
	 */
	double delta(double time, double *rayParam = 0);

	/**
	 * \brief Calculate travel time as a function of ray parameter.
	 *
	 * Calculate the travel time given the ray parameter and depth
	 *
	 * NOTE Does not appear to be used
	 *
	 * \param rayParam - A double variable containing the ray parameter.
	 * \param earthRadius - A double variable containing the earth radius in
	 * kilometers
	 * \return Returns the calculated travel time in seconds if successful, -1
	 * otherwise if there is no such arrival
	 */
	double T(double rayParam, double earthRadius);

	/**
	 * \brief Calculate distance as a function of ray parameter.
	 *
	 * Calculate the distance given the ray parameter and
	 * depth (expressed as an earth radius)
	 *
	 * NOTE Does not appear to be used
	 *
	 * \param rayParam - A double variable containing the ray parameter.
	 * \param earthRadius - A double variable containing the earth radius in
	 * kilometers
	 * \return Returns the calculated distance in radians if successful, -1
	 * otherwise if there is no such arrival
	 */
	double D(double rayParam, double earthRadius);

	/**
	 * \brief Calculate tau as a function of ray parameter.
	 *
	 * Calculate the tau given the ray parameter and
	 * depth (expressed as an earth radius)
	 *
	 * \param rayParam - A double variable containing the ray parameter.
	 * \param earthRadius - A double variable containing the earth radius in
	 * kilometers
	 * \return Returns the calculated tau if successful, -1
	 * otherwise
	 */
	double tau(double rayParam, double earthRadius);

	/**
	 * \brief Calculate time, distance, or tau integrals over depth.
	 *
	 * Calculate the time, distance, or tau integrals given the ray parameter
	 * and depth (expressed as an earth radius)
	 *
	 * This is an internal routine
	 *
	 * \param functionIndex - An integer parameter indicating what to integrate
	 * \param rayParam - A double variable containing the ray parameter.
	 * \param earthRadius - A double variable containing the earth radius in
	 * kilometers
	 * \return Returns the calculated tau if successful, -1
	 * otherwise
	 */
	double integrateFunction(int functionIndex, double rayParam,
								double earthRadius);

	/**
	 * \brief Calculates theta function
	 *
	 * Calculates theta function after Buland and Chapman(1983)
	 * I think....
	 *
	 * \param rayParam - The ray parameter
	 * \param dFunFac - The function factor
	 * \param dDelta - The distance
	 * \param dRcvr - The receiver function
	 * \return Returns the theta function
	 */
	double calculateThetaFunction(double rayParam, double dFunFac,
									double dDelta, double dRcvr);

	/**
	 * \brief Calculate bracket minima
	 *
	 * Calculate the bracket minima from a provided 1-dimensional function
	 *
	 * \param x - A pointer to an array of 6 doubles defining the function
	 * \param dFunFac - The function factor
	 * \param dDelta - The distance
	 * \param dRcvr - The receiver function
	 */
	void calculateBracketMinima(double *x, double dFunFac, double dDelta,
								double dRcvr);

	/**
	 * \brief Calculate minimum using brent algorithm
	 *
	 * Calculate the minimum using the brent algorithm
	 *
	 * \param xx - A pointer to an array of 3 doubles defining the function
	 * \param tol - A double value containing the tolerance
	 * \param xmin - A pointer to a double value to hold the minimum function x
	 * \param dFunFac - The function factor
	 * \param dDelta - The distance
	 * \param dRcvr - The receiver function
	 * \return returns the minimum
	 */
	double brentMinimization(double *xx, double tol, double *xmin,
								double dFunFac, double dDelta, double dRcvr);

	/**
	 * \brief A pointer to the CTerra object containing the earth
	 * structure model.
	 */
	CTerra *pTerra;

	/**
	 * \brief An integer variable containing the index of the phase
	 * (from the phase enumeration) for this ray.
	 */
	int iPhaseIndex;

	/**
	 * \brief A double variable containing the radius of the earth
	 * at a specific source depth in kilometers.
	 */
	double dEarthRadius;

	/**
	 * \brief A double variable containing the minimum ray parameter
	 * for the phase.
	 */
	double dMinimumRayParam;

	/**
	 * \brief A double variable containing the maximum ray parameter
	 * for the phase.
	 */
	double dMaximumRayParam;
};
}  // namespace traveltime
#endif  // RAY_H

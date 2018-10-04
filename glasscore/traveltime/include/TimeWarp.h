/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef TIMEWARP_H
#define TIMEWARP_H

namespace traveltime {

/**
 * \brief traveltime branch generator
 *
 * The CGenTrv class is used to generate the travel time tables used
 * by CTrv to rapidly calculate travel times using cubic interpolation
 * a N x M grid.
 *
 * CGenTrv uses smart pointers (std::shared_ptr).
 */
class CTimeWarp {
 public:
	/**
	 * \brief CTimeWarp constructor
	 *
	 * The constructor for the CTimeWarp class.
	 */
	CTimeWarp();

	/**
	 * \brief CTimeWarp copy constructor
	 *
	 * The copy constructor for the CTimeWarp class.
	 */
	CTimeWarp(const CTimeWarp & timeWarp);

	/**
	 * \brief CTimeWarp advanced constructor
	 *
	 * The advanced constructor for the CTimeWarp class.
	 *
	 * \param gridMin - A double value containing the Lowest value mapped to the
	 * grid
	 * \param gridMax - A double value containing the Highest value mapped to
	 * the grid
	 * \param decayConst - A double value containing the Decay exponent
	 * \param slopeZero - A double value containing the slope value per grid at
	 * 0
	 * \param slopeInf - A double value containing the  slope value per grid at
	 * infinity
	 */
	CTimeWarp(double gridMin, double gridMax, double decayConst,
				double slopeZero, double slopeInf);

	/**
	 * \brief CTimeWarp destructor
	 *
	 * The destructor for the CTimeWarp class.
	 */
	~CTimeWarp();

	/**
	 * \brief CTimeWarp clear function
	 */
	void clear();

	/**
	 * \brief CTimeWarp setup function
	 *
	 * The setup function for the CTimeWarp class,
	 * generating the time warp from the given values
	 *
	 * \param gridMin - A double value containing the Lowest value mapped to the
	 * grid
	 * \param gridMax - A double value containing the Highest value mapped to
	 * the grid
	 * \param decayConst - A double value containing the Decay exponent
	 * \param slopeZero - A double value containing the slope value per grid at
	 * 0
	 * \param slopeInf - A double value containing the  slope value per grid at
	 * infinity
	 */
	void setup(double gridMin, double gridMax, double decayConst,
				double slopeZero, double slopeInf);

	/**
	 * \brief Calculate grid index
	 *
	 * Calculate grid index from interpolated value
	 *
	 * \param value - A double value containing the interpolated value to use
	 * \return Returns the corresponding grid index
	 */
	double calculateGridPoint(double value);

	/**
	 * \brief Calculate interpolated value
	 *
	 * Calculate interpolated value at given grid point
	 *
	 * \param gridPoint - A double value containing the grid point to use
	 * \return Returns the corresponding interpolated value
	 */
	double calculateValue(double gridPoint);

	/**
	 * \brief A double value containing the Lowest value mapped to the grid
	 */
	double m_dGridMinimum;

	/**
	 * \brief A double value containing the Highest value mapped to the grid
	 */
	double m_dGridMaximum;

	/**
	 * \brief A double value containing the Decay exponent
	 */
	double m_dDecayConstant;

	/**
	 * \brief A double value containing the slope value per grid at 0
	 */
	double m_dSlopeZero;

	/**
	 * \brief A double value containing the slope value per grid at infinity
	 */
	double m_dSlopeInfinity;

	/**
	 * \brief A boolean value containing the flag indicating whether the time
	 * warp is setup
	 */
	bool m_bSetup;
};
}  // namespace traveltime
#endif  // TIMEWARP_H

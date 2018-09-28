/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef TAPER_H
#define TAPER_H

namespace glass3 {
namespace util {

/**
 * \brief glassutil taper class
 *
 * The Taper class represents a simple function used for averaging over an
 * interval. It uses a cosine based ramp up and ramp down at both ends for
 * stability.
 */
class Taper {
 public:
	/**
	 * \brief Taper constructor
	 *
	 * The constructor for the Taper class.
	 * Initializes members to default values.
	 */
	Taper();

	/**
	 * \brief Taper constructor
	 *
	 * The constructor for the Taper class.
	 * Initializes members to provided values.
	 *
	 * \param x1 - A double value representing the start point of the averaging
	 * function
	 * \param x2 - A double value representing the end of the cosine ramp up
	 * \param x3 - A double value representing the start of the cosine ramp
	 * down
	 * \param x4 - A double value representing the end point of the averaging
	 * function
	 */
	Taper(double x1, double x2, double x3, double x4);

	/**
	 * \brief Taper copy constructor
	 *
	 * The copy constructor for the Taper class.
	 */
	Taper(const Taper &taper);

	/**
	 * \brief Taper destructor
	 *
	 * The destructor for the Taper class.
	 */
	~Taper();

	/**
	 * \brief Calculate the value of the function
	 * Calculate the value of the function for a given value
	 *
	 * Values less than dX1 evaluate to 0.0.
	 * Between dX1 and dX2 the value ramps up to 1.0 with a cosine taper,
	 * then is constant at 1.0 from dX2 to dX3 whence it ramps back down to 0.0
	 * again with a cosine taper in reverse.
	 * Values greater than dX4 evaluate to 0.0.
	 *
	 * \param x - A double value to calculate the taper value from.
	 * \return Returns a double value containing the calculated taper value.
	 */
	double calculateValue(double x);

	/**
	 * \brief A double value representing the start point of the averaging
	 * function and cosine ramp up
	 */
	double m_dX1;

	/**
	 * \brief A double value representing the end of the cosine ramp up
	 */
	double m_dX2;

	/**
	 * \brief A double value representing the start of the cosine ramp down
	 */
	double m_dX3;

	/**
	 * \brief A double value representing the end point of the averaging
	 * function and cosine ramp down
	 */
	double m_dX4;
};
}  // namespace util
}  // namespace glass3
#endif  // TAPER_H

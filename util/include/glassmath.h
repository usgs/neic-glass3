/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef GLASSMATH_H
#define GLASSMATH_H

#include <random>

// mathmatical defines
#define RAD2DEG 57.29577951308
#define DEG2RAD	0.01745329251994
#define TWOPI 6.283185307179586
#define PI 3.14159265359

namespace glass3 {
namespace util {

/**
 * \brief glassutil logging class
 *
 * The CLogit class encapsulates the logic and functionality needed
 * to write logging information to disk.
 */
class GlassMath {
 public:
	/**
	 * \brief CGlass significance function
	 *
	 * This function calculates the significance function for glasscore,
	 * which is the bell shaped curve with sig(0, x) pinned to 0.
	 *
	 * \param tdif - A double containing x value.
	 * \param sig - A double value containing the sigma,
	 * \return Returns a double value containing significance function result
	 */
	static double sig(double tdif, double sig);

	/**
	 * \brief CGlass laplacian significance function (PDF)
	 *
	 * This function calculates a laplacian significance used in associator.
	 * This should have the affect of being L1 normish, instead of L2 normish.
	 * Unlike the other significance function, this returns the PDF value
	 * \param tdif - A double containing x value.
	 * \param sig - A double value containing the sigma,
	 * \return Returns a double value containing significance function result
	 */
	static double sig_laplace_pdf(double tdif, double sig);

	/**
	 * \brief Generate Random Number
	 *
	 * Generates random number between x and y. This function is used by gauss
	 * in determining the randomized step sizes for relocation (anneal and
	 * localize)
	 *
	 * \param x - The minimum random number
	 * \param y - The maximum random number
	 * \return Returns the random sample
	 */
	static double random(double x, double y);

	/**
	 * \brief Calculate Gaussian random sample
	 *
	 * Calculate random normal gaussian deviate value using Box-Muller method.
	 * This function is used in determining the randomized step sizes for
	 * relocation (anneal and localize)
	 *
	 * \param avg - The mean average value to use in the Box-Muller method
	 * \param std - The standard deviation value to use in the Box-Muller method
	 * \return Returns the Gaussian random sample
	 */
	static double gauss(double avg, double std);

	/**
	 * \brief initialize random number generator
	 *
	 * Initializes the random number generator by seeding m_RandomGenerator.
	 */
	static void initializeRandom();

 private:
	/**
	 * \brief A boolean flag to disable all logging
	 */
	static bool m_bInitialized;

	/**
	 * \brief The random engine for random()
	 */
	static std::default_random_engine m_RandomGenerator;
};
}  // namespace util
}  // namespace glass3
#endif  // GLASSMATH_H

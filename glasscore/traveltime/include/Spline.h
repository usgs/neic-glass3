/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef SPLINE_H
#define SPLINE_H

namespace traveltime {
/**
 * \brief traveltime spline class
 *
 * The traveltime CSpline class is a class that encapsulates
 * a cubic spline with natural end points (second derivative 0)
 * to increase the accuracy of travel-time lookups
 */
class CSpline {
 public:
	/**
	 * \brief CSpline constructor
	 *
	 * The constructor for the CSpline class.
	 */
	CSpline();

	/**
	 * \brief CSpline advanced constructor
	 *
	 * The advanced constructor for the CSpline class.
	 *
	 * \param n - An integer value containing the number of
	 * control points for the spline
	 * \param x - A pointer to an array of doubles (of size n) containing
	 * the x part of the control points
	 * \param y - A pointer to an array of doubles (of size n) containing
	 * the y part of the control points
	 */
	CSpline(int n, double *x, double *y);

	/**
	 * \brief CSpline destructor
	 *
	 * The destructor for the CSpline class.
	 */
	virtual ~CSpline();

	/**
	 * \brief CSpline clear function
	 */
	void clear();

	/**
	 * \brief CSpline setup function
	 *
	 * The setup function for the CSpline class,
	 * generating the spline from a set of n (x,y) control
	 * points.
	 *
	 * \param n - An integer value containing the number of
	 * control points for the spline
	 * \param x - A pointer to an array of doubles (of size n) containing
	 * the x part of the control points
	 * \param y - A pointer to an array of doubles (of size n) containing
	 * the y part of the control points
	 */
	void setup(int n, double *x, double *y);

	/**
	 * \brief Get a point on the spline
	 *
	 * Given an x, get the corresponding y point on the spline
	 *
	 * \param x - A double value containing the x to use
	 * \return Returns the corresponding y value, returns 0.0 if
	 * no corresponding value.
	 */
	double Y(double x);

	/**
	 * \brief Spline test function
	 */
	void test();

	/**
	 * \brief An integer value containing the number of control points
	 * in the spline
	 */
	int nX;

	/**
	 * \brief A pointer to a dynamic array of double values (of size nX)
	 * containing the x part of the control points.
	 */
	double *dX;

	/**
	 * \brief A pointer to a dynamic array of double values (of size nX)
	 * containing the y part of the control points.
	 */
	double *dY;

	/**
	 * \brief A pointer to a dynamic array of double values (of size nX)
	 * containing the derivative values
	 */
	double *dY2;
};
}  // namespace traveltime
#endif  // SPLINE_H

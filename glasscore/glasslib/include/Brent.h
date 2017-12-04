/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef BRENT_H
#define BRENT_H

#include <cmath>
#include <cfloat>

/**
 * \namespace glasscore
 * \brief namespace containing the glass core library
 *
 * The glasscore namespace contains the primary glass core library, including
 * all the classes and components necessary for association and nuclation
 * of new events.
 */
namespace glasscore {

/**
 * \brief The declaration of a template class type to pass the function
 * to minimize
 */
template<class TFunction>

/**
 * \brief Performs a Brent Minimization of the function
 *
 * Given a function f, bracketing intervals, and a stopping tolerance
 * compute the minimum of the function, and the location of that minimum.
 * The Notation and implementation based on Chapter 5 of Richard Brent's book
 * "Algorithms for Minimization Without Derivatives".
 *
 * \param f - An objective function of type TFunction to minimize
 * \param leftEnd - A Double value containing the smaller bracketing interval
 * \param rightEnd = A Double value containing the larger bracketing interval
 * \param epsilon - A Double value containing the stopping tolerance
 * \param minLoc - A Double reference to return the location where f takes its
 * minimum
 * \return Returns a double value containing the minimum of the function
 */
double Minimize(TFunction& f, double leftEnd, double rightEnd, double epsilon, //NOLINT
		double& minLoc) { // NOLINT
	double d, e, m, p, q, r, tol, t2, u, v, w, fu, fv, fw, fx;
	static const double c = 0.5 * (3.0 - sqrt(5.0));
	static const double SQRT_DBL_EPSILON = sqrt(DBL_EPSILON);

	double& a = leftEnd;
	double& b = rightEnd;
	double& x = minLoc;

	v = w = x = a + c * (b - a);
	d = e = 0.0;
	fv = fw = fx = f(x);
	int counter = 0;
	loop: counter++;
	m = 0.5 * (a + b);
	tol = SQRT_DBL_EPSILON * fabs(x) + epsilon;
	t2 = 2.0 * tol;
	// Check stopping criteria
	if (fabs(x - m) > t2 - 0.5 * (b - a)) {
		p = q = r = 0.0;
		if (fabs(e) > tol) {
			// fit parabola
			r = (x - w) * (fx - fv);
			q = (x - v) * (fx - fw);
			p = (x - v) * q - (x - w) * r;
			q = 2.0 * (q - r);
			(q > 0.0) ? p = -p : q = -q;
			r = e;
			e = d;
		}
		if (fabs(p) < fabs(0.5 * q * r) && p < q * (a - x) && p < q * (b - x)) {
			// A parabolic interpolation step
			d = p / q;
			u = x + d;
			// f must not be evaluated too close to a or b
			if (u - a < t2 || b - u < t2)
				d = (x < m) ? tol : -tol;
		} else {
			// A golden section step
			e = (x < m) ? b : a;
			e -= x;
			d = c * e;
		}
		// f must not be evaluated too close to x
		if (fabs(d) >= tol)
			u = x + d;
		else if (d > 0.0)
			u = x + tol;
		else
			u = x - tol;
		fu = f(u);
		// Update a, b, v, w, and x
		if (fu <= fx) {
			(u < x) ? b = x : a = x;
			v = w;
			fv = fw;
			w = x;
			fw = fx;
			x = u;
			fx = fu;
		} else {
			(u < x) ? a = u : b = u;
			if (fu <= fw || w == x) {
				v = w;
				fv = fw;
				w = u;
				fw = fu;
			} else if (fu <= fv || v == x || v == w) {
				v = u;
				fv = fu;
			}
		}
		goto loop;
		// Yes, the dreaded goto statement. But the code here is faithful to
		// Brent's original pseudocode.
	}
	return fx;
}
}  // namespace glasscore
#endif  // BRENT_H

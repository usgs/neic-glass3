#include "Spline.h"
#include <logger.h>
#include <cmath>

namespace traveltime {

// ---------------------------------------------------------CSpline
CSpline::CSpline() {
	clear();
}

// ---------------------------------------------------------CSpline
CSpline::CSpline(int n, double *x, double *y) {
	setup(n, x, y);
}

// ---------------------------------------------------------~CSpline
CSpline::~CSpline() {
	clear();
}

// ---------------------------------------------------------clear
void CSpline::clear() {
	nX = 0;

	if (dX) {
		delete[] (dX);
		dX = NULL;
	}
	if (dY) {
		delete[] (dY);
		dY = NULL;
	}
	if (dY2) {
		delete[] (dY2);
		dY2 = NULL;
	}
}

// ---------------------------------------------------------CSpline
void CSpline::setup(int n, double *x, double *y) {
	clear();

	// nullchecks
	if (n == 0) {
		glass3::util::Logger::log("error",
								"CSpline::setup: n is 0.");
		return;
	}
	if (x == NULL) {
		glass3::util::Logger::log("error",
								"CSpline::setup: NULL x");
		return;
	}
	if (y == NULL) {
		glass3::util::Logger::log("error",
								"CSpline::setup: NULL y.");
		return;
	}

	// remember number of points
	nX = n;

	// create control point arrays
	dX = new double[n];
	dY = new double[n];
	dY2 = new double[n];
	double *u = new double[n];

	// fill in control point arrays
	for (int i = 0; i < n; i++) {
		dX[i] = x[i];
		dY[i] = y[i];
	}

	// init derivative value arrays
	u[0] = 0.0;
	dY2[0] = 0.0;

	// compute derivative values
	for (int i = 1; i < n - 1; i++) {
		double sig = (x[i] - x[i - 1]) / (x[i + 1] - x[i - 1]);
		double p = sig * dY2[i - 1] + 2.0;
		dY2[i] = (sig - 1.0) / p;
		u[i] = (y[i + 1] - y[i]) / (x[i + 1] - x[i])
				- (y[i] - y[i - 1]) / (x[i] - x[i - 1]);
		u[i] = (6.0 * u[i] / (x[i + 1] - x[i - 1]) - sig * u[i - 1]) / p;
	}
	dY2[n - 1] = 0.0;

	for (int k = n - 2; k >= 0; k--) {
		dY2[k] = dY2[k] * dY2[k + 1] + u[k];
	}

	// cleanup
	delete[] (u);
}

// ---------------------------------------------------------CSpline
double CSpline::Y(double x) {
	// nullchecks
	if (nX == 0) {
		glass3::util::Logger::log("error",
								"CSpline::Y: nX is 0.");
		return (0);
	}
	if (dX == NULL) {
		glass3::util::Logger::log("error",
								"CSpline::Y: NULL dX");
		return (0);
	}
	if (dY == NULL) {
		glass3::util::Logger::log("error",
								"CSpline::Y: NULL dY.");
		return (0);
	}
	if (dY2 == NULL) {
		glass3::util::Logger::log("error",
								"CSpline::Y: NULL dY2.");
		return (0);
	}

	int klo = 1;
	int khi = nX;

	// find index corresponding to x
	while (khi - klo > 1) {
		int k = (khi + klo) >> 1;

		if (dX[k - 1] > x) {
			khi = k;
		} else {
			klo = k;
		}
	}

	// compute and check x value
	double h = dX[khi - 1] - dX[klo - 1];
	// NOTE: Why 1.0 - 30?
	if (h < 1.0 - 30) {
		return (0.0);
	}

	double a = (dX[khi - 1] - x) / h;
	double b = (x - dX[klo - 1]) / h;

	// compute y value
	double y = a * dY[klo - 1] + b * dY[khi - 1]
			+ ((a * a * a - a) * dY2[klo - 1] + (b * b * b - b) * dY2[khi - 1])
					* h * h / 6.0;

	return (y);
}

// ---------------------------------------------------------Test
/*  move to unit test
 void CSpline::test() {
 double x[100];
 double y[100];
 CSpline spline;
 double xx;
 double yy;
 double rr;

 for (int i = 0; i < 11; i++) {
 x[i] = 0.1 * i;
 y[i] = sin(x[i] * 6.28318530);
 }
 spline.Setup(11, x, y);
 for (int i = 0; i < 101; i++) {
 xx = 0.01 * i;
 yy = spline.Y(xx);
 rr = sin(xx * 6.28318530);
 }
 }*/
}  // namespace traveltime

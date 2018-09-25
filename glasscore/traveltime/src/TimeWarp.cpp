#include "TimeWarp.h"
#include <logger.h>
#include <cmath>

namespace traveltime {

// ---------------------------------------------------------CTimeWarp
CTimeWarp::CTimeWarp() {
	clear();
}

// ---------------------------------------------------------CTimeWarp
CTimeWarp::CTimeWarp(double gridMin, double gridMax, double decayConst,
						double slopeZero, double slopeInf) {
	clear();

	setup(gridMin, gridMax, decayConst, slopeZero, slopeInf);
}

// ---------------------------------------------------------CTimeWarp
CTimeWarp::CTimeWarp(const CTimeWarp & timeWarp) {
	clear();

	dGridMinimum = timeWarp.dGridMinimum;
	dGridMaximum = timeWarp.dGridMaximum;
	dDecayConstant = timeWarp.dDecayConstant;
	dSlopeZero = timeWarp.dSlopeZero;
	dSlopeInfinity = timeWarp.dSlopeInfinity;
	bSetup = timeWarp.bSetup;
}

// ---------------------------------------------------------~CTimeWarp
CTimeWarp::~CTimeWarp() {
	clear();
}

// ---------------------------------------------------------clear
void CTimeWarp::clear() {
	dGridMinimum = 0;
	dGridMaximum = 0;
	dDecayConstant = 0;
	dSlopeZero = 0;
	dSlopeInfinity = 0;
	bSetup = false;
}

// ---------------------------------------------------------setup
void CTimeWarp::setup(double gridMin, double gridMax, double decayConst,
						double slopeZero, double slopeInf) {
	dGridMinimum = gridMin;
	dGridMaximum = gridMax;
	dDecayConstant = decayConst;
	dSlopeZero = slopeZero;
	dSlopeInfinity = slopeInf;
	bSetup = true;
}

// ---------------------------------------------------------grid
double CTimeWarp::grid(double val) {
	// Calculate grid index from value
	if (bSetup == false) {
		glass3::util::Logger::log("error",
								"CTimeWarp::grid: Time Warp is not set up.");
	}

	// init
	double a = 1.0 / dSlopeInfinity;
	double b = 1.0 / dSlopeZero - 1.0 / dSlopeInfinity;
	double c = b * exp(-dDecayConstant * dGridMinimum) / dDecayConstant
			- a * dGridMinimum;

	// calculate grid index
	double grid = a * val - b * exp(-dDecayConstant * val) / dDecayConstant + c;

	return (grid);
}

// ---------------------------------------------------------value
double CTimeWarp::value(double gridIndex) {
	// Calculate interpolated value at given grid point
	if (bSetup == false) {
		glass3::util::Logger::log("error",
								"CTimeWarp::value: Time Warp is not set up.");
	}

	// init
	double a = 1.0 / dSlopeInfinity;
	double b = 1.0 / dSlopeZero - 1.0 / dSlopeInfinity;
	double val = 0.5 * (dGridMinimum + dGridMaximum);

	// Calculate interpolated value
	// NOTE: Why 100 here?
	for (int i = 0; i < 100; i++) {
		double f = grid(val) - gridIndex;
		if (fabs(f) < 0.000001) {
			break;
		}

		double fp = a + b * exp(-dDecayConstant * val);
		val -= f / fp;
	}
	return (val);
}
}  // namespace traveltime

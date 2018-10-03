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

	m_dGridMinimum = timeWarp.m_dGridMinimum;
	m_dGridMaximum = timeWarp.m_dGridMaximum;
	m_dDecayConstant = timeWarp.m_dDecayConstant;
	m_dSlopeZero = timeWarp.m_dSlopeZero;
	m_dSlopeInfinity = timeWarp.m_dSlopeInfinity;
	m_bSetup = timeWarp.m_bSetup;
}

// ---------------------------------------------------------~CTimeWarp
CTimeWarp::~CTimeWarp() {
	clear();
}

// ---------------------------------------------------------clear
void CTimeWarp::clear() {
	m_dGridMinimum = 0;
	m_dGridMaximum = 0;
	m_dDecayConstant = 0;
	m_dSlopeZero = 0;
	m_dSlopeInfinity = 0;
	m_bSetup = false;
}

// ---------------------------------------------------------setup
void CTimeWarp::setup(double gridMin, double gridMax, double decayConst,
						double slopeZero, double slopeInf) {
	m_dGridMinimum = gridMin;
	m_dGridMaximum = gridMax;
	m_dDecayConstant = decayConst;
	m_dSlopeZero = slopeZero;
	m_dSlopeInfinity = slopeInf;
	m_bSetup = true;
}

// ---------------------------------------------------------grid
double CTimeWarp::calculateGridPoint(double value) {
	// Calculate grid point value from given value
	if (m_bSetup == false) {
		glass3::util::Logger::log(
				"error", "CTimeWarp::grid: Time Warp is not set up.");
	}

	// Transform a value on a physical measurement scale to one on a pre-
	// conceived internal scale (grid point), where data points exist at each
	// natural-integral value, such that the data points are equidistant on the
	// internal scale, and conceived such that they efficiently represent
	// changes in the physical curve scale.
	double a = 1.0 / m_dSlopeInfinity;
	double b = 1.0 / m_dSlopeZero - 1.0 / m_dSlopeInfinity;
	double c = b * exp(-m_dDecayConstant * m_dGridMinimum) / m_dDecayConstant
			- a * m_dGridMinimum;

	// calculate grid point value
	double grid = a * value
			- b * exp(-m_dDecayConstant * value) / m_dDecayConstant + c;

	return (grid);
}

// ---------------------------------------------------------value
double CTimeWarp::calculateValue(double gridPoint) {
	// Calculate interpolated value at given grid point
	if (m_bSetup == false) {
		glass3::util::Logger::log(
				"error", "CTimeWarp::value: Time Warp is not set up.");
	}

	// init
	double a = 1.0 / m_dSlopeInfinity;
	double b = 1.0 / m_dSlopeZero - 1.0 / m_dSlopeInfinity;
	double value = 0.5 * (m_dGridMinimum + m_dGridMaximum);

	// Calculate interpolated value
	// NOTE: Why 100 here?
	for (int i = 0; i < 100; i++) {
		double f = calculateGridPoint(value) - gridPoint;
		if (fabs(f) < 0.000001) {
			break;
		}

		double fp = a + b * exp(-m_dDecayConstant * value);
		value -= f / fp;
	}
	return (value);
}
}  // namespace traveltime

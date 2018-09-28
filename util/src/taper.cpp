#include <taper.h>
#include <glassmath.h>

#include <cmath>

namespace glass3 {
namespace util {

// ---------------------------------------------------------Taper
Taper::Taper() {
	// Default constructor generates a taper function that
	// ramps from 0.0 to 1.0 reaching its maximum at 0.5
	m_dX1 = 0.0;
	m_dX2 = 0.5;
	m_dX3 = 0.5;
	m_dX4 = 1.0;
}

// ---------------------------------------------------------Taper
Taper::Taper(double x1, double x2, double x3, double x4) {
	m_dX1 = x1;
	m_dX2 = x2;
	m_dX3 = x3;
	m_dX4 = x4;
	// printf("Taper %.2f %.2f %.2f %.2f\n", dX1, dX2, dX3, dX4);
}

// ---------------------------------------------------------Taper
Taper::Taper(const Taper &taper) {
	m_dX1 = taper.m_dX1;
	m_dX2 = taper.m_dX2;
	m_dX3 = taper.m_dX3;
	m_dX4 = taper.m_dX4;
}

// ---------------------------------------------------------~Taper
Taper::~Taper() {
}

// --------------------------------------------------------------calculateValue
double Taper::calculateValue(double x) {
	// Values less than dX1 evaluate to 0.0, between dX1 and dX2
	// the value ramps up with a cosine taper, then is constant
	// at 1.0 from dX2 to dX3 whence it ramps back down to 0.0
	// again with a cosine taper in reverse
	if (x <= m_dX1) {
		return (0.0);
	}
	if (x >= m_dX4) {
		return (0.0);
	}
	if ((x >= m_dX2) && (x <= m_dX3)) {
		return 1.0;
	}
	if (x < m_dX2) {
		return (0.5 - 0.5 * cos(PI * (x - m_dX1) / (m_dX2 - m_dX1)));
	}
	if (x > m_dX3) {
		return (0.5 - 0.5 * cos(PI * (m_dX4 - x) / (m_dX4 - m_dX3)));
	}

	return (0.0);
}
}  // namespace util
}  // namespace glass3

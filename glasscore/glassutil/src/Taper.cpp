#include <cmath>
#include "Taper.h"
#include "Geo.h"

namespace glassutil {

// ---------------------------------------------------------CTaper
CTaper::CTaper() {
	// Default constructor generates a taper function that
	// ramps from 0.0 to 1.0 reaching its maximum at 0.5
	dX1 = 0.0;
	dX2 = 0.5;
	dX3 = 0.5;
	dX4 = 1.0;
}

// ---------------------------------------------------------CTaper
CTaper::CTaper(double x1, double x2, double x3, double x4) {
	dX1 = x1;
	dX2 = x2;
	dX3 = x3;
	dX4 = x4;
	// printf("Taper %.2f %.2f %.2f %.2f\n", dX1, dX2, dX3, dX4);
}

// ---------------------------------------------------------CTaper
CTaper::CTaper(const CTaper &taper) {
	dX1 = taper.dX1;
	dX2 = taper.dX2;
	dX3 = taper.dX3;
	dX4 = taper.dX4;
}

// ---------------------------------------------------------~CTaper
CTaper::~CTaper() {
}

// ----------------------------------------------------------------Val
double CTaper::Val(double x) {
	// Values less than dX1 evaluate to 0.0, between dX1 and dX2
	// the value ramps up with a cosine taper, then is constant
	// at 1.0 from dX2 to dX3 whence it ramps back down to 0.0
	// again with a cosine taper in reverse
	if (x <= dX1) {
		return (0.0);
	}
	if (x >= dX4) {
		return (0.0);
	}
	if ((x >= dX2) && (x <= dX3)) {
		return 1.0;
	}
	if (x < dX2) {
		return (0.5 - 0.5 * cos(PI * (x - dX1) / (dX2 - dX1)));
	}
	if (x > dX3) {
		return (0.5 - 0.5 * cos(PI * (dX4 - x) / (dX4 - dX3)));
	}

	return (0.0);
}
}  // namespace glassutil

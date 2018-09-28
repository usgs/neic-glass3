#include "Trav.h"
#include <glassmath.h>
#include <logger.h>
#include <memory>
#include <string>
#include "Ray.h"
#include "Spline.h"

namespace traveltime {

// ---------------------------------------------------------CTrav
CTrav::CTrav() {
	nTrv = 0;
	dTrv0 = 0;
	dTrv1 = 0;
	dDeg0 = 0;
	dDeg1 = 0;
	dZ = 0;
	sPhase = "";

	dTrv = NULL;
	dDeg = NULL;
	tSpline = NULL;
	dSpline = NULL;
}

// ---------------------------------------------------------~CTrav
CTrav::~CTrav() {
	if (tSpline) {
		delete (tSpline);
	}
	if (dSpline) {
		delete (dSpline);
	}
	if (dTrv) {
		delete (dTrv);
	}
	if (dDeg) {
		delete (dDeg);
	}
}

// ---------------------------------------------------------genBranch
bool CTrav::genBranch(CRay *ray, std::string phase, double depth) {
	// nullcheck
	if (ray == NULL) {
		glass3::util::Logger::log("error",
								"CTrav::genBranch: NULL CRay provided");
		return (false);
	}

	if (phase == "") {
		glass3::util::Logger::log("error",
								"CTrav::genBranch: empty phase provided");
		return (false);
	}

	// set phase and depth
	dZ = depth;
	sPhase = phase;

	// set initial values
	double startDistance = 0.0;
	double endDistance = 180.0;
	nTrv = 0;

	// allocate distance and travel time arrays
	dDeg = new double[MAX_POINTS];
	dTrv = new double[MAX_POINTS];

	// set up ray
	ray->setDepth(depth);

	// generate branch based on phase name
	if (phase == "P") {
		// P is special, combine Pup, P, and Pdiff
		// generate Pup
		ray->setPhase("Pup");
		ray->setupRayParam();
		branch(ray, startDistance, endDistance);

		// set start distance for next branch
		if (nTrv) {
			startDistance = dDeg[nTrv - 1] + 0.25;
		}

		// generate P
		ray->setPhase("P");
		ray->setupRayParam();
		branch(ray, startDistance, endDistance);

		// set start distance for next branch
		if (nTrv) {
			startDistance = dDeg[nTrv - 1] + 0.25;
		}

		// generate PDiff
		ray->setPhase("Pdiff");
		ray->setupRayParam();
		branch(ray, startDistance, endDistance);
	} else if (phase == "S") {
		// S is also special, combine Sup and S
		// generate Sup
		ray->setPhase("Sup");
		ray->setupRayParam();
		branch(ray, startDistance, endDistance);

		// set start distance for next branch
		if (nTrv) {
			startDistance = dDeg[nTrv - 1] + 0.25;
		}

		// generate S
		ray->setPhase("S");
		ray->setupRayParam();
		branch(ray, startDistance, endDistance);
	} else {
		// handle any other branch
		// generate branch
		ray->setPhase(phase.c_str());
		ray->setupRayParam();
		branch(ray, startDistance, endDistance);
	}

	// if we have at least 3 points, generate
	// time and distance splines
	if (nTrv > 3) {
		tSpline = new CSpline(nTrv, dDeg, dTrv);
		dSpline = new CSpline(nTrv, dTrv, dDeg);
	}

	// set distance validity range
	dDeg0 = dDeg[0];
	dDeg1 = dDeg[nTrv - 1];

	// set the travel time validity range
	dTrv0 = dTrv[0];
	dTrv1 = dTrv[nTrv - 1];

	// cleanup distance and travel time arrays
	delete[] (dDeg);
	delete[] (dTrv);

	// check splines
	if ((tSpline != NULL) && (dSpline != NULL)) {
		return (true);
	}

	glass3::util::Logger::log(
			"error",
			"CTrav::genBranch: Problem generating branch, splines were not"
			" created successfully");

	// something went wrong
	return (false);
}

// ---------------------------------------------------------branch
void CTrav::branch(CRay *ray, double startDistance, double endDistance) {
	// int state = 0;
	double minDistance = 0.01;  // NOTE: hardcoded

	// for each distance increment between the starting and ending distance
	// limits.  NOTE: Distance increment is hardcoded to 0.25
	for (double distance = startDistance; distance < endDistance; distance +=
			0.25) {
		// branches arrays bounds check
		if ((nTrv + 1) > MAX_POINTS) {
			continue;
		}

		double traveltime = -1;

		// check to see if distance less than the minimum allowed
		if (distance <= minDistance) {
			// use minimum
			traveltime = ray->travel(DEG2RAD * minDistance);
		} else {
			// use provided
			traveltime = ray->travel(DEG2RAD * distance);
		}

		// check for valid travel time
		if (traveltime < 0.0) {
			continue;
		}

		// populate distance / travel time for this point
		dDeg[nTrv] = distance;
		dTrv[nTrv] = traveltime;
		nTrv++;

		/* NOTE: This is the original code, however I don't see how this is
		 * effectively different than the above.  It looks like an attempt
		 * to have a different starting point than subsequent points, however
		 * other than the state change, the cases are identical.
		 * Keep in case this was somehow important.
		 switch (state) {
		 case 0:
		 // check for valid travel time
		 if (traveltime < 0.0) {
		 continue;
		 }
		 state = 1;

		 // populate distance / traveltime for this point
		 dDeg[nTrv] = distance;
		 dTrv[nTrv] = traveltime;
		 nTrv++;

		 break;
		 case 1:
		 // check for valid travel time
		 if (traveltime < 0.0) {
		 continue;
		 }

		 // populate distance / traveltime for this point
		 dDeg[nTrv] = distance;
		 dTrv[nTrv] = traveltime;
		 nTrv++;

		 break;
		 }
		 */
	}
}

// ---------------------------------------------------------T
double CTrav::T(double delta) {
	// Calculate travel time from distance in degrees
	if (tSpline == NULL) {
		glass3::util::Logger::log("error",
								"CTrav::T: NULL tSpline");
		return (-1.0);
	}

	// make sure delta is in the valid distance range
	if (delta < (dDeg0 - 0.001)) {
		return (-1.0);
	}
	if (delta > dDeg1) {
		return (-1.0);
	}

	return (tSpline->Y(delta));
}

// ---------------------------------------------------------D
double CTrav::D(double travelTime) {
	// Calculate distance (degrees) from travel time
	if (dSpline == NULL) {
		glass3::util::Logger::log("error",
								"CTrav::D: NULL dSpline");
		return (-1.0);
	}

	// make sure travelTime is in the valid time range
	if (travelTime < (dTrv0 - 0.001)) {
		return (-1.0);
	}
	if (travelTime > dTrv1) {
		return (-1.0);
	}

	return (dSpline->Y(travelTime));
}
}  // namespace traveltime

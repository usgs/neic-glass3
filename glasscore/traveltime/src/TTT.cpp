#include "TTT.h"
#include <logger.h>
#include <taper.h>
#include <geo.h>
#include <fstream>
#include <string>
#include <cmath>
#include "TravelTime.h"
#include "Ray.h"

namespace traveltime {

// ---------------------------------------------------------CTTT
CTTT::CTTT() {
	clear();
}

// ---------------------------------------------------------CTTT
CTTT::CTTT(const CTTT &ttt) {
	clear();

	nTrv = ttt.nTrv;
	geoOrg = ttt.geoOrg;
	dWeight = ttt.dWeight;

	for (int i = 0; i < ttt.nTrv; i++) {
		if (ttt.pTrv[i] != NULL) {
			pTrv[i] = new CTravelTime(*ttt.pTrv[i]);
		} else {
			pTrv[i] = NULL;
		}

		if (ttt.pTaper[i] != NULL) {
			pTaper[i] = new glass3::util::Taper(*ttt.pTaper[i]);
		} else {
			pTaper[i] = NULL;
		}

		dAssocMin[i] = ttt.dAssocMin[i];
		dAssocMax[i] = ttt.dAssocMax[i];
	}
}

// ---------------------------------------------------------~CTTT
CTTT::~CTTT() {
	for (int i = 0; i < nTrv; i++) {
		delete (pTrv[i]);

		if (pTaper[i] != NULL) {
			delete (pTaper[i]);
		}
	}
}

void CTTT::clear() {
	nTrv = 0;
	geoOrg.clear();
	dWeight = 0;

	for (int i = 0; i < MAX_TRAV; i++) {
		pTrv[i] = NULL;
		pTaper[i] = NULL;
		dAssocMin[i] = -1.0;
		dAssocMax[i] = -1.0;
	}
}

// ---------------------------------------------------------addPhase
// Add phase to list to be calculated
bool CTTT::addPhase(std::string phase, double *weightRange, double *assocRange,
					std::string file) {
	// bounds check
	if ((nTrv + 1) > MAX_TRAV) {
		glass3::util::Logger::log(
				"error",
				"CTTT::addPhase: Maximum number of phases ("
						+ std::to_string(MAX_TRAV) + ") reached");
		return (false);
	}

	// create and setup traveltime from phase
	CTravelTime *trv = new CTravelTime();
	trv->setup(phase, file);

	// add traveltime to list
	pTrv[nTrv] = trv;
	nTrv++;

	// setup taper for phase weighting
	if (weightRange != NULL) {
		pTaper[nTrv] = new glass3::util::Taper(weightRange[0], weightRange[1],
												weightRange[2], weightRange[3]);
	}
	// set up association range
	if (assocRange != NULL) {
		dAssocMin[nTrv] = assocRange[0];
		dAssocMax[nTrv] = assocRange[1];
	}

	return (true);
}

// ---------------------------------------------------------setOrigin
void CTTT::setOrigin(double lat, double lon, double z) {
	// this should go ahead and update the CGeo
	geoOrg.setGeographic(lat, lon, EARTHRADIUSKM - z);
}

// ---------------------------------------------------------setOrigin
void CTTT::setOrigin(const glass3::util::Geo &geoOrigin) {
  geoOrg = geoOrigin;
}

// ---------------------------------------------------------T
double CTTT::T(glass3::util::Geo *geo, std::string phase) {
	// Calculate travel time from distance in degrees
	// for each phase
	for (int i = 0; i < nTrv; i++) {
		// is this the phase we're looking for
		if (pTrv[i]->sPhase == phase) {
			// set origin
			pTrv[i]->setOrigin(geoOrg);

			// get travel time and phase
			double traveltime = pTrv[i]->T(geo);
			sPhase = phase;

			// use taper to compute weight if present
			if (pTaper[i] != NULL) {
				dWeight = pTaper[i]->calculateValue(pTrv[i]->dDelta);
			} else {
				dWeight = 0.0;
			}

			return (traveltime);
		}
	}

	// no valid travel time
	dWeight = 0.0;
	sPhase = "?";
	return (-1.0);
}

// ---------------------------------------------------------T
double CTTT::Td(double delta, std::string phase, double depth) {
	geoOrg.m_dGeocentricRadius = EARTHRADIUSKM - depth;
	// Calculate time from delta (degrees) and depth
	// for each phase
	for (int i = 0; i < nTrv; i++) {
		// is this the phase we're looking for
		if (pTrv[i]->sPhase == phase) {
			// set origin and depth
			pTrv[i]->setOrigin(geoOrg);

			// get travel time and phase
			double traveltime = pTrv[i]->T(delta);
			sPhase = phase;

			// use taper to compute weight if present
			if (pTaper[i] != NULL) {
				dWeight = pTaper[i]->calculateValue(delta);
			} else {
				dWeight = 0.0;
			}
			return (traveltime);
		}
	}

	// no valid travel time
	sPhase = "?";
	dWeight = 0.0;
	return (-1.0);
}

// ---------------------------------------------------------T
double CTTT::T(double delta, std::string phase) {
	// Calculate time from delta (degrees)
	// for each phase
	for (int i = 0; i < nTrv; i++) {
		// is this the phase we're looking for
		if (pTrv[i]->sPhase == phase) {
			// set origin
			pTrv[i]->setOrigin(geoOrg);

			// get travel time and phase
			double traveltime = pTrv[i]->T(delta);
			sPhase = phase;

			// use taper to compute weight if present
			if (pTaper[i] != NULL) {
				dWeight = pTaper[i]->calculateValue(delta);
			} else {
				dWeight = 0.0;
			}
			return (traveltime);
		}
	}

	// no valid travel time
	sPhase = "?";
	dWeight = 0.0;
	return (-1.0);
}

// ---------------------------------------------------------T
double CTTT::testTravelTimes(std::string phase) {
	// Calculate time from delta (degrees)
	double time = 0;
	std::ofstream outfile;
	std::string filename = phase + "_travel_time_Z_0.txt";
	outfile.open(filename, std::ios::out);

	for (double i = 0; i < 5.; i += 0.005) {
		time = Td(i, phase, 0.0);
		outfile << std::to_string(i) << ", " << std::to_string(time) << "\n";
	}
	outfile.close();

	filename = phase + "_travel_time_Z_50.txt";
	outfile.open(filename, std::ios::out);

	for (double i = 0; i < 5.; i += 0.005) {
		time = Td(i, phase, 50.);
		outfile << std::to_string(i) << ", " << std::to_string(time) << "\n";
	}
	outfile.close();

	filename = phase + "_travel_time_Z_100.txt";
	outfile.open(filename, std::ios::out);

	for (double i = 0; i < 5.; i += 0.005) {
		time = Td(i, phase, 100.);
		outfile << std::to_string(i) << ", " << std::to_string(time) << "\n";
	}
	outfile.close();

	return (1.0);
}

// ---------------------------------------------------------T
double CTTT::T(glass3::util::Geo *geo, double tObserved) {
	// Find Phase with least residual, returns time

	double bestTraveltime;
	std::string bestPhase;
	double weight;
	double minResidual = 1000.0;

	// for each phase
	for (int i = 0; i < nTrv; i++) {
		// get current aTrv
		CTravelTime * aTrv = pTrv[i];

		// set origin
		aTrv->setOrigin(geoOrg);

		// get traveltime
		double traveltime = aTrv->T(geo);

		// check traveltime
		if (traveltime < 0.0) {
			continue;
		}

		// check to see if phase is associable
		// based on minimum assoc distance, if present
		if (dAssocMin[i] >= 0) {
			if (aTrv->dDelta < dAssocMin[i]) {
				// this phase is not associable  at this distance
				continue;
			}
		}

		// check to see if phase is associable
		// based on maximum assoc distance, if present
		if (dAssocMax[i] > 0) {
			if (aTrv->dDelta > dAssocMax[i]) {
				// this phase is not associable  at this distance
				continue;
			}
		}

		// compute residual
		double residual = std::abs(tObserved - traveltime);

		// check to see if this residual is better than the previous
		//  best
		if (residual < minResidual) {
			// this is the new best travel time
			minResidual = residual;
			bestPhase = aTrv->sPhase;
			bestTraveltime = traveltime;

			// use taper to compute weight if present
			if (pTaper[i] != NULL) {
				weight = pTaper[i]->calculateValue(aTrv->dDelta);
			} else {
				weight = 0.0;
			}
		}
	}

	// check to see if minimum residual is valid
	if (minResidual < 999.0) {
		sPhase = bestPhase;
		dWeight = weight;

		return (bestTraveltime);
	}

	// no valid travel time
	sPhase = "?";
	dWeight = 0.0;
	return (-1.0);
}
}  // namespace traveltime

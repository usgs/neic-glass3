#include "TTT.h"
#include <logger.h>
#include <taper.h>
#include <geo.h>
#include <fstream>
#include <string>
#include <cmath>
#include "TravelTime.h"

namespace traveltime {

// constants
constexpr double CTTT::k_dTTTooLargeToBeValid;
const int CTTT::k_iMaximumNumberOfTravelTimes;

// ---------------------------------------------------------CTTT
CTTT::CTTT() {
	clear();
}

// ---------------------------------------------------------CTTT
CTTT::CTTT(const CTTT &ttt) {
	clear();

	m_iNumTravelTimes = ttt.m_iNumTravelTimes;
	m_geoTTOrigin = ttt.m_geoTTOrigin;
	m_dWeight = ttt.m_dWeight;

	for (int i = 0; i < ttt.m_iNumTravelTimes; i++) {
		if (ttt.m_pTravelTimes[i] != NULL) {
			m_pTravelTimes[i] = new CTravelTime(*ttt.m_pTravelTimes[i]);
		} else {
			m_pTravelTimes[i] = NULL;
		}

		if (ttt.m_pTapers[i] != NULL) {
			m_pTapers[i] = new glass3::util::Taper(*ttt.m_pTapers[i]);
		} else {
			m_pTapers[i] = NULL;
		}

		m_adMinimumAssociationValues[i] = ttt.m_adMinimumAssociationValues[i];
		m_adMaximumAssociationValues[i] = ttt.m_adMaximumAssociationValues[i];
	}
}

// ---------------------------------------------------------~CTTT
CTTT::~CTTT() {
	for (int i = 0; i < m_iNumTravelTimes; i++) {
		delete (m_pTravelTimes[i]);

		if (m_pTapers[i] != NULL) {
			delete (m_pTapers[i]);
		}
	}
}

void CTTT::clear() {
	m_iNumTravelTimes = 0;
	m_geoTTOrigin.clear();
	m_dWeight = 0;

	for (int i = 0; i < k_iMaximumNumberOfTravelTimes; i++) {
		m_pTravelTimes[i] = NULL;
		m_pTapers[i] = NULL;
		m_adMinimumAssociationValues[i] = CTravelTime::k_dTravelTimeInvalid;
		m_adMaximumAssociationValues[i] = CTravelTime::k_dTravelTimeInvalid;
	}
}

// ---------------------------------------------------------addPhase
// Add phase to list to be calculated
bool CTTT::addPhase(std::string phase, double *weightRange, double *assocRange,
					std::string file) {
	// bounds check
	if ((m_iNumTravelTimes + 1) > k_iMaximumNumberOfTravelTimes) {
		glass3::util::Logger::log(
				"error",
				"CTTT::addPhase: Maximum number of phases ("
						+ std::to_string(k_iMaximumNumberOfTravelTimes) + ") reached");
		return (false);
	}

	// create and setup traveltime from phase
	CTravelTime *trv = new CTravelTime();
	trv->setup(phase, file);

	// add traveltime to list
	m_pTravelTimes[m_iNumTravelTimes] = trv;
	m_iNumTravelTimes++;

	// setup taper for phase weighting
	if (weightRange != NULL) {
		m_pTapers[m_iNumTravelTimes] = new glass3::util::Taper(weightRange[0],
																weightRange[1],
																weightRange[2],
																weightRange[3]);
	}
	// set up association range
	if (assocRange != NULL) {
		m_adMinimumAssociationValues[m_iNumTravelTimes] = assocRange[0];
		m_adMaximumAssociationValues[m_iNumTravelTimes] = assocRange[1];
	}

	return (true);
}

// ---------------------------------------------------------setTTOrigin
void CTTT::setTTOrigin(double lat, double lon, double z) {
	// this should go ahead and update the CGeo
	m_geoTTOrigin.setGeographic(lat, lon,
								glass3::util::Geo::k_EarthRadiusKm - z);
}

// ---------------------------------------------------------setTTOrigin
void CTTT::setTTOrigin(const glass3::util::Geo &geoOrigin) {
	m_geoTTOrigin = geoOrigin;
}

// ---------------------------------------------------------T
double CTTT::T(glass3::util::Geo *geo, std::string phase) {
	// Calculate travel time from distance in degrees
	// for each phase
	for (int i = 0; i < m_iNumTravelTimes; i++) {
		// is this the phase we're looking for
		if (m_pTravelTimes[i]->m_sPhase == phase) {
			// set origin
			m_pTravelTimes[i]->setTTOrigin(m_geoTTOrigin);

			// get travel time and phase
			double traveltime = m_pTravelTimes[i]->T(geo);
			m_sPhase = phase;

			// use taper to compute weight if present
			if (m_pTapers[i] != NULL) {
				m_dWeight = m_pTapers[i]->calculateValue(
						m_pTravelTimes[i]->m_dDelta);
			} else {
				m_dWeight = 0.0;
			}

			return (traveltime);
		}
	}

	// no valid travel time
	m_dWeight = 0.0;
	m_sPhase = "?";
	return (CTravelTime::k_dTravelTimeInvalid);
}

// ---------------------------------------------------------T
double CTTT::Td(double delta, std::string phase, double depth) {
	m_geoTTOrigin.m_dGeocentricRadius = glass3::util::Geo::k_EarthRadiusKm
			- depth;
	// Calculate time from delta (degrees) and depth
	// for each phase
	for (int i = 0; i < m_iNumTravelTimes; i++) {
		// is this the phase we're looking for
		if (m_pTravelTimes[i]->m_sPhase == phase) {
			// set origin and depth
			m_pTravelTimes[i]->setTTOrigin(m_geoTTOrigin);

			// get travel time and phase
			double traveltime = m_pTravelTimes[i]->T(delta);
			m_sPhase = phase;

			// use taper to compute weight if present
			if (m_pTapers[i] != NULL) {
				m_dWeight = m_pTapers[i]->calculateValue(delta);
			} else {
				m_dWeight = 0.0;
			}
			return (traveltime);
		}
	}

	// no valid travel time
	m_sPhase = "?";
	m_dWeight = 0.0;
	return (CTravelTime::k_dTravelTimeInvalid);
}

// ---------------------------------------------------------T
double CTTT::T(double delta, std::string phase) {
	// Calculate time from delta (degrees)
	// for each phase
	for (int i = 0; i < m_iNumTravelTimes; i++) {
		// is this the phase we're looking for
		if (m_pTravelTimes[i]->m_sPhase == phase) {
			// set origin
			m_pTravelTimes[i]->setTTOrigin(m_geoTTOrigin);

			// get travel time and phase
			double traveltime = m_pTravelTimes[i]->T(delta);
			m_sPhase = phase;

			// use taper to compute weight if present
			if (m_pTapers[i] != NULL) {
				m_dWeight = m_pTapers[i]->calculateValue(delta);
			} else {
				m_dWeight = 0.0;
			}
			return (traveltime);
		}
	}

	// no valid travel time
	m_sPhase = "?";
	m_dWeight = 0.0;
	return (CTravelTime::k_dTravelTimeInvalid);
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
	double minResidual = k_dTTTooLargeToBeValid;

	// for each phase
	for (int i = 0; i < m_iNumTravelTimes; i++) {
		// get current aTrv
		CTravelTime * aTrv = m_pTravelTimes[i];

		// set origin
		aTrv->setTTOrigin(m_geoTTOrigin);

		// get traveltime
		double traveltime = aTrv->T(geo);

		// check traveltime
		if (traveltime < 0.0) {
			continue;
		}

		// check to see if phase is associable
		// based on minimum assoc distance, if present
		if (m_adMinimumAssociationValues[i] >= 0) {
			if (aTrv->m_dDelta < m_adMinimumAssociationValues[i]) {
				// this phase is not associable  at this distance
				continue;
			}
		}

		// check to see if phase is associable
		// based on maximum assoc distance, if present
		if (m_adMaximumAssociationValues[i] > 0) {
			if (aTrv->m_dDelta > m_adMaximumAssociationValues[i]) {
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
			bestPhase = aTrv->m_sPhase;
			bestTraveltime = traveltime;

			// use taper to compute weight if present
			if (m_pTapers[i] != NULL) {
				weight = m_pTapers[i]->calculateValue(aTrv->m_dDelta);
			} else {
				weight = 0.0;
			}
		}
	}

	// check to see if minimum residual is valid
	if (minResidual < k_dTTTooLargeToBeValid) {
		m_sPhase = bestPhase;
		m_dWeight = weight;

		return (bestTraveltime);
	}

	// no valid travel time
	m_sPhase = "?";
	m_dWeight = 0.0;
	return (CTravelTime::k_dTravelTimeInvalid);
}
}  // namespace traveltime

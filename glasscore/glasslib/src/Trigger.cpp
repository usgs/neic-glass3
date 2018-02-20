#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <mutex>
#include <algorithm>
#include "Trigger.h"
#include "Pick.h"
#include "Logit.h"

namespace glasscore {

// ---------------------------------------------------------CTrigger
CTrigger::CTrigger() {
	clear();
}

// ---------------------------------------------------------CNode
CTrigger::CTrigger(double lat, double lon, double z, double ot,
					double resolution, double sum, int count,
					std::vector<std::shared_ptr<CPick>> picks, CWeb *web) {
	if (!initialize(lat, lon, z, ot, resolution, sum, count, picks, web)) {
		clear();
	}
}

// ---------------------------------------------------------~CTrigger
CTrigger::~CTrigger() {
	clear();
}

// ---------------------------------------------------------clear
void CTrigger::clear() {
	std::lock_guard<std::recursive_mutex> guard(triggerMutex);

	dLat = 0;
	dLon = 0;
	dZ = 0;
	tOrg = 0;
	dResolution = 0;
	dSum = 0;
	nCount = 0;
	pWeb = NULL;

	vPick.clear();
}

bool CTrigger::initialize(double lat, double lon, double z, double ot,
							double resolution, double sum, int count,
							std::vector<std::shared_ptr<CPick>> picks,
							CWeb *web) {
	clear();

	std::lock_guard<std::recursive_mutex> guard(triggerMutex);

	dLat = lat;
	dLon = lon;
	dZ = z;
	tOrg = ot;
	dResolution = resolution;
	dSum = sum;
	nCount = count;
	pWeb = web;

	for (auto aPick : picks) {
		vPick.push_back(aPick);
	}

/*	glassutil::CLogit::log(
			glassutil::log_level::debug,
			"CTrigger::initialize: dLat:" + std::to_string(dLat) + "; dLon:"
					+ std::to_string(dLon) + "; dZ:" + std::to_string(dZ)
					+ "; tOrg:" + std::to_string(tOrg) + "; dResolution:"
					+ std::to_string(dResolution) + "; dSum:"
					+ std::to_string(dSum) + "; nCount:"
					+ std::to_string(nCount) + "; vPick Size:"
					+ std::to_string(vPick.size()));*/

	return (true);
}

double CTrigger::getLat() const {
	return (dLat);
}

double CTrigger::getLon() const {
	return (dLon);
}

double CTrigger::getZ() const {
	return (dZ);
}

double CTrigger::getTOrg() const {
	return (tOrg);
}

double CTrigger::getResolution() const {
	return (dResolution);
}

double CTrigger::getSum() const {
	return (dSum);
}

int CTrigger::getCount() const {
	return (nCount);
}

const CWeb* CTrigger::getWeb() const {
	return (pWeb);
}

const std::vector<std::shared_ptr<CPick> > CTrigger::getVPick() const {
	std::lock_guard<std::recursive_mutex> guard(triggerMutex);
	return (vPick);
}

}  // namespace glasscore

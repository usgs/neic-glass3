#include "Trigger.h"
#include <logger.h>
#include <geo.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <mutex>
#include <algorithm>
#include "Pick.h"

namespace glasscore {

// ---------------------------------------------------------CTrigger
CTrigger::CTrigger() {
	clear();
}

// ---------------------------------------------------------CTrigger
CTrigger::CTrigger(double lat, double lon, double z, double ot,
					double resolution, double maxZ, double sum, int count,
					bool aSeismic,
					std::vector<std::shared_ptr<CPick>> picks, CWeb *web) {
	if (!initialize(lat, lon, z, ot, resolution, maxZ, sum, count,
					aSeismic, picks, web)) {
		clear();
	}
}

// ---------------------------------------------------------~CTrigger
CTrigger::~CTrigger() {
}

// ---------------------------------------------------------clear
void CTrigger::clear() {
	std::lock_guard<std::recursive_mutex> guard(m_TriggerMutex);

	m_dLatitude = 0;
	m_dLongitude = 0;
	m_dDepth = 0;
	m_tOrigin = 0;
	m_dWebResolution = 0;
	m_dNodeMaxDepth = 0;
	m_dBayesValue = 0;
	m_iPickCount = 0;
	m_pWeb = NULL;
	m_dNodeAseismic = false;

	m_vPick.clear();
}

// ---------------------------------------------------------initialize
bool CTrigger::initialize(double lat, double lon, double z, double ot,
							double resolution, double maxZ, double sum,
							int count, bool aSeismic,
							std::vector<std::shared_ptr<CPick>> picks,
							CWeb *web) {
	clear();

	std::lock_guard<std::recursive_mutex> guard(m_TriggerMutex);

	m_dLatitude = lat;
	m_dLongitude = lon;
	m_dDepth = z;
	m_tOrigin = ot;
	m_dWebResolution = resolution;
	m_dNodeMaxDepth = maxZ;
	m_dBayesValue = sum;
	m_iPickCount = count;
	m_pWeb = web;
	m_dNodeAseismic = aSeismic;

	for (auto aPick : picks) {
		m_vPick.push_back(aPick);
	}

	return (true);
}

// ---------------------------------------------------------getLatitude
double CTrigger::getLatitude() const {
	return (m_dLatitude);
}

// ---------------------------------------------------------getLongitude
double CTrigger::getLongitude() const {
	return (m_dLongitude);
}

// ---------------------------------------------------------getDepth
double CTrigger::getDepth() const {
	return (m_dDepth);
}

// ---------------------------------------------------------getTOrigin
double CTrigger::getTOrigin() const {
	return (m_tOrigin);
}

// ---------------------------------------------------------getGeo
glass3::util::Geo CTrigger::getGeo() const {
	glass3::util::Geo geoTrigger;
	geoTrigger.setGeographic(m_dLatitude, m_dLongitude,
								glass3::util::Geo::k_EarthRadiusKm - m_dDepth);
	return (geoTrigger);
}

// ---------------------------------------------------------getResolution
double CTrigger::getWebResolution() const {
	return (m_dWebResolution);
}

// ---------------------------------------------------------getNodeMaxDepth
double CTrigger::getNodeMaxDepth() const {
	return (m_dNodeMaxDepth);
}

// ---------------------------------------------------------getBayesValue
double CTrigger::getBayesValue() const {
	return (m_dBayesValue);
}

// ---------------------------------------------------------getPickCount
int CTrigger::getPickCount() const {
	return (m_iPickCount);
}

// ---------------------------------------------------------getWeb
CWeb* CTrigger::getWeb() {
	return (m_pWeb);
}

// ---------------------------------------------------------getVPick
const std::vector<std::shared_ptr<CPick> > CTrigger::getVPick() const {
	std::lock_guard<std::recursive_mutex> guard(m_TriggerMutex);
	return (m_vPick);
}

// ---------------------------------------------------------getNodeAseismic
bool CTrigger::getNodeAseismic() const {
	return (m_dNodeAseismic);
}
}  // namespace glasscore

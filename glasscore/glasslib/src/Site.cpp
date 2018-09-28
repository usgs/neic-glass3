#include "Site.h"
#include <json.h>
#include <logger.h>
#include <geo.h>
#include <sstream>
#include <cmath>
#include <utility>
#include <tuple>
#include <string>
#include <memory>
#include <vector>
#include <set>
#include <algorithm>
#include <mutex>
#include <ctime>
#include "Glass.h"
#include "Pick.h"
#include "Node.h"
#include "Trigger.h"
#include "Hypo.h"
#include "Web.h"

namespace glasscore {

// ---------------------------------------------------------CSite
CSite::CSite() {
	clear();
}

// ---------------------------------------------------------CSite
CSite::CSite(std::string sta, std::string comp, std::string net,
				std::string loc, double lat, double lon, double elv,
				double qual, bool enable, bool useTele) {
	// pass to initialization function
	initialize(sta, comp, net, loc, lat, lon, elv, qual, enable, useTele);
}

// ---------------------------------------------------------CSite
CSite::CSite(std::shared_ptr<json::Object> site) {
	// null check json
	if (site == NULL) {
		glass3::util::Logger::log("error",
								"CSite::CSite: NULL json site.");
		return;
	}

	// check type
	if (site->HasKey("Type")
			&& ((*site)["Type"].GetType() == json::ValueType::StringVal)) {
		std::string type = (*site)["Type"].ToString();

		if (type != "StationInfo") {
			glass3::util::Logger::log(
					"warning",
					"CSite::CSite: Non-StationInfo message passed in.");
			return;
		}
	} else {
		glass3::util::Logger::log("error",
								"CSite::CSite: Missing required Type Key.");
		return;
	}

	// site definition variables
	std::string station = "";
	std::string channel = "";
	std::string network = "";
	std::string location = "";
	double latitude = 0;
	double longitude = 0;
	double elevation = 0;

	// optional values
	double quality = 0;
	bool enable = true;
	bool useForTelesiesmic = true;

	// get site information from json
	// scnl
	if (((*site).HasKey("Site"))
			&& ((*site)["Site"].GetType() == json::ValueType::ObjectVal)) {
		json::Object siteobj = (*site)["Site"].ToObject();

		// station
		if (siteobj.HasKey("Station")
				&& (siteobj["Station"].GetType() == json::ValueType::StringVal)) {
			station = siteobj["Station"].ToString();
		} else {
			glass3::util::Logger::log(
					"error",
					"CSite::CSite: Missing required Station Key.");

			return;
		}

		// channel (optional)
		if (siteobj.HasKey("Channel")
				&& (siteobj["Channel"].GetType() == json::ValueType::StringVal)) {
			channel = siteobj["Channel"].ToString();
		} else {
			channel = "";
		}

		// network
		if (siteobj.HasKey("Network")
				&& (siteobj["Network"].GetType() == json::ValueType::StringVal)) {
			network = siteobj["Network"].ToString();
		} else {
			glass3::util::Logger::log(
					"error",
					"CSite::CSite: Missing required Network Key.");

			return;
		}

		// location (optional)
		if (siteobj.HasKey("Location")
				&& (siteobj["Location"].GetType() == json::ValueType::StringVal)) {
			location = siteobj["Location"].ToString();
		} else {
			location = "";
		}

		// -- is used by some networks to represent an empty
		// location code
		if (location == "--") {
			location = "";
		}
	} else {
		glass3::util::Logger::log("error",
								"CSite::CSite: Missing required Site Object.");

		return;
	}

	// latitude for this site
	if (((*site).HasKey("Latitude"))
			&& ((*site)["Latitude"].GetType() == json::ValueType::DoubleVal)) {
		latitude = (*site)["Latitude"].ToDouble();
	} else {
		glass3::util::Logger::log("error",
								"CSite::CSite: Missing required Latitude Key.");

		return;
	}

	// longitude for this site
	if (((*site).HasKey("Longitude"))
			&& ((*site)["Longitude"].GetType() == json::ValueType::DoubleVal)) {
		longitude = (*site)["Longitude"].ToDouble();
	} else {
		glass3::util::Logger::log(
				"error",
				"CSite::CSite: Missing required Longitude Key.");

		return;
	}

	// elevation for this site
	if (((*site).HasKey("Elevation"))
			&& ((*site)["Elevation"].GetType() == json::ValueType::DoubleVal)) {
		elevation = (*site)["Elevation"].ToDouble();
	} else {
		glass3::util::Logger::log(
				"error",
				"CSite::CSite: Missing required Elevation Key.");

		return;
	}

	// quality for this site (if present)
	if (((*site).HasKey("Quality"))
			&& ((*site)["Quality"].GetType() == json::ValueType::DoubleVal)) {
		quality = (*site)["Quality"].ToDouble();
	} else {
		quality = 1.0;
	}

	// enable for this site (if present)
	if (((*site).HasKey("Enable"))
			&& ((*site)["Enable"].GetType() == json::ValueType::BoolVal)) {
		enable = (*site)["Enable"].ToBool();
	} else {
		enable = true;
	}

	// enable for this site (if present)
	if (((*site).HasKey("UseForTeleseismic"))
			&& ((*site)["UseForTeleseismic"].GetType()
					== json::ValueType::BoolVal)) {
		useForTelesiesmic = (*site)["UseForTeleseismic"].ToBool();
	} else {
		useForTelesiesmic = true;
	}

	// pass to initialization function
	initialize(station, channel, network, location, latitude, longitude,
				elevation, quality, enable, useForTelesiesmic);
}

// --------------------------------------------------------initialize
bool CSite::initialize(std::string sta, std::string comp, std::string net,
						std::string loc, double lat, double lon, double elv,
						double qual, bool enable, bool useTele) {
	clear();

	std::lock_guard<std::recursive_mutex> guard(m_SiteMutex);

	// generate scnl
	m_sSCNL = "";

	// station, required
	if (sta != "") {
		m_sSCNL += sta;
	} else {
		glass3::util::Logger::log("error",
								"CSite::initialize: missing sSite.");
		return (false);
	}

	// component, optional
	if (comp != "") {
		m_sSCNL += "." + comp;
	}

	// network, required
	if (net != "") {
		m_sSCNL += "." + net;
	} else {
		glass3::util::Logger::log("error",
								"CSite::initialize: missing sNet.");
		return (false);
	}

	// location, optional
	if (loc != "") {
		m_sSCNL += "." + loc;
	}

	// fill in site/net/etc
	m_sSite = sta;
	m_sNetwork = net;
	m_sComponent = comp;
	m_sLocation = loc;

	// set geographic location
	// convert site elevation in meters to surface depth in km
	// (invert the sign to get positive (above earth radius)
	// and then divide by 1000 (meters to km))
	setLocation(lat, lon, -0.001 * elv);

	// quality
	m_dQuality = qual;

	// copy use
	m_bEnable = enable;
	m_bUse = true;
	m_bUseForTeleseismic = useTele;

	return (true);
}

// ---------------------------------------------------------~CSite
CSite::~CSite() {
	clear();
}

// --------------------------------------------------------clear
void CSite::clear() {
	std::lock_guard<std::recursive_mutex> guard(m_SiteMutex);
	// clear scnl
	m_sSCNL = "";
	m_sSite = "";
	m_sComponent = "";
	m_sNetwork = "";
	m_sLocation = "";

	m_bUse = true;
	m_bEnable = true;
	m_bUseForTeleseismic = true;
	m_dQuality = 1.0;

	// clear geographic
	m_Geo = glass3::util::Geo();
	m_daUnitVectors[0] = 0;
	m_daUnitVectors[1] = 0;
	m_daUnitVectors[2] = 0;

	// clear lists
	m_vNodeMutex.lock();
	m_vNode.clear();
	m_vNodeMutex.unlock();

	vPickMutex.lock();

	// init the upper and lower values
	std::shared_ptr<CSite> nullSite;
	if (m_LowerValue == NULL) {
		m_LowerValue = std::make_shared<CPick>(nullSite, 0, "lower", 0, 0);
	}
	if (m_UpperValue == NULL) {
		m_UpperValue = std::make_shared<CPick>(nullSite, 0, "upper", 0, 0);
	}

	m_msPickList.clear();

	vPickMutex.unlock();

	// reset last pick added time
	m_tLastPickAdded = std::time(NULL);

	// reset picks since last check
	setPickCountSinceCheck(0);
}

// --------------------------------------------------------update
void CSite::update(CSite *aSite) {
	std::lock_guard<std::recursive_mutex> guard(m_SiteMutex);
	// scnl check
	if (m_sSCNL != aSite->getSCNL()) {
		return;
	}

	// update station quality metrics
	m_bEnable = aSite->getEnable();
	m_bUseForTeleseismic = aSite->getUseForTeleseismic();
	m_dQuality = aSite->getQuality();

	// update location
	m_Geo = glass3::util::Geo(aSite->getGeo());
	double vec[3];
	aSite->getUnitVectors(vec);

	m_daUnitVectors[0] = vec[0];
	m_daUnitVectors[1] = vec[1];
	m_daUnitVectors[2] = vec[2];

	// copy statistics
	m_tLastPickAdded = aSite->getTLastPickAdded();

	// leave lists, and pointers alone
}

// --------------------------------------------------------getUnitVectors
double * CSite::getUnitVectors(double * vec) {
	if (vec == NULL) {
		return (NULL);
	}

	vec[0] = m_daUnitVectors[0];
	vec[1] = m_daUnitVectors[1];
	vec[2] = m_daUnitVectors[2];

	return (vec);
}

// ---------------------------------------------------------setLocation
void CSite::setLocation(double lat, double lon, double z) {
	std::lock_guard<std::recursive_mutex> guard(m_SiteMutex);
	// construct unit vector in cartesian earth coordinates
	double rxy = cos(DEG2RAD * lat);
	m_daUnitVectors[0] = rxy * cos(DEG2RAD * lon);
	m_daUnitVectors[1] = rxy * sin(DEG2RAD * lon);
	m_daUnitVectors[2] = sin(DEG2RAD * lat);

	// set geographic object
	m_Geo.setGeographic(lat, lon, EARTHRADIUSKM - z);
}

// ---------------------------------------------------------getDelta
double CSite::getDelta(glass3::util::Geo *geo2) {
	// nullcheck
	if (geo2 == NULL) {
		glass3::util::Logger::log("warning",
								"CSite::getDelta: NULL CGeo provided.");
		return (0);
	}

	// use CGeo to calculate distance in radians
	return (m_Geo.delta(geo2));
}

// ---------------------------------------------------------getDistance
double CSite::getDistance(std::shared_ptr<CSite> site) {
	// nullcheck
	if (site == NULL) {
		glass3::util::Logger::log("warning",
								"CSite::getDistance: NULL CSite provided.");
		return (0);
	}

	// use unit vectors in cartesian earth coordinates
	// to quickly great circle distance in km
	double dot = 0;
	double dkm;
	for (int i = 0; i < 3; i++) {
		dot += m_daUnitVectors[i] * site->m_daUnitVectors[i];
	}
	dkm = 6366.2 * acos(dot);

	// return distance
	return (dkm);
}

// ---------------------------------------------------------addPick
void CSite::addPick(std::shared_ptr<CPick> pck) {
	// lock for editing
	std::lock_guard<std::mutex> guard(vPickMutex);

	// nullcheck
	if (pck == NULL) {
		glass3::util::Logger::log("warning",
								"CSite::addPick: NULL CPick provided.");
		return;
	}

	// ensure this pick is for this site
	if (pck->getSite()->m_sSCNL != m_sSCNL) {
		glass3::util::Logger::log(
				"warning",
				"CSite::addPick: CPick for different site: (" + m_sSCNL + "!="
						+ pck->getSite()->m_sSCNL + ")");
		return;
	}

	// add pick to site pick multiset
	m_msPickList.insert(pck);

	// remember the time the last pick was added
	m_tLastPickAdded = std::time(NULL);

	// keep track of how many picks
	setPickCountSinceCheck(getPickCountSinceCheck() + 1);
}

// ---------------------------------------------------------removePick
void CSite::removePick(std::shared_ptr<CPick> pck) {
	// nullcheck
	if (pck == NULL) {
		glass3::util::Logger::log("warning",
								"CSite::removePick: NULL CPick provided.");
		return;
	}
	if (pck->getID() == "") {
		return;
	}

	std::lock_guard<std::mutex> guard(vPickMutex);

	// erase it
	eraseFromMultiset(pck);
}

// ---------------------------------------------------------getVPick
std::vector<std::shared_ptr<CPick>> CSite::getPicks(double t1, double t2) {
	std::vector<std::shared_ptr<CPick>> picks;

	if (t1 > t2) {
		double temp = t2;
		t2 = t1;
		t1 = temp;
	}

	// nullcheck
	if ((m_LowerValue == NULL) || (m_UpperValue == NULL)) {
		return (picks);
	}

	std::lock_guard<std::mutex> listGuard(vPickMutex);

	// don't bother if the list is empty
	if (m_msPickList.size() == 0) {
		return (picks);
	}

	// get the bounds for this window
	std::multiset<std::shared_ptr<CPick>, SitePickCompare>::iterator lower =
			getLower(t1);
	std::multiset<std::shared_ptr<CPick>, SitePickCompare>::iterator upper =
			getUpper(t2);

	// found nothing
	if (lower == getEnd()) {
		return (picks);
	}

	// found one
	if ((lower == upper) && (lower != getEnd())) {
		std::shared_ptr<CPick> aPick = *lower;

		if (aPick != NULL) {
			// add to the list of picks
			picks.push_back(aPick);
		}

		return (picks);
	}  // end found one

	// loop through found picks
	for (std::multiset<std::shared_ptr<CPick>, SitePickCompare>::iterator it =
			lower; ((it != upper) && (it != getEnd())); ++it) {
		std::shared_ptr<CPick> aPick = *it;

		if (aPick != NULL) {
			// add to the list of picks
			picks.push_back(aPick);
		}
	}

	// return the list of picks we found
	return (picks);
}

// ---------------------------------------------------------getLower
std::multiset<std::shared_ptr<CPick>, SitePickCompare>::iterator CSite::getLower(  // NOLINT
		double min) {
	m_LowerValue->setTSort(min);
	return (m_msPickList.lower_bound(m_LowerValue));
}

// ---------------------------------------------------------getUpper
std::multiset<std::shared_ptr<CPick>, SitePickCompare>::iterator CSite::getUpper(  // NOLINT
		double max) {
	m_UpperValue->setTSort(max);
	return (m_msPickList.upper_bound(m_UpperValue));
}

// ---------------------------------------------------------getEnd
std::multiset<std::shared_ptr<CPick>, SitePickCompare>::iterator CSite::getEnd() {  // NOLINT
	return (m_msPickList.end());
}

// ---------------------------------------------------------getPickMutex
std::mutex & CSite::getPickMutex() {
	return (vPickMutex);
}

// ---------------------------------------------------------addNode
void CSite::addNode(std::shared_ptr<CNode> node, double distDeg,
					double travelTime1, double travelTime2) {
	// lock for editing
	std::lock_guard<std::mutex> guard(m_vNodeMutex);

	// nullcheck
	if (node == NULL) {
		glass3::util::Logger::log("warning",
								"CSite::addNode: NULL CNode provided.");
		return;
	}
	// check travel times
	/*
	 if ((travelTime1 < 0) && (travelTime2 < 0)) {
	 glass3::util::Logger::log("error",
	 "CSite::addNode: No valid travel times.");
	 return;
	 }
	 */

	// add node link to vector of nodes linked to this site
	// NOTE: no duplication check, but multiple nodes from the
	// same web can exist at the same site (travel times would be different)
	NodeLink link = std::make_tuple(node, travelTime1, travelTime2, distDeg);
	m_vNode.push_back(link);
}

// ---------------------------------------------------------removeNode
void CSite::removeNode(std::string nodeID) {
	// lock for editing
	std::lock_guard<std::mutex> guard(m_vNodeMutex);

	// nullcheck
	if (nodeID == "") {
		glass3::util::Logger::log("warning",
								"CSite::removeNode: empty web name provided.");
		return;
	}

	// clean up expired pointers
	for (auto it = m_vNode.begin(); it != m_vNode.end();) {
		if (std::get<LINK_PTR>(*it).expired() == true) {
			it = m_vNode.erase(it);
		} else {
			++it;
		}
	}

	for (auto it = m_vNode.begin(); it != m_vNode.end();) {
		if (auto aNode = std::get<LINK_PTR>(*it).lock()) {
			// erase target pick
			if (aNode->getID() == nodeID) {
				it = m_vNode.erase(it);
				return;
			} else {
				++it;
			}
		} else {
			++it;
		}
	}
}

// ---------------------------------------------------------nucleate
std::vector<std::shared_ptr<CTrigger>> CSite::nucleate(double tPick) {
	std::lock_guard<std::mutex> guard(m_vNodeMutex);

	// create trigger vector
	std::vector<std::shared_ptr<CTrigger>> vTrigger;

	// are we enabled?
	m_SiteMutex.lock();
	if (m_bUse == false) {
		m_SiteMutex.unlock();
		return (vTrigger);
	}
	m_SiteMutex.unlock();

	// for each node linked to this site
	for (const auto &link : m_vNode) {
		// compute potential origin time from tPick and travel time to node
		// first get traveltime1 to node
		double travelTime1 = std::get< LINK_TT1>(link);

		// second get traveltime2 to node
		double travelTime2 = std::get< LINK_TT2>(link);

		// third get shared pointer to node
		std::shared_ptr<CNode> node = std::get<LINK_PTR>(link).lock();

		if (node == NULL) {
			continue;
		}

		if (node->getEnabled() == false) {
			continue;
		}

		// compute first origin time
		double tOrigin1 = -1;
		if (travelTime1 > 0) {
			tOrigin1 = tPick - travelTime1;
		}

		// compute second origin time
		double tOrigin2 = -1;
		if (travelTime2 > 0) {
			tOrigin2 = tPick - travelTime2;
		}

		// attempt to nucleate an event located
		// at the current node with the potential origin times
		bool primarySuccessful = false;
		if (tOrigin1 > 0) {
			std::shared_ptr<CTrigger> trigger1 = node->nucleate(tOrigin1);

			if (trigger1 != NULL) {
				// if node triggered, add to triggered vector
				addTriggerToList(&vTrigger, trigger1);
				primarySuccessful = true;
			}
		}

		// only attempt secondary phase nucleation if primary nucleation
		// was unsuccessful
		if ((primarySuccessful == false) && (tOrigin2 > 0)) {
			std::shared_ptr<CTrigger> trigger2 = node->nucleate(tOrigin2);

			if (trigger2 != NULL) {
				// if node triggered, add to triggered vector
				addTriggerToList(&vTrigger, trigger2);
			}
		}

		if ((tOrigin1 < 0) && (tOrigin2 < 0)) {
			glass3::util::Logger::log(
					"warning",
					"CSite::nucleate: " + m_sSCNL + " No valid travel times. ("
							+ std::to_string(travelTime1) + ", "
							+ std::to_string(travelTime2) + ") web: "
							+ node->getWeb()->getName());
		}
	}

	return (vTrigger);
}

// ---------------------------------------------------------addTrigger
void CSite::addTriggerToList(std::vector<std::shared_ptr<CTrigger>> *vTrigger,
								std::shared_ptr<CTrigger> trigger) {
	if (trigger == NULL) {
		return;
	}
	if (trigger->getWeb() == NULL) {
		return;
	}

	// clean up expired pointers
	for (auto it = vTrigger->begin(); it != vTrigger->end();) {
		std::shared_ptr<CTrigger> aTrigger = (*it);

		// if current trigger is part of latest trigger's web
		if (trigger->getWeb()->getName() == aTrigger->getWeb()->getName()) {
			// if current trigger's sum is less than latest trigger's sum
			if (trigger->getBayesValue() > aTrigger->getBayesValue()) {
				it = vTrigger->erase(it);
				it = vTrigger->insert(it, trigger);
			}

			// we're done
			return;
		} else {
			++it;
		}
	}

	// add triggering node to vector of triggered nodes
	vTrigger->push_back(trigger);
}

// ---------------------------------------------------------getNodeLinksCount
int CSite::getNodeLinksCount() const {
	std::lock_guard<std::mutex> guard(m_vNodeMutex);
	return (m_vNode.size());
}

// ---------------------------------------------------------getEnable
bool CSite::getEnable() const {
	return (m_bEnable);
}

// ---------------------------------------------------------setEnable
void CSite::setEnable(bool enable) {
	m_bEnable = enable;
}

// ---------------------------------------------------------getUse
bool CSite::getUse() const {
	return (m_bUse && m_bEnable);
}

// ---------------------------------------------------------setUse
void CSite::setUse(bool use) {
	m_bUse = use;
}

// ---------------------------------------------------------getUseForTeleseismic
bool CSite::getUseForTeleseismic() const {
	return (m_bUseForTeleseismic);
}

// ---------------------------------------------------------setUseForTeleseismic
void CSite::setUseForTeleseismic(bool useForTele) {
	m_bUseForTeleseismic = useForTele;
}

// ---------------------------------------------------------getQuality
double CSite::getQuality() const {
	return (m_dQuality);
}

// ---------------------------------------------------------setQuality
void CSite::setQuality(double qual) {
	m_dQuality = qual;
}

// ---------------------------------------------------------getGeo
glass3::util::Geo &CSite::getGeo() {
	return (m_Geo);
}

// ---------------------------------------------------------getComponent
const std::string& CSite::getComponent() const {
	return (m_sComponent);
}

// ---------------------------------------------------------getLocation
const std::string& CSite::getLocation() const {
	return (m_sLocation);
}

// ---------------------------------------------------------getNetwork
const std::string& CSite::getNetwork() const {
	return (m_sNetwork);
}

// ---------------------------------------------------------getSCNL
const std::string& CSite::getSCNL() const {
	return (m_sSCNL);
}

// ---------------------------------------------------------getSite
const std::string& CSite::getSite() const {
	return (m_sSite);
}

// ---------------------------------------------------------getTLastPickAdded
time_t CSite::getTLastPickAdded() const {
	return (m_tLastPickAdded);
}

// ------------------------------------------------------setPickCountSinceCheck
void CSite::setPickCountSinceCheck(int count) {
	m_iPickCountSinceCheck = count;
}

// ------------------------------------------------------getPickCountSinceCheck
int CSite::getPickCountSinceCheck() const {
	return (m_iPickCountSinceCheck);
}

// ------------------------------------------------------getPickCount
int CSite::getPickCount() const {
	std::lock_guard<std::recursive_mutex> guard(m_SiteMutex);
	return (m_msPickList.size());
}

// --------------------------------------------------------updatePosition
void CSite::updatePosition(std::shared_ptr<CPick> pick) {
	// nullchecks
	if (pick == NULL) {
		return;
	}
	if (pick->getSite()->getSCNL() != getSCNL()) {
		return;
	}

	std::lock_guard<std::recursive_mutex> listGuard(m_SiteMutex);

	// from my research, the best way to "update" the position of an item
	// in a multiset when the key value has changed (in this case, the pick
	// time) is to remove and re-add the item. This will give us O(log n)
	// complexity for updating one item, which is better than a full sort
	// (which I'm not really sure how to do on a multiset)
	// erase
	eraseFromMultiset(pick);

	// update tSort
	pick->setTSort(pick->getTPick());

	// insert
	m_msPickList.insert(pick);
}

// ---------------------------------------------------------eraseFromMultiset
void CSite::eraseFromMultiset(std::shared_ptr<CPick> pick) {
	// nullchecks
	if (pick == NULL) {
		return;
	}
	if (pick->getSite()->getSCNL() != getSCNL()) {
		return;
	}

	std::lock_guard<std::recursive_mutex> listGuard(m_SiteMutex);

	if (m_msPickList.size() == 0) {
		return;
	}

	// first, try to delete the hypo the efficient way
	// we need to be careful, because multiple hypos in the mulitset
	// can have the same tSort, and a simple erase would delete
	// them all, which would be BAD, so we need to confirm the id
	auto lower = m_msPickList.lower_bound(pick);
	auto upper = m_msPickList.upper_bound(pick);

	// for all matching (tSort range) hypos
	for (auto it = lower; ((it != upper) && (it != m_msPickList.end())); ++it) {
		std::shared_ptr<CPick> aPick = *it;

		// only erase the correct one
		if (aPick->getID() == pick->getID()) {
			m_msPickList.erase(it);
			return;
		}
	}

	glass3::util::Logger::log(
			"warning",
			"CSite::eraseFromMultiset: efficient delete for pick "
					+ pick->getID() + " didn't work.");

	// if we didn't delete it efficiently, loop through all picks, I know this is
	// brute force, but the efficient delete didn't work and we want to be sure
	// note: this may just be me being paranoid
	for (auto it = m_msPickList.begin(); (it != m_msPickList.end()); ++it) {
		std::shared_ptr<CPick> aPick = *it;

		// only erase the correct one
		if (aPick->getID() == pick->getID()) {
			m_msPickList.erase(it);
			return;
		}
	}

	glass3::util::Logger::log(
			"error",
			"CSite::eraseFromMultiset: did not delete pick " + pick->getID()
					+ " in multiset, id not found.");
}

}  // namespace glasscore

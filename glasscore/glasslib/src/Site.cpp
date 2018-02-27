#include <json.h>
#include <sstream>
#include <cmath>
#include <utility>
#include <tuple>
#include <string>
#include <memory>
#include <vector>
#include <algorithm>
#include <mutex>
#include "Glass.h"
#include "Pick.h"
#include "Site.h"
#include "Logit.h"
#include "Node.h"
#include "Trigger.h"
#include "Web.h"

namespace glasscore {

std::vector<std::string> &split(const std::string &s, char delim,
								std::vector<std::string> &elems) {  // NOLINT
	std::string item;

	// convert to stringstream
	std::stringstream ss(s);

	// search through string looking for delimiter
	while (std::getline(ss, item, delim)) {
		// add substring to list
		elems.push_back(item);
	}
	// return list
	return (elems);
}

std::vector<std::string> split(const std::string &s, char delim) {
	std::vector<std::string> elems;

	// split using string and delimiter
	split(s, delim, elems);

	// return list
	return (elems);
}

// ---------------------------------------------------------CSite
CSite::CSite() {
	clear();
}

// ---------------------------------------------------------CSite
CSite::CSite(std::string sta, std::string comp, std::string net,
				std::string loc, double lat, double lon, double elv,
				double qual, bool enable, bool useTele, CGlass *glassPtr) {
	// pass to initialization function
	initialize(sta, comp, net, loc, lat, lon, elv, qual, enable, useTele,
				glassPtr);
}

// ---------------------------------------------------------CSite
CSite::CSite(std::shared_ptr<json::Object> site, CGlass *glassPtr) {
	clear();

	// null check json
	if (site == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CSite::CSite: NULL json site.");
		return;
	}

	// check type
	if (site->HasKey("Type")
			&& ((*site)["Type"].GetType() == json::ValueType::StringVal)) {
		std::string type = (*site)["Type"].ToString();

		if (type != "StationInfo") {
			glassutil::CLogit::log(
					glassutil::log_level::warn,
					"CSite::CSite: Non-StationInfo message passed in.");
			return;
		}
	} else {
		glassutil::CLogit::log(glassutil::log_level::error,
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
			glassutil::CLogit::log(
					glassutil::log_level::error,
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
			glassutil::CLogit::log(
					glassutil::log_level::error,
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
		glassutil::CLogit::log(glassutil::log_level::error,
								"CSite::CSite: Missing required Site Object.");

		return;
	}

	// latitude for this site
	if (((*site).HasKey("Latitude"))
			&& ((*site)["Latitude"].GetType() == json::ValueType::DoubleVal)) {
		latitude = (*site)["Latitude"].ToDouble();
	} else {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CSite::CSite: Missing required Latitude Key.");

		return;
	}

	// longitude for this site
	if (((*site).HasKey("Longitude"))
			&& ((*site)["Longitude"].GetType() == json::ValueType::DoubleVal)) {
		longitude = (*site)["Longitude"].ToDouble();
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"CSite::CSite: Missing required Longitude Key.");

		return;
	}

	// elevation for this site
	if (((*site).HasKey("Elevation"))
			&& ((*site)["Elevation"].GetType() == json::ValueType::DoubleVal)) {
		elevation = (*site)["Elevation"].ToDouble();
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::error,
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
				elevation, quality, enable, useForTelesiesmic, glassPtr);
}

// ---------------------------------------------------------CSite
bool CSite::initialize(std::string sta, std::string comp, std::string net,
						std::string loc, double lat, double lon, double elv,
						double qual, bool enable, bool useTele,
						CGlass *glassPtr) {
	clear();

	std::lock_guard<std::recursive_mutex> guard(siteMutex);

	// generate scnl
	sScnl = "";

	// station, required
	if (sta != "") {
		sScnl += sta;
	} else {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CSite::initialize: missing sSite.");
		return (false);
	}

	// component, optional
	if (comp != "") {
		sScnl += "." + comp;
	}

	// network, required
	if (net != "") {
		sScnl += "." + net;
	} else {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CSite::initialize: missing sNet.");
		return (false);
	}

	// location, optional
	if (loc != "") {
		sScnl += "." + loc;
	}

	// fill in site/net/etc
	sSite = sta;
	sNet = net;
	sComp = comp;
	sLoc = loc;

	// set geographic location
	setLocation(lat, lon, -0.001 * elv);

	// quality
	dQual = qual;

	// copy use
	bUse = enable;
	bUseForTele = useTele;

	// pointer to main glass class
	pGlass = glassPtr;

	if (pGlass) {
		nSitePickMax = pGlass->getSitePickMax();
	}

	return (true);
}

// ---------------------------------------------------------~CSite
CSite::~CSite() {
	clear();
}

void CSite::clear() {
	std::lock_guard<std::recursive_mutex> guard(siteMutex);
	// clear scnl
	sScnl = "";
	sSite = "";
	sComp = "";
	sNet = "";
	sLoc = "";

	bUse = true;
	bUseForTele = true;
	dQual = 1.0;

	pGlass = NULL;

	// clear geographic
	geo = glassutil::CGeo();
	dVec[0] = 0;
	dVec[1] = 0;
	dVec[2] = 0;

	// clear lists
	vNodeMutex.lock();
	vNode.clear();
	vNodeMutex.unlock();

	clearVPick();

	// reset max picks
	nSitePickMax = 200;
}

void CSite::clearVPick() {
	vPickMutex.lock();
	vPick.clear();
	vPickMutex.unlock();
}

void CSite::update(CSite *site) {
	std::lock_guard<std::recursive_mutex> guard(siteMutex);
	// scnl check
	if (sScnl != site->sScnl) {
		return;
	}

	// update station quality metrics
	bUse = site->bUse;
	bUseForTele = site->bUseForTele;
	dQual = site->dQual;

	// update location
	geo = glassutil::CGeo(site->geo);
	dVec[0] = site->dVec[0];
	dVec[1] = site->dVec[1];
	dVec[2] = site->dVec[2];

	// leave lists, and pointers alone
}

// ---------------------------------------------------------setLocation
void CSite::setLocation(double lat, double lon, double z) {
	std::lock_guard<std::recursive_mutex> guard(siteMutex);
	// construct unit vector in cartesian earth coordinates
	double rxy = cos(DEG2RAD * lat);
	dVec[0] = rxy * cos(DEG2RAD * lon);
	dVec[1] = rxy * sin(DEG2RAD * lon);
	dVec[2] = sin(DEG2RAD * lat);

	// set geographic object
	geo.setGeographic(lat, lon, 6371.0 - z);
}

// ---------------------------------------------------------getDelta
double CSite::getDelta(glassutil::CGeo *geo2) {
	// nullcheck
	if (geo2 == NULL) {
		glassutil::CLogit::log(glassutil::log_level::warn,
								"CSite::getDelta: NULL CGeo provided.");
		return (0);
	}

	// use CGeo to calculate distance in radians
	return (geo.delta(geo2));
}

// ---------------------------------------------------------getDistance
double CSite::getDistance(std::shared_ptr<CSite> site) {
	// nullcheck
	if (site == NULL) {
		glassutil::CLogit::log(glassutil::log_level::warn,
								"CSite::getDelta: NULL CSite provided.");
		return (0);
	}

	// use unit vectors in cartesian earth coordinates
	// to quickly great circle distance in km
	double dot = 0;
	double dkm;
	for (int i = 0; i < 3; i++) {
		dot += dVec[i] * site->dVec[i];
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
		glassutil::CLogit::log(glassutil::log_level::warn,
								"CSite::addPick: NULL CPick provided.");
		return;
	}

	// ensure this pick is for this site
	if (pck->getSite()->sScnl != sScnl) {
		glassutil::CLogit::log(
				glassutil::log_level::warn,
				"CSite::addPick: CPick for different site: (" + sScnl + "!="
						+ pck->getSite()->sScnl + ")");
		return;
	}

	// check to see if we're at the pick limit
	if (vPick.size() == nSitePickMax) {
		// erase first pick from vector
		vPick.erase(vPick.begin());
	}

	// add pick to site pick vector
	// NOTE: Need to add duplicate pick protection
	std::weak_ptr<CPick> wpPck = pck;
	vPick.push_back(wpPck);
}

// ---------------------------------------------------------remPick
void CSite::remPick(std::shared_ptr<CPick> pck) {
	// lock for editing
	std::lock_guard<std::mutex> guard(vPickMutex);

	// nullcheck
	if (pck == NULL) {
		glassutil::CLogit::log(glassutil::log_level::warn,
								"CSite::remPick: NULL CPick provided.");
		return;
	}

	// remove pick from site pick vector
	for (auto it = vPick.begin(); it != vPick.end();) {
		if ((*it).expired() == true) {
			// clean up expired pointers
			it = vPick.erase(it);
		} else if (auto aPck = (*it).lock()) {
			// erase target pick
			if (aPck->getPid() == pck->getPid()) {
				it = vPick.erase(it);
			} else {
				++it;
			}
		} else {
			++it;
		}
	}
}

// ---------------------------------------------------------addNode
void CSite::addNode(std::shared_ptr<CNode> node, double travelTime1,
					double travelTime2) {
	// lock for editing
	std::lock_guard<std::mutex> guard(vNodeMutex);

	// nullcheck
	if (node == NULL) {
		glassutil::CLogit::log(glassutil::log_level::warn,
								"CSite::addNode: NULL CNode provided.");
		return;
	}
	// check travel times
	/*
	 if ((travelTime1 < 0) && (travelTime2 < 0)) {
	 glassutil::CLogit::log(glassutil::log_level::error,
	 "CSite::addNode: No valid travel times.");
	 return;
	 }
	 */

	// add node link to vector of nodes linked to this site
	// NOTE: no duplication check, but multiple nodes from the
	// same web can exist at the same site (travel times would be different)
	NodeLink link = std::make_tuple(node, travelTime1, travelTime2);
	vNode.push_back(link);
}

// ---------------------------------------------------------remNode
void CSite::remNode(std::string nodeID) {
	// lock for editing
	std::lock_guard<std::mutex> guard(vNodeMutex);

	// nullcheck
	if (nodeID == "") {
		glassutil::CLogit::log(glassutil::log_level::warn,
								"CSite::remNode: empty web name provided.");
		return;
	}

	// clean up expired pointers
	for (auto it = vNode.begin(); it != vNode.end();) {
		if (std::get<LINK_PTR>(*it).expired() == true) {
			it = vNode.erase(it);
		} else {
			++it;
		}
	}

	for (auto it = vNode.begin(); it != vNode.end();) {
		if (auto aNode = std::get<LINK_PTR>(*it).lock()) {
			// erase target pick
			if (aNode->getPid() == nodeID) {
				it = vNode.erase(it);
				return;
			} else {
				++it;
			}
		} else {
			++it;
		}
	}
}

// ---------------------------------------------------------Nucleate
std::vector<std::shared_ptr<CTrigger>> CSite::nucleate(double tPick) {
	std::lock_guard<std::mutex> guard(vNodeMutex);

	// create trigger vector
	std::vector<std::shared_ptr<CTrigger>> vTrigger;

	// for each node linked to this site
	for (const auto &link : vNode) {
		// compute potential origin time from tpick and traveltime to node
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
				addTrigger(&vTrigger, trigger1);
				primarySuccessful = true;
			}
		}

		// only attempt secondary phase nucleation if primary nucleation
		// was unsuccessful
		if ((primarySuccessful == false) && (tOrigin2 > 0)) {
			std::shared_ptr<CTrigger> trigger2 = node->nucleate(tOrigin2);

			if (trigger2 != NULL) {
				// if node triggered, add to triggered vector
				addTrigger(&vTrigger, trigger2);
			}
		}

		if ((tOrigin1 < 0) && (tOrigin2 < 0)) {
			glassutil::CLogit::log(
					glassutil::log_level::warn,
					"CSite::nucleate: " + sScnl + " No valid travel times. ("
							+ std::to_string(travelTime1) + ", "
							+ std::to_string(travelTime2) + ") web: "
							+ node->getWeb()->getName());
		}
	}

	return (vTrigger);
}

// ---------------------------------------------------------addTrigger
void CSite::addTrigger(std::vector<std::shared_ptr<CTrigger>> *vTrigger,
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
			if (trigger->getSum() > aTrigger->getSum()) {
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

int CSite::getNodeLinksCount() const {
	std::lock_guard<std::mutex> guard(vNodeMutex);
	int size = vNode.size();

	return (size);
}

bool CSite::getUse() const {
	std::lock_guard<std::recursive_mutex> guard(siteMutex);
	return (bUse);
}

void CSite::setUse(bool use) {
	std::lock_guard<std::recursive_mutex> guard(siteMutex);
	bUse = use;
}

bool CSite::getUseForTele() const {
	std::lock_guard<std::recursive_mutex> guard(siteMutex);
	return (bUseForTele);
}

void CSite::setUseForTele(bool useForTele) {
	std::lock_guard<std::recursive_mutex> guard(siteMutex);
	bUseForTele = useForTele;
}

double CSite::getQual() const {
	std::lock_guard<std::recursive_mutex> guard(siteMutex);
	return (dQual);
}

void CSite::setQual(double qual) {
	std::lock_guard<std::recursive_mutex> guard(siteMutex);
	dQual = qual;
}

glassutil::CGeo& CSite::getGeo() {
	return (geo);
}

int CSite::getSitePickMax() const {
	return (nSitePickMax);
}

CGlass* CSite::getGlass() const {
	return (pGlass);
}

const std::string& CSite::getComp() const {
	return (sComp);
}

const std::string& CSite::getLoc() const {
	return (sLoc);
}

const std::string& CSite::getNet() const {
	return (sNet);
}

const std::string& CSite::getScnl() const {
	return (sScnl);
}

const std::string& CSite::getSite() const {
	return (sSite);
}

const std::vector<std::shared_ptr<CPick> > CSite::getVPick() const {
	std::lock_guard<std::mutex> guard(vPickMutex);

	std::vector<std::shared_ptr<CPick>> picks;
	for (auto awpPck : vPick) {
		std::shared_ptr<CPick> aPck = awpPck.lock();

		if (aPck != NULL) {
			picks.push_back(aPck);
		}
	}

	return (picks);
}
}  // namespace glasscore

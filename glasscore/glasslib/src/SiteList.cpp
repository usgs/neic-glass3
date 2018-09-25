#include "SiteList.h"
#include <json.h>
#include <logger.h>
#include <cmath>
#include <string>
#include <memory>
#include <mutex>
#include <vector>
#include <ctime>
#include "Glass.h"
#include "Site.h"
#include "WebList.h"
#include "Node.h"
#include "Web.h"

namespace glasscore {

// ---------------------------------------------------------CSiteList
CSiteList::CSiteList(int numThreads, int sleepTime, int checkInterval)
		: glass3::util::ThreadBaseClass("SiteList", sleepTime, numThreads,
										checkInterval) {
	clear();

	// start up the thread
	start();
}

// ---------------------------------------------------------~CSiteList
CSiteList::~CSiteList() {
	clear();
}

// ---------------------------------------------------------clear
void CSiteList::clear() {
	std::lock_guard<std::recursive_mutex> siteListGuard(m_SiteListMutex);

	// clear sites
	for (auto site : m_vSite) {
		site->clear();
	}

	// clear the vector and map
	m_vSite.clear();
	m_mSite.clear();
	m_mLastTimeSiteLookedUp.clear();

	m_iMaxHoursWithoutPicking = -1;
	m_iHoursBeforeLookingUp = -1;
	m_iMaxPicksPerHour = -1;
	m_tLastChecked = std::time(NULL);
}

// -------------------------------------------------------receiveExternalMessage
bool CSiteList::receiveExternalMessage(std::shared_ptr<json::Object> com) {
	// null check json
	if (com == NULL) {
		glass3::util::Logger::log(
				"error",
				"CSiteList::receiveExternalMessage: NULL json communication.");
		return (false);
	}

	// check for a command
	if (com->HasKey("Cmd")
			&& ((*com)["Cmd"].GetType() == json::ValueType::StringVal)) {
		// dispatch to appropriate function based on Cmd value
		json::Value v = (*com)["Cmd"].ToString();

		// get the current site list
		if (v == "ReqSiteList") {
			generateSiteListMessage();
			return (true);
		}
	}

	// Input data can have Type keys
	if (com->HasKey("Type")
			&& ((*com)["Type"].GetType() == json::ValueType::StringVal)) {
		// dispatch to appropriate function based on Cmd value
		json::Value v = (*com)["Type"].ToString();

		// add or update a site
		if (v == "StationInfo") {
			return (addSiteFromJSON(com));
		}
		// add a list of sites
		if (v == "StationInfoList") {
			return (addListOfSitesFromJSON(com));
		}
	}

	// this communication was not handled
	return (false);
}

// ---------------------------------------------------------addSiteFromJSON
bool CSiteList::addSiteFromJSON(std::shared_ptr<json::Object> com) {
	// null check json
	if (com == NULL) {
		glass3::util::Logger::log("error",
								"CSiteList::addSite: NULL json configuration.");
		return (false);
	}

	// check type
	if (com->HasKey("Type")
			&& ((*com)["Type"].GetType() == json::ValueType::StringVal)) {
		std::string type = (*com)["Type"].ToString();

		if (type != "StationInfo") {
			glass3::util::Logger::log(
					"warning",
					"CSiteList::addSite: Non-StationInfo message passed in.");
			return (false);
		}
	} else {
		glass3::util::Logger::log(
				"error",
				"CSiteList::addSite: Missing required Cmd Key.");
	}

	// create a new a site from the json message;
	CSite * site = new CSite(com);

	// make sure a site was actually created
	if (site->getSCNL() == "") {
		glass3::util::Logger::log("warning",
								"CSiteList::addSite: Site not created.");
		delete (site);
		return (false);
	}

	// create new shared pointer to the site
	std::shared_ptr<CSite> newSite(site);

	return (addSite(newSite));
}

// ---------------------------------------------------------addSiteListFromJSON
bool CSiteList::addListOfSitesFromJSON(std::shared_ptr<json::Object> com) {
	// null check json
	if (com == NULL) {
		glass3::util::Logger::log(
				"error",
				"CSiteList::addSiteList: NULL json configuration.");
		return (false);
	}

	// check type
	if (com->HasKey("Type")
			&& ((*com)["Type"].GetType() == json::ValueType::StringVal)) {
		std::string type = (*com)["Type"].ToString();

		if (type != "StationInfoList") {
			glass3::util::Logger::log(
					"warning",
					"CSiteList::addSite: Non-StationInfoList message passed"
					" in.");
			return (false);
		}
	} else {
		glass3::util::Logger::log(
				"error",
				"CSiteList::addSite: Missing required Type Key.");
	}

	// get the list
	if (((*com).HasKey("StationList"))
			&& ((*com)["StationList"].GetType() == json::ValueType::ArrayVal)) {
		json::Array stationList = (*com)["StationList"].ToArray();
		// for each station in the list
		for (auto v : stationList) {
			if (v.GetType() == json::ValueType::ObjectVal) {
				// convert object to json pointer
				std::shared_ptr<json::Object> siteObj = std::make_shared<
						json::Object>(v.ToObject());

				// create a new a site from the station json;
				CSite * site = new CSite(siteObj);

				// make sure a site was actually created
				if (site->getSCNL() == "") {
					glass3::util::Logger::log(
							"warning",
							"CSiteList::addSiteList: Site not created.");
					delete (site);
					continue;
				}

				// create new shared pointer to the site
				std::shared_ptr<CSite> newSite(site);

				if (addSite(newSite) == false) {
					glass3::util::Logger::log(
							"warning",
							"CSiteList::addSiteList: Site not added.");
					delete (site);
					continue;
				}
			}
		}
	}

	return (true);
}

// ---------------------------------------------------------addSite
bool CSiteList::addSite(std::shared_ptr<CSite> site) {
	// null check
	if (site == NULL) {
		glass3::util::Logger::log("error",
								"CSiteList::addSite: NULL CSite provided.");
		return (false);
	}

	// check to see if we have an existing site
	std::shared_ptr<CSite> oldSite = getSite(site->getSCNL());

	std::lock_guard<std::recursive_mutex> guard(m_SiteListMutex);

	// check if we already had this site
	if (oldSite) {
		// update existing site
		oldSite->update(site.get());

		// pass updated site to webs
		if (CGlass::getWebList()) {
			CGlass::getWebList()->addSite(oldSite);
		}
	} else {
		// add new site to list and map
		m_vSite.push_back(site);
		m_mSite[site->getSCNL()] = site;

		// pass new site to webs
		if (CGlass::getWebList()) {
			CGlass::getWebList()->addSite(site);
		}
	}

	// what time is it
	time_t tNow;
	std::time(&tNow);

	// since we've just added or updated
	// set the lookup time to now
	m_mLastTimeSiteLookedUp[site->getSCNL()] = tNow;

	return (true);
}

// ---------------------------------------------------------getSite
std::shared_ptr<CSite> CSiteList::getSite(std::string scnl) {
	if (scnl == "") {
		glass3::util::Logger::log("error",
								"CSiteList::getSite: Empty site passed in.");
		return (NULL);
	}

	std::lock_guard<std::recursive_mutex> guard(m_SiteListMutex);

	// lookup the site in the map by scnl
	auto itsite = m_mSite.find(scnl);
	if (itsite != m_mSite.end()) {
		return (m_mSite[scnl]);
	}

	// nothing found
	return (NULL);
}

// ---------------------------------------------------------getSite
std::shared_ptr<CSite> CSiteList::getSite(std::string site, std::string comp,
											std::string net, std::string loc) {
	if (site == "") {
		glass3::util::Logger::log("error",
								"CSiteList::getSite: Empty site passed in.");
		return (NULL);
	}

	if (net == "") {
		glass3::util::Logger::log("error",
								"CSiteList::getSite: Empty network passed in.");
		return (NULL);
	}

	// generate SCNL the same way that it is built in CSite::initialize
	std::string scnl = "";

	// station, required
	if (site != "") {
		scnl += site;
	} else {
		glass3::util::Logger::log("error",
								"CSiteList::getSite: missing sSite.");
		return (NULL);
	}

	// component, optional
	if (comp != "") {
		scnl += "." + comp;
	}

	// network, required
	if (net != "") {
		scnl += "." + net;
	} else {
		glass3::util::Logger::log("error",
								"CSiteList::getSite: missing sNet.");
		return (NULL);
	}

	// location, optional, -- is used by some networks to represent an empty
	// location code
	if (loc == "--") {
		loc = "";
	}
	if (loc != "") {
		scnl += "." + loc;
	}

	// send request for information about this station
	// this section is above the getSite() call so that
	// we could be constantly requesting new station information,
	// which seems useful.
	if (m_iHoursBeforeLookingUp >= 0) {
		// what time is it
		time_t tNow;
		std::time(&tNow);

		// get what time this station has been looked up before
		int tLookup = 0;
		auto itsite = m_mLastTimeSiteLookedUp.find(scnl);
		if (itsite != m_mLastTimeSiteLookedUp.end()) {
			tLookup = m_mLastTimeSiteLookedUp[scnl];
		}

		// only ask for a station occasionally
		if ((tNow - tLookup) > (60 * 60 * m_iHoursBeforeLookingUp)) {
			// construct request json message
			std::shared_ptr<json::Object> request = std::make_shared<
					json::Object>(json::Object());
			(*request)["Type"] = "SiteLookup";
			(*request)["Site"] = site;
			(*request)["Comp"] = comp;
			(*request)["Net"] = net;
			(*request)["Loc"] = loc;

			// log
			char sLog[1024];
			snprintf(sLog, sizeof(sLog), "CSiteList::getSite: SCNL:%s, "
						"requesting information.",
						scnl.c_str());
			glass3::util::Logger::log(sLog);

			// send request
			CGlass::sendExternalMessage(request);

			// remember when we tried
			m_mLastTimeSiteLookedUp[scnl] = tNow;
		}
	}

	// lookup the site from the map
	std::shared_ptr<CSite> foundSite = getSite(scnl);
	if (foundSite != NULL) {
		return (foundSite);
	}

	// site not found
	return (NULL);
}

// ---------------------------------------------------------getListOfSites
std::vector<std::shared_ptr<CSite>> CSiteList::getListOfSites() {
	std::lock_guard<std::recursive_mutex> guard(m_SiteListMutex);

	std::vector<std::shared_ptr<CSite>> siteList;

	// move through whole vector
	for (const auto &site : m_vSite) {
		siteList.push_back(site);
	}

	// return what we got
	return (siteList);
}

// ------------------------------------------------------generateSiteListMessage
std::shared_ptr<json::Object> CSiteList::generateSiteListMessage(bool send) {
	std::shared_ptr<json::Object> sitelistObj = std::make_shared<json::Object>(
			json::Object());
	(*sitelistObj)["Cmd"] = "SiteList";

	// array to hold data
	json::Array stationList;

	std::lock_guard<std::recursive_mutex> guard(m_SiteListMutex);

	// move through whole vector
	for (const auto &site : m_vSite) {
		json::Object stationObj;
		double lat;
		double lon;
		double z;

		// convert to geographic coordinates
		site->getGeo().getGeographic(&lat, &lon, &z);

		// convert from earth radius to elevation
		double elv = (z - 6317.0) * -1000.0;

		// construct a json message containing our site info
		stationObj["Lat"] = lat;
		stationObj["Lon"] = lon;
		stationObj["Z"] = elv;
		stationObj["Qual"] = site->getQuality();
		stationObj["Use"] = site->getEnable();
		stationObj["UseForTele"] = site->getUseForTeleseismic();

		stationObj["Sta"] = site->getSite();
		if (site->getComponent() != "") {
			stationObj["Comp"] = site->getComponent();
		}
		stationObj["Net"] = site->getNetwork();
		if (site->getLocation() != "") {
			stationObj["Loc"] = site->getLocation();
		}

		// add to station list
		stationList.push_back(stationObj);
	}

	(*sitelistObj)["SiteList"] = stationList;

	if (send == true) {
		CGlass::sendExternalMessage(sitelistObj);
	}

	return (sitelistObj);
}

// ------------------------------------------------------size
int CSiteList::size() const {
	std::lock_guard<std::recursive_mutex> vSiteGuard(m_SiteListMutex);
	return (m_vSite.size());
}

// ------------------------------------------------------work
glass3::util::WorkState CSiteList::work() {
	// don't bother if we're not configured to check sites
	if ((getMaxHoursWithoutPicking() < 0) && (getHoursBeforeLookingUp() < 0)
			&& (getMaxPicksPerHour() < 0)) {
		return (glass3::util::WorkState::Idle);
	}

	if (size() <= 0) {
		return (glass3::util::WorkState::Idle);
	}

	// what time is it
	time_t tNow;
	std::time(&tNow);

	// check every hour
	// NOTE: hardcoded to one hour, any more often seemed excessive
	// didn't seem like a parameter that would be changed
	if ((tNow - m_tLastChecked) < (60 * 60)) {
		// no
		return (glass3::util::WorkState::Idle);
	}

	std::lock_guard<std::recursive_mutex> siteListGuard(m_SiteListMutex);

	// remember when we last checked
	m_tLastChecked = tNow;

	glass3::util::Logger::log("debug",
							"CSiteList::work: checking for sites not picking");

	// for each used site in the site list
	for (auto aSite : m_vSite) {
		// skip disabled sites
		if (aSite->getEnable() == false) {
			continue;
		}
		// skip sites that are not used
		if (aSite->getUse() == false) {
			continue;
		}

		bool disableSite = false;

		// check for sites that are not picking
		if (m_iMaxHoursWithoutPicking > 0) {
			// when was the last pick added to this site
			time_t tLastPickAdded = aSite->getTLastPickAdded();

			// have we not seen data?
			if ((tNow - tLastPickAdded)
					> (60 * 60 * m_iMaxHoursWithoutPicking)) {
				glass3::util::Logger::log(
						"debug",
						"CSiteList::work: Removing " + aSite->getSCNL()
								+ " for not picking in "
								+ std::to_string(tNow - tLastPickAdded)
								+ " seconds ");
				disableSite = true;
			}
		}

		// check for sites that are picking too much
		if (m_iMaxPicksPerHour > 0) {
			// how many picks since last check
			int picksSinceCheck = aSite->getPickCountSinceCheck();

			// we check every hour, so picks since check is picks per hour
			if (picksSinceCheck > m_iMaxPicksPerHour) {
				glass3::util::Logger::log(
						"debug",
						"CSiteList::work: Removing " + aSite->getSCNL()
								+ " for picking more than "
								+ std::to_string(m_iMaxPicksPerHour)
								+ " in the last hour ("
								+ std::to_string(picksSinceCheck) + ")");
				disableSite = true;
			}

			// reset for next check
			aSite->setPickCountSinceCheck(0);
		}

		if (disableSite == true) {
			// disable the site
			aSite->setUse(false);

			// remove site from webs
			if (CGlass::getWebList()) {
				CGlass::getWebList()->removeSite(aSite);
			}
		}

		// update thread status
		setThreadHealth();
	}

	// for each unused site in the site list
	for (auto aSite : m_vSite) {
		// skip disabled sites
		if (aSite->getEnable() == false) {
			continue;
		}
		// skip sites that are used
		if (aSite->getUse() == true) {
			continue;
		}

		bool enableSite = false;

		// check or sites that started picking
		if (m_iMaxHoursWithoutPicking > 0) {
			// when was the last pick added to this site
			time_t tLastPickAdded = aSite->getTLastPickAdded();

			// have we seen data?
			if ((tNow - tLastPickAdded)
					< (60 * 60 * m_iMaxHoursWithoutPicking)) {
				glass3::util::Logger::log(
						"debug",
						"CSiteList::work: Added " + aSite->getSCNL()
								+ " because it has picked within "
								+ std::to_string(tNow - tLastPickAdded)
								+ " seconds ");

				enableSite = true;
			}
		}

		// check for sites who's rate has fallen
		if (m_iMaxPicksPerHour > 0) {
			// how many picks since last check
			int picksSinceCheck = aSite->getPickCountSinceCheck();

			// we check every hour, so picks since check is picks per hour
			if (picksSinceCheck < m_iMaxPicksPerHour) {
				glass3::util::Logger::log(
						"debug",
						"CSiteList::work: Added " + aSite->getSCNL()
								+ " for picking less than "
								+ std::to_string(m_iMaxPicksPerHour)
								+ " in the last hour ("
								+ std::to_string(picksSinceCheck) + ")");
				enableSite = true;
			}

			// reset for next check
			aSite->setPickCountSinceCheck(0);
		}

		if (enableSite == true) {
			// enable the site
			aSite->setUse(true);

			// add site to webs
			if (CGlass::getWebList()) {
				CGlass::getWebList()->addSite(aSite);
			}
		}

		// update thread status
		setThreadHealth();
	}

	return (glass3::util::WorkState::OK);
}

// ----------------------------------------------------setMaxHoursWithoutPicking
void CSiteList::setMaxHoursWithoutPicking(int hoursWithoutPicking) {
	m_iMaxHoursWithoutPicking = hoursWithoutPicking;
}

// ----------------------------------------------------getMaxHoursWithoutPicking
int CSiteList::getMaxHoursWithoutPicking() const {
	return (m_iMaxHoursWithoutPicking);
}

// ----------------------------------------------------setHoursBeforeLookingUp
void CSiteList::setHoursBeforeLookingUp(int hoursBeforeLookingUp) {
	m_iHoursBeforeLookingUp = hoursBeforeLookingUp;
}

// ----------------------------------------------------getHoursBeforeLookingUp
int CSiteList::getHoursBeforeLookingUp() const {
	return (m_iHoursBeforeLookingUp);
}

// ----------------------------------------------------setMaxPicksPerHour
void CSiteList::setMaxPicksPerHour(int maxPicksPerHour) {
	m_iMaxPicksPerHour = maxPicksPerHour;
}

// ----------------------------------------------------getMaxPicksPerHour
int CSiteList::getMaxPicksPerHour() const {
	return (m_iMaxPicksPerHour);
}

}  // namespace glasscore

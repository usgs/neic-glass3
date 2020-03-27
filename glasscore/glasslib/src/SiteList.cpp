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

// constants
const int CSiteList::k_nHoursToSeconds;

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
	m_tLastUpdated = std::time(NULL);
	m_tCreated = std::time(NULL);
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

	// what time is it
	time_t tNow;
	std::time(&tNow);
	int siteCount = 0;
	int usedSiteCount = 0;

	// get the list from the json
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

				// check site statistics, to preset the site use flag as needed
				// check for a site that hasn't been picked in awhile
				// this is to avoid a flurry of dead sites taking themselves out of the
				// webs 1 hour after startup
				if ((m_iMaxHoursWithoutPicking > 0) && (newSite->getIsUsed() == true)
					  && (newSite->getTLastPickAdded() > 0)) {
					// have we not seen data?
					if ((tNow - newSite->getTLastPickAdded())
							> (k_nHoursToSeconds * m_iMaxHoursWithoutPicking)) {
						glass3::util::Logger::log(
							"debug",
							"CSiteList::addSiteList: Marking Site " + site->getSCNL()
							+ " not used due to exceeding MaxHoursWithoutPicking.");
						newSite->setUse(false);
					}
				}

				// NOTE: Placeholder code for if we decide to persist site picking rate
				// in the station list, currently it is not persisted.
				// check for a site that is picking too often
				// if ((m_iMaxPicksPerHour > 0) && (newSite->getIsUsed() == true)) {
				// how many picks since last check
				// if (newSite->getPickCountSinceCheck() > m_iMaxPicksPerHour) {
				// newSite->setUse(false);
				// }
				// }

				// add the new site to the list
				if (addSite(newSite) == false) {
					glass3::util::Logger::log(
							"warning",
							"CSiteList::addSiteList: Site " + site->getSCNL()
							+ " not added.");
					delete (site);
					continue;
				} else {
					siteCount++;
					if (newSite->getIsUsed() == true) {
						usedSiteCount++;
					}
				}
			}
		}
	}

	glass3::util::Logger::log(
							"debug",
							"CSiteList::addSiteList: Loaded " + std::to_string(siteCount)
							+ " sites; " + std::to_string(usedSiteCount) + " usable sites.");

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
			CGlass::getWebList()->updateSite(oldSite);
		}
	} else {
		// add new site to list and map
		m_vSite.push_back(site);
		m_mSite[site->getSCNL()] = site;

		// pass new site to webs
		if (CGlass::getWebList()) {
			CGlass::getWebList()->updateSite(site);
		}
	}

	// what time is it
	time_t tNow;
	std::time(&tNow);

	// list was modified
	m_tLastUpdated = tNow;

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

	// lookup the site from the map
	std::shared_ptr<CSite> foundSite = getSite(scnl);
	if (foundSite != NULL) {
		return (foundSite);
	}

	// if we didn't find the site in the
	// send request for information about this station
	if (m_iHoursBeforeLookingUp >= 0) {
		// what time is it
		time_t tNow;
		std::time(&tNow);

		// lock while we are searching / editing m_mLastTimeSiteLookedUp
		std::lock_guard<std::recursive_mutex> guard(m_SiteListMutex);

		// get what time this station has been looked up before
		int tLookup = 0;
		auto itsite = m_mLastTimeSiteLookedUp.find(scnl);
		if (itsite != m_mLastTimeSiteLookedUp.end()) {
			tLookup = m_mLastTimeSiteLookedUp[scnl];
		}

		// only ask for a station occasionally
		if ((tNow - tLookup) > (k_nHoursToSeconds * m_iHoursBeforeLookingUp)) {
			// construct request json message
			std::shared_ptr<json::Object> request = std::make_shared<
					json::Object>(json::Object());
			(*request)["Type"] = "SiteLookup";
			(*request)["Site"] = site;
			(*request)["Comp"] = comp;
			(*request)["Net"] = net;
			(*request)["Loc"] = loc;

			// log
			char sLog[glass3::util::Logger::k_nMaxLogEntrySize];
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

		// construct a json message containing our site info
		stationObj["Lat"] = site->getRawLatitude();
		stationObj["Lon"] = site->getRawLongitude();
		stationObj["Z"] = site->getRawElevation();
		stationObj["Qual"] = site->getQuality();
		stationObj["Enable"] = site->getEnable();
		stationObj["Use"] = site->getUse();
		stationObj["UseForTele"] = site->getUseForTeleseismic();

		int lastPicked = site->getTLastPickAdded();
		if ((lastPicked > 0) && (m_iMaxHoursWithoutPicking > 0)) {
			stationObj["LastPicked"] = static_cast<int>(site->getTLastPickAdded());
		}

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
	if ((tNow - m_tLastChecked) < (1 * k_nHoursToSeconds)) {
		// no
		return (glass3::util::WorkState::Idle);
	}

	// lock the site list while we are checking it
	while ((m_SiteListMutex.try_lock() == false) &&
					(getTerminate() == false)) {
		// update thread status
		setThreadHealth(true);

		// wait a little while
		std::this_thread::sleep_for(
				std::chrono::milliseconds(getSleepTime()));
	}

	// remember when we last checked
	m_tLastChecked = tNow;

	glass3::util::Logger::log("info",
							"CSiteList::work: Updating used sites based on statistics.");

	// create a vector to hold the sites that have changed
	std::vector<std::shared_ptr<CSite>> vModifiedSites;

	// for each site in the site list
	for (auto aSite : m_vSite) {
		// skip manually disabled sites, we can't do anything with them
		if (aSite->getEnable() == false) {
			continue;
		}

		// if we are currently using a site
		if (aSite->getUse() == true) {
			bool disableSite = false;

			// check for a used site that is not picking
			if (m_iMaxHoursWithoutPicking > 0) {
				// when was the last pick added to this site
				time_t tLastPickAdded = aSite->getTLastPickAdded();

				// have we not seen data?
				if ((tNow - tLastPickAdded)
						> (k_nHoursToSeconds * m_iMaxHoursWithoutPicking)) {
					glass3::util::Logger::log(
							"debug",
							"CSiteList::work: Removing " + aSite->getSCNL()
									+ " for not picking in "
									+ std::to_string(tNow - tLastPickAdded)
									+ " seconds ");
					disableSite = true;
				}
			}

			// check for a used site that is picking too often
			if ((disableSite != true) && (m_iMaxPicksPerHour > 0)) {
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

			// if we're to disable this site
			if (disableSite == true) {
				// disable the site
				aSite->setUse(false);

				// site list was modified
				m_tLastUpdated = tNow;

				// add to modified sites (no duplicates)
				if(std::find(vModifiedSites.begin(), vModifiedSites.end(), aSite) ==
					vModifiedSites.end()) {
					// did not find in vector, add it
					vModifiedSites.push_back(aSite);
				}
			}
		} else {  // if we are currently not using a site
			bool enableSite = false;

			// check for an unused site that has started picking
			if (m_iMaxHoursWithoutPicking > 0) {
				// when was the last pick added to this site
				int tLastPickAdded = aSite->getTLastPickAdded();

				// if we've got no time from site, default tLastPickAdded to when
				// the sitelist was created (effectively glass startup time)
				if (tLastPickAdded < 0) {
					tLastPickAdded = m_tCreated;
				}

				// have we seen data?
				if ((tNow - tLastPickAdded)
						< (k_nHoursToSeconds * m_iMaxHoursWithoutPicking)) {
					glass3::util::Logger::log(
							"debug",
							"CSiteList::work: Added " + aSite->getSCNL()
									+ " because it has picked within "
									+ std::to_string(tNow - tLastPickAdded)
									+ " seconds ");

					enableSite = true;
				}
			}

			// check for an unused site that is no longer picking to often
			if (m_iMaxPicksPerHour > 0) {
				// how many picks since last check
				int picksSinceCheck = aSite->getPickCountSinceCheck();

				// we check every hour, so picks since check is picks per hour
				// also, don't bother turning on a site that hasn't seen any picks
				if ((picksSinceCheck > 0) && (picksSinceCheck < m_iMaxPicksPerHour)) {
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

			// if we're to enable this site
			if (enableSite == true) {
				// enable the site
				aSite->setUse(true);

				// site list was modified
				m_tLastUpdated = tNow;

				// add to modified sites (no duplicates)
				if(std::find(vModifiedSites.begin(), vModifiedSites.end(), aSite) ==
					vModifiedSites.end()) {
					// did not find in vector, add it
					vModifiedSites.push_back(aSite);
				}
			}
		}

		// update thread status
		setThreadHealth();
	}

	// done with site list
	m_SiteListMutex.unlock();

	// pass all the modified sites to the webs for updateing
	for (auto aSite : vModifiedSites) {
		if (CGlass::getWebList()) {
			CGlass::getWebList()->updateSite(aSite);

			// update thread status
			setThreadHealth();
		}
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

// ----------------------------------------------------getLastUpdated
int CSiteList::getLastUpdated() const {
	return (m_tLastUpdated);
}

}  // namespace glasscore

#include "WebList.h"
#include <json.h>
#include <logger.h>
#include <memory>
#include <string>
#include "Node.h"
#include "Glass.h"
#include "Web.h"
#include "SiteList.h"
#include "Site.h"
#include "Pick.h"

namespace glasscore {

// ---------------------------------------------------------CWebList
CWebList::CWebList(int numThreads) {
	m_iNumThreads = numThreads;
	clear();
}

// ---------------------------------------------------------~CWebList
CWebList::~CWebList() {
	clear();
}

// ---------------------------------------------------------clear
void CWebList::clear() {
	std::lock_guard<std::recursive_mutex> webListGuard(m_WebListMutex);

	m_pSiteList = NULL;

	// clear out all the  webs
	for (auto &web : m_vWebs) {
		web->clear();
	}
	m_vWebs.clear();
}

// -------------------------------------------------------receiveExternalMessage
bool CWebList::receiveExternalMessage(std::shared_ptr<json::Object> com) {
	// null check json
	if (com == NULL) {
		glass3::util::Logger::log(
				"error",
				"CWebList::receiveExternalMessage: NULL json configuration.");
		return (false);
	}

	// we only care about messages with a string Cmd key.
	if (com->HasKey("Cmd")
			&& ((*com)["Cmd"].GetType() == json::ValueType::StringVal)) {
		// dispatch to appropriate function based on Cmd value
		json::Value v = (*com)["Cmd"].ToString();

		// if the Cmd value is for one of the web generation commands,
		// call addWeb
		if ((v == "Global") || (v == "Grid") || (v == "Grid_Explicit")) {
			return (addWeb(com));
		}

		// if the  Cmd value is a web removal command
		// call removeWeb
		if (v == "RemoveWeb") {
			return (removeWeb(com));
		}
	}
	return (false);
}

// ---------------------------------------------------------addWeb
bool CWebList::addWeb(std::shared_ptr<json::Object> com) {
	// null check json
	if (com == NULL) {
		glass3::util::Logger::log("error",
								"CWebList::addWeb: NULL json configuration.");
		return (false);
	}

	// get the web name
	std::string name;
	if ((*com).HasKey("Name")) {
		name = (*com)["Name"].ToString();
	} else {
		return (false);
	}

	std::lock_guard<std::recursive_mutex> webListGuard(m_WebListMutex);

	// check to see if we have a web with this name already
	for (int i = 0; i < m_vWebs.size(); i++) {
		std::shared_ptr<CWeb> web = m_vWebs[i];

		// look for name match
		if (web->getName() == name) {
			glass3::util::Logger::log(
					"warning",
					"CWebList::addWeb: Already have a web with the name " + name
							+ " call RemoveWeb before adding a new web.");
			return (false);
		}
	}

	// Create a new web object
	std::shared_ptr<CWeb> web(new CWeb(m_iNumThreads));
	if (m_pSiteList != NULL) {
		web->setSiteList(m_pSiteList);
	}

	// send the config to web so that it can generate itself
	if (web->receiveExternalMessage(com)) {
		// add the web to the list if it was successfully created
		m_vWebs.push_back(web);

		return (true);
	}

	// failed to create a web
	return (false);
}

// ---------------------------------------------------------removeWeb
bool CWebList::removeWeb(std::shared_ptr<json::Object> com) {
	// null check json
	if (com == NULL) {
		glass3::util::Logger::log(
				"error",
				"CWebList::removeWeb: NULL json configuration.");
		return (false);
	}

	// get the web name
	std::string name;
	if ((*com).HasKey("Name")) {
		name = (*com)["Name"].ToString();
	} else {
		return (false);
	}

	// make sure we have a valid name for removal
	if (name == "Nemo") {
		glass3::util::Logger::log(
				"warning",
				"CWebList::removeWeb: Unnamed subnets cannot be removed.");
		return (false);
	}

	std::lock_guard<std::recursive_mutex> webListGuard(m_WebListMutex);

	// look for a web with that name
	for (int i = 0; i < m_vWebs.size(); i++) {
		std::shared_ptr<CWeb> web = m_vWebs[i];

		if (web->getName() == name) {
			// clear the web and remove it
			web->clear();
			m_vWebs.erase(m_vWebs.begin() + i);

			// removed
			return (true);
		}
	}
	return (false);
}

// ---------------------------------------------------------addSite
void CWebList::addSite(std::shared_ptr<CSite> site) {
	std::lock_guard<std::recursive_mutex> webListGuard(m_WebListMutex);

	// Don't process adds before web definitions
	if (m_vWebs.size() < 1) {
		return;
	}

	glass3::util::Logger::log(
			"debug",
			"CWebList::addSite: Adding station " + site->getSCNL() + ".");

	// Update all web node site lists that might be changed
	// by the addition of this site
	for (auto &web : m_vWebs) {
		if (web->getUpdate() == true) {
			if (web->isSiteAllowed(site) == true) {
				web->addJob(std::bind(&CWeb::addSite, web, site));
			}
		}
	}
}

// ---------------------------------------------------------removeSite
void CWebList::removeSite(std::shared_ptr<CSite> site) {
	std::lock_guard<std::recursive_mutex> webListGuard(m_WebListMutex);

	// Don't process removes before web definitions
	if (m_vWebs.size() < 1) {
		return;
	}

	glass3::util::Logger::log(
			"debug",
			"CWebList::remSite: Removing station " + site->getSCNL() + ".");

	// Remove site from all web nodes that link to it and restructure
	// node site lists
	for (auto &web : m_vWebs) {
		if (web->getUpdate() == true) {
			if (web->isSiteAllowed(site) == true) {
				web->addJob(std::bind(&CWeb::removeSite, web, site));
			}
		}
	}
}

// ---------------------------------------------------------hasSite
bool CWebList::hasSite(std::shared_ptr<CSite> site) {
	//  nullcheck
	if (site == NULL) {
		return (false);
	}

	std::lock_guard<std::recursive_mutex> webListGuard(m_WebListMutex);

	if (m_vWebs.size() < 1) {
		return (false);
	}

	// for each node in web
	for (auto &web : m_vWebs) {
		// check to see if we have this site
		if (web->hasSite(site) == true) {
			return (true);
		}
	}

	// site not found
	return (false);
}

// ---------------------------------------------------------healthCheck
bool CWebList::healthCheck() {
	std::lock_guard<std::recursive_mutex> webListGuard(m_WebListMutex);

	// Don't bother if there is no webs
	if (m_vWebs.size() < 1) {
		return (true);
	}

	// if we're updating in background
	if (m_iNumThreads > 0) {
		// for each web
		for (auto web : m_vWebs) {
			// check if it's alive
			if (web->healthCheck() == false) {
				return (false);
			}
		}
	}

	// all's well
	return (true);
}

// ---------------------------------------------------------getSiteList
const CSiteList* CWebList::getSiteList() const {
	std::lock_guard<std::recursive_mutex> webListGuard(m_WebListMutex);
	return (m_pSiteList);
}

// ---------------------------------------------------------setSiteList
void CWebList::setSiteList(CSiteList* siteList) {
	std::lock_guard<std::recursive_mutex> webListGuard(m_WebListMutex);
	m_pSiteList = siteList;
}

// ---------------------------------------------------------size
int CWebList::size() const {
	std::lock_guard<std::recursive_mutex> webListGuard(m_WebListMutex);
	return (m_vWebs.size());
}
}  // namespace glasscore

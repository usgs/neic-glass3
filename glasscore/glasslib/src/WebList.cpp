#include <json.h>
#include <memory>
#include <string>
#include "WebList.h"
#include "Node.h"
#include "Glass.h"
#include "Web.h"
#include "SiteList.h"
#include "Site.h"
#include "Pick.h"
#include "Logit.h"

namespace glasscore {

// ---------------------------------------------------------CWebList
CWebList::CWebList(bool useBackgroundThreads) {
	m_bUseBackgroundThreads = useBackgroundThreads;
	clear();
}

// ---------------------------------------------------------~CWebList
CWebList::~CWebList() {
	clear();
}

// ---------------------------------------------------------clear
void CWebList::clear() {
	pGlass = NULL;

	// clear out all the  webs
	for (auto &web : vWeb) {
		web->clear();
	}
	vWeb.clear();
}

// ---------------------------------------------------------Dispatch
bool CWebList::dispatch(json::Object *com) {
	// null check json
	if (com == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CWebList::dispatch: NULL json configuration.");
		return (false);
	}

	// we only care about messages with a string Cmd key.
	if (com->HasKey("Cmd")
			&& ((*com)["Cmd"].GetType() == json::ValueType::StringVal)) {
		// dispatch to appropriate function based on Cmd value
		json::Value v = (*com)["Cmd"].ToString();

		// if the Cmd value is for one of the web generation commands,
		// call addWeb
		if ((v == "Global") || (v == "Shell") || (v == "Single")
				|| (v == "Grid") || (v == "Grid_Explicit")) {
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

// ---------------------------------------------------------AddWeb
bool CWebList::addWeb(json::Object *com) {
	// null check json
	if (com == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
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

	// check to see if we have a web with this name already
	for (int i = 0; i < vWeb.size(); i++) {
		std::shared_ptr<CWeb> web = vWeb[i];

		// look for name match
		if (web->sName == name) {
			glassutil::CLogit::log(
					glassutil::log_level::warn,
					"CWebList::addWeb: Already have a web with the name " + name
							+ " call RemoveWeb before adding a new web.");
			return (false);
		}
	}

	// Create a new web object
	std::shared_ptr<CWeb> web(new CWeb(m_bUseBackgroundThreads));
	web->pGlass = pGlass;

	// send the config to web so that it can generate itself
	if (web->dispatch(com)) {
		// add the web to the list if it was successfully created
		vWeb.push_back(web);

		return (true);
	}

	// failed to create a web
	return (false);
}

// ---------------------------------------------------------RemoveWeb
bool CWebList::removeWeb(json::Object *com) {
	// null check json
	if (com == NULL) {
		glassutil::CLogit::log(
				glassutil::log_level::error,
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
		glassutil::CLogit::log(
				glassutil::log_level::warn,
				"CWebList::removeWeb: Unnamed subnets cannot be removed.");
		return (false);
	}

	// look for a web with that name
	for (int i = 0; i < vWeb.size(); i++) {
		std::shared_ptr<CWeb> web = vWeb[i];

		if (web->sName == name) {
			// clear the web and remove it
			web->clear();
			vWeb.erase(vWeb.begin() + i);

			// removed
			return (true);
		}
	}
	return (false);
}

// ---------------------------------------------------------addSite
void CWebList::addSite(std::shared_ptr<CSite> site) {
	// Don't process adds before web definitions
	if (vWeb.size() < 1) {
		return;
	}

	glassutil::CLogit::log(
			glassutil::log_level::debug,
			"CWebList::addSite: Adding station " + site->sScnl + ".");

	// Update all web node site lists that might be changed
	// by the addition of this site
	for (auto &web : vWeb) {
		if (web->bUpdate == true) {
			if (web->isSiteAllowed(site) == true) {
				web->addJob(std::bind(&CWeb::addSite, web, site));
			}
		}
	}
}

// ---------------------------------------------------------remSite
void CWebList::remSite(std::shared_ptr<CSite> site) {
	// Don't process removes before web definitions
	if (vWeb.size() < 1) {
		return;
	}

	glassutil::CLogit::log(
			glassutil::log_level::debug,
			"CWebList::addSite: Removing station " + site->sScnl + ".");

	// Remove site from all web nodes that link to it and restructure
	// node site lists
	for (auto &web : vWeb) {
		if (web->bUpdate == true) {
			if (web->isSiteAllowed(site) == true) {
				web->addJob(std::bind(&CWeb::remSite, web, site));
			}
		}
	}
}

// ---------------------------------------------------------statusCheck
bool CWebList::statusCheck() {
	// Don't bother if there is no webs
	if (vWeb.size() < 1) {
		return (true);
	}

	// if we're updating in background
	if (m_bUseBackgroundThreads == true) {
		// for each web
		for (auto web : vWeb) {
			// check if it's alive
			if (web->statusCheck() == false) {
				return (false);
			}
		}
	}

	// all's well
	return (true);
}
}  // namespace glasscore

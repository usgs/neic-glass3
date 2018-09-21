#include <json.h>
#include <memory>
#include <string>
#include <vector>
#include <limits>
#include <algorithm>
#include "Pid.h"
#include "Web.h"
#include "Trigger.h"
#include "Node.h"
#include "PickList.h"
#include "HypoList.h"
#include "Hypo.h"
#include "Pick.h"
#include "Site.h"
#include "SiteList.h"
#include "Date.h"
#include "Glass.h"
#include "Logit.h"

namespace glasscore {

// ---------------------------------------------------------CPick
CPick::CPick() {
	clear();
}

// ---------------------------------------------------------CPick
CPick::CPick(std::shared_ptr<CSite> pickSite, double pickTime,
				std::string pickIdString, double backAzimuth, double slowness) {
	initialize(pickSite, pickTime, pickIdString, backAzimuth, slowness);
}

// ---------------------------------------------------------CPick
CPick::CPick(std::shared_ptr<json::Object> pick, CSiteList *pSiteList) {
	clear();

	// null check json
	if (pick == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CPick::CPick: NULL json communication.");
		return;
	}

	// check type
	if (pick->HasKey("Type")
			&& ((*pick)["Type"].GetType() == json::ValueType::StringVal)) {
		std::string type = (*pick)["Type"].ToString();

		if (type != "Pick") {
			glassutil::CLogit::log(
					glassutil::log_level::warn,
					"CPick::CPick: Non-Pick message passed in.");
			return;
		}
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"CPick::CPick: Missing required Type Key.");
		return;
	}

	// pick definition variables
	std::string sta = "";
	std::string comp = "";
	std::string net = "";
	std::string loc = "";
	std::shared_ptr<CSite> site = NULL;
	std::string ttt = "";
	double tpick = 0;
	double backAzimuth = -1;
	double slowness = -1;
	std::string pid = "";

	// site
	if (pick->HasKey("Site")
			&& ((*pick)["Site"].GetType() == json::ValueType::ObjectVal)) {
		// site is an object, create scnl std::string from it
		// get object
		json::Object siteobj = (*pick)["Site"].ToObject();

		// scnl varibles

		// site
		if (siteobj.HasKey("Station")
				&& (siteobj["Station"].GetType() == json::ValueType::StringVal)) {
			sta = siteobj["Station"].ToString();
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::error,
					"CPick::CPick: Missing required Station Key.");

			return;
		}

		// comp (optional)
		if (siteobj.HasKey("Channel")
				&& (siteobj["Channel"].GetType() == json::ValueType::StringVal)) {
			comp = siteobj["Channel"].ToString();
		} else {
			comp = "";
		}

		// net
		if (siteobj.HasKey("Network")
				&& (siteobj["Network"].GetType() == json::ValueType::StringVal)) {
			net = siteobj["Network"].ToString();
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::error,
					"CPick::CPick: Missing required Network Key.");

			return;
		}

		// loc (optional)
		if (siteobj.HasKey("Location")
				&& (siteobj["Location"].GetType() == json::ValueType::StringVal)) {
			loc = siteobj["Location"].ToString();
		} else {
			loc = "";
		}
	} else {
		// no site key
		glassutil::CLogit::log(glassutil::log_level::error,
								"CPick::CPick: Missing required Site Key.");

		return;
	}

	// lookup the site, if we have a sitelist available
	if (pSiteList) {
		site = pSiteList->getSite(sta, comp, net, loc);
	}

	// check to see if we got a site
	if (site == NULL) {
		glassutil::CLogit::log(glassutil::log_level::warn,
								"CPick::CPick: site is null.");

		return;
	}

	// check to see if we're using this site
	if (!site->getEnable()) {
		return;
	}

	// time
	// get the pick time based on which key we find
	if (pick->HasKey("Time")
			&& ((*pick)["Time"].GetType() == json::ValueType::StringVal)) {
		// Time is formatted in iso8601, convert to julian seconds
		ttt = (*pick)["Time"].ToString();
		glassutil::CDate dt = glassutil::CDate();
		tpick = dt.decodeISO8601Time(ttt);
	} else if (pick->HasKey("T")
			&& ((*pick)["T"].GetType() == json::ValueType::StringVal)) {
		// T is formatted in datetime, convert to julian seconds
		ttt = (*pick)["T"].ToString();
		glassutil::CDate dt = glassutil::CDate();
		tpick = dt.decodeDateTime(ttt);
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"CPick::CPick: Missing required Time or T Key.");

		return;
	}

	// pid
	// get the pick id based on which key we find
	if (pick->HasKey("ID")
			&& ((*pick)["ID"].GetType() == json::ValueType::StringVal)) {
		pid = (*pick)["ID"].ToString();
	} else if (pick->HasKey("Pid")
			&& ((*pick)["Pid"].GetType() == json::ValueType::StringVal)) {
		pid = (*pick)["Pid"].ToString();
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::warn,
				"CPick::CPick: Missing required ID or Pid Key.");

		return;
	}

	// beam
	if (pick->HasKey("Beam")
			&& ((*pick)["Beam"].GetType() == json::ValueType::ObjectVal)) {
		// beam is an object
		json::Object beamobj = (*pick)["Beam"].ToObject();

		// backAzimuth
		if (beamobj.HasKey("BackAzimuth")
				&& (beamobj["BackAzimuth"].GetType()
						== json::ValueType::DoubleVal)) {
			backAzimuth = beamobj["BackAzimuth"].ToDouble();
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::warn,
					"CPick::CPick: Missing Beam BackAzimuth Key.");
			backAzimuth = std::numeric_limits<double>::quiet_NaN();
		}

		// slowness
		if (beamobj.HasKey("Slowness")
				&& (beamobj["Slowness"].GetType() == json::ValueType::DoubleVal)) {
			slowness = beamobj["Slowness"].ToDouble();
		} else {
			glassutil::CLogit::log(glassutil::log_level::warn,
									"CPick::CPick: Missing Beam Slowness Key.");
			slowness = std::numeric_limits<double>::quiet_NaN();
		}
	} else {
		backAzimuth = std::numeric_limits<double>::quiet_NaN();
		slowness = std::numeric_limits<double>::quiet_NaN();
	}

	// pass to initialization function
	if (!initialize(site, tpick, pid, backAzimuth, slowness)) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CPick::CPick: Failed to initialize pick.");
		return;
	}

	std::lock_guard<std::recursive_mutex> guard(m_PickMutex);

	// remember input json for hypo message generation
	// note, move to init?
	m_JSONPick = pick;
}

// ---------------------------------------------------------~CPick
CPick::~CPick() {
	clear();
}

// ---------------------------------------------------------clear
void CPick::clear() {
	std::lock_guard<std::recursive_mutex> guard(m_PickMutex);

	m_wpSite.reset();
	m_wpHypo.reset();
	m_JSONPick.reset();

	m_sPhaseName = "";
	m_sID = "";
	m_tPick = 0;
	m_dBackAzimuth = std::numeric_limits<double>::quiet_NaN();
	m_dSlowness = std::numeric_limits<double>::quiet_NaN();
}

// ---------------------------------------------------------initialize
bool CPick::initialize(std::shared_ptr<CSite> pickSite, double pickTime,
						std::string pickIdString, double backAzimuth,
						double slowness) {
	std::lock_guard<std::recursive_mutex> guard(m_PickMutex);

	clear();

	m_tPick = pickTime;
	m_sID = pickIdString;
	m_dBackAzimuth = backAzimuth;
	m_dSlowness = slowness;

	// nullcheck
	if (pickSite == NULL) {
		return (false);
	} else {
		m_wpSite = pickSite;
	}

	return (true);
}

// ---------------------------------------------------------addHypo
void CPick::addHypoReference(std::shared_ptr<CHypo> hyp, bool force) {
	std::lock_guard<std::recursive_mutex> guard(m_PickMutex);

	// nullcheck
	if (hyp == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CPick::addHypo: NULL hypo provided.");
		return;
	}

	// Add hypo data reference to this pick
	if (force == true) {
		m_wpHypo = hyp;
	} else if (m_wpHypo.expired() == true) {
		m_wpHypo = hyp;
	}
}

// ---------------------------------------------------------remHypo
void CPick::removeHypoReference(std::shared_ptr<CHypo> hyp) {
	// nullcheck
	if (hyp == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CPick::remHypo: NULL hypo provided.");
		return;
	}

	removeHypoReference(hyp->getID());
}

void CPick::removeHypoReference(std::string pid) {
	std::lock_guard<std::recursive_mutex> guard(m_PickMutex);

	// is the pointer still valid
	if (auto pHypo = m_wpHypo.lock()) {
		// Remove hypo reference from this pick
		if (pHypo->getID() == pid) {
			clearHypoReference();
		}
	} else {
		// remove invalid pointer
		clearHypoReference();
	}
}

// ---------------------------------------------------------clearHypo
void CPick::clearHypoReference() {
	std::lock_guard<std::recursive_mutex> guard(m_PickMutex);
	m_wpHypo.reset();
}

// ---------------------------------------------------------nucleate
bool CPick::nucleate() {
	// get the site shared_ptr
	std::shared_ptr<CSite> pickSite = m_wpSite.lock();
	std::string pt = glassutil::CDate::encodeDateTime(m_tPick);
	char sLog[1024];

	// Use site nucleate to scan all nodes
	// linked to this pick's site and calculate
	// the stacked agoric at each node.  If the threshold
	// is exceeded, the node is added to the site's trigger list
	std::vector<std::shared_ptr<CTrigger>> vTrigger = pickSite->nucleate(
			m_tPick);

	// if there were no triggers, we're done
	if (vTrigger.size() == 0) {
		glassutil::CLogit::log(
				glassutil::log_level::debug,
				"CPick::nucleate: NOTRG site:" + pickSite->getSCNL()
						+ "; tPick:" + pt + "; sID:" + m_sID);

		return (false);
	}

	for (const auto &trigger : vTrigger) {
		if (trigger->getWeb() == NULL) {
			continue;
		}

		// check to see if the pick is currently associated to a hypo
		if (m_wpHypo.expired() == false) {
			// get the hypo and compute distance between it and the
			// current trigger
			std::shared_ptr<CHypo> pHypo = m_wpHypo.lock();
			if (pHypo != NULL) {
				glassutil::CGeo geoHypo = pHypo->getGeo();

				glassutil::CGeo trigHypo = trigger->getGeo();

				double dist = (geoHypo.delta(&trigHypo) / DEG2RAD) * 111.12;

				// is the associated hypo close enough to this trigger to skip
				// close enough means within the resolution of the trigger
				if (dist < trigger->getWebResolution()) {
					glassutil::CLogit::log(
							glassutil::log_level::debug,
							"CPick::nucleate: SKIPTRG because pick proximal hypo ("
									+ std::to_string(dist) + " < "
									+ std::to_string(trigger->getWebResolution())
									+ ")");
					continue;
				}
			}
		}

		// create the hypo using the node
		std::shared_ptr<CHypo> hypo = std::make_shared<CHypo>(
				trigger, CGlass::getAssociationTravelTimes());

		// add links to all the picks that support the hypo
		std::vector<std::shared_ptr<CPick>> vTriggerPicks = trigger->getVPick();

		for (auto pick : vTriggerPicks) {
			// they're not associated yet, just potentially
			hypo->addPickReference(pick);
		}

		// use the hypo's nucleation threshold, which is really the
		// web's nucleation threshold
		int ncut = hypo->getNucleationDataThreshold();
		double thresh = hypo->getNucleationStackThreshold();
		bool bad = false;

		// First localization attempt after nucleation
		// make 3 passes
		for (int ipass = 0; ipass < 3; ipass++) {
			// get an initial location via synthetic annealing,
			// which also prunes out any poorly fitting picks
			// the search is based on the grid resolution, and how
			// far out the ot can change without losing the initial pick
			// this all assumes that the closest grid triggers
			// values derived from testing global event association
			double bayes = hypo->anneal(
					2000, trigger->getWebResolution() / 2.,
					trigger->getWebResolution() / 100.,
					std::max(trigger->getWebResolution() / 10.0, 5.0), .1);

			// get the number of picks we have now
			int npick = hypo->getPickDataSize();

			snprintf(sLog, sizeof(sLog), "CPick::nucleate: -- Pass:%d; nPick:%d"
						"/nCut:%d; bayes:%f/thresh:%f; %s",
						ipass, npick, ncut, bayes, thresh,
						hypo->getID().c_str());
			glassutil::CLogit::log(sLog);

			// check to see if we still have a high enough bayes value for this
			// hypo to survive.
			if (bayes < thresh) {
				// it isn't
				snprintf(sLog, sizeof(sLog),
							"CPick::nucleate: -- Abandoning solution %s "
							"due to low bayes value "
							"(bayes:%f/thresh:%f)",
							hypo->getID().c_str(), bayes, thresh);
				glassutil::CLogit::log(sLog);

				// don't bother making additional passes
				bad = true;
				break;
			}

			// check to see if we still have enough picks for this hypo to
			// survive.
			// NOTE, in Node, ncut is used as a threshold for the number of
			// *stations* here it's used for the number of *picks*, which only
			// since we only nucleate on a single phase.
			if (npick < ncut) {
				// we don't
				snprintf(sLog, sizeof(sLog),
							"CPick::nucleate: -- Abandoning solution %s "
							"due to lack of picks "
							"(npick:%d/ncut:%d)",
							hypo->getID().c_str(), npick, ncut);
				glassutil::CLogit::log(sLog);

				// don't bother making additional passes
				bad = true;
				break;
			}
		}

		// we've abandoned the potential hypo at this node
		if (bad) {
			// move on to the next triggering node
			continue;
		}

		// log the hypo
		std::string st = glassutil::CDate::encodeDateTime(hypo->getTOrigin());
		glassutil::CLogit::log(
				glassutil::log_level::debug,
				"CPick::nucleate: TRG site:" + pickSite->getSCNL() + "; tPick:"
						+ pt + "; sID:" + m_sID + " => web:"
						+ hypo->getWebName() + "; hyp: " + hypo->getID()
						+ "; lat:" + std::to_string(hypo->getLatitude())
						+ "; lon:" + std::to_string(hypo->getLongitude())
						+ "; z:" + std::to_string(hypo->getDepth()) + "; tOrg:"
						+ st);

		// if we got this far, the hypo has enough supporting data to
		// merit adding it to the hypo list
		CGlass::getHypoList()->addHypo(hypo);
	}

	// done
	return (true);
}

// ---------------------------------------------------------getJSONPick
const std::shared_ptr<json::Object>& CPick::getJSONPick() const {
	std::lock_guard<std::recursive_mutex> pickGuard(m_PickMutex);
	return (m_JSONPick);
}

// ---------------------------------------------------------getHypo
const std::shared_ptr<CHypo> CPick::getHypoReference() const {
	std::lock_guard<std::recursive_mutex> pickGuard(m_PickMutex);
	return (m_wpHypo.lock());
}

// ---------------------------------------------------------getSite
const std::shared_ptr<CSite> CPick::getSite() const {
	std::lock_guard<std::recursive_mutex> pickGuard(m_PickMutex);
	return (m_wpSite.lock());
}

// ---------------------------------------------------------getPhaseName
const std::string& CPick::getPhaseName() const {
	std::lock_guard<std::recursive_mutex> pickGuard(m_PickMutex);
	return (m_sPhaseName);
}

// ---------------------------------------------------------getID
const std::string& CPick::getID() const {
	std::lock_guard<std::recursive_mutex> pickGuard(m_PickMutex);
	return (m_sID);
}

// ---------------------------------------------------------getBackAzimuth
double CPick::getBackAzimuth() const {
	return (m_dBackAzimuth);
}

// ---------------------------------------------------------getSlowness
double CPick::getSlowness() const {
	return (m_dSlowness);
}

// ---------------------------------------------------------getTPick
double CPick::getTPick() const {
	return (m_tPick);
}

// ---------------------------------------------------------setTPick
void CPick::setTPick(double tPick) {
	m_tPick = tPick;
}

}  // namespace glasscore

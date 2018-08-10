#include <json.h>
#include <memory>
#include <string>
#include <vector>
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
CPick::CPick(std::shared_ptr<CSite> pickSite, double pickTime, int pickId,
				std::string pickIdString, double backAzimuth, double slowness) {
	// nullcheck
	if (pickSite == NULL) {
		return;
	}

	initialize(pickSite, pickTime, pickId, pickIdString, backAzimuth, slowness);
}

// ---------------------------------------------------------CPick
CPick::CPick(std::shared_ptr<json::Object> pick, int pickId,
				CSiteList *pSiteList) {
	clear();

	// null check json
	if (pick == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CPick::CPick: NULL json communication.");
		return;
	}

	// check cmd
	if (pick->HasKey("Cmd")
			&& ((*pick)["Cmd"].GetType() == json::ValueType::StringVal)) {
		std::string cmd = (*pick)["Cmd"].ToString();

		if (cmd != "Pick") {
			glassutil::CLogit::log(
					glassutil::log_level::warn,
					"CPick::CPick: Non-Pick message passed in.");
			return;
		}
	} else if (pick->HasKey("Type")
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
				"CPick::CPick: Missing required Cmd or Type Key.");
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
			backAzimuth = -1;
		}

		// slowness
		if (beamobj.HasKey("Slowness")
				&& (beamobj["Slowness"].GetType() == json::ValueType::DoubleVal)) {
			slowness = beamobj["Slowness"].ToDouble();
		} else {
			glassutil::CLogit::log(glassutil::log_level::warn,
									"CPick::CPick: Missing Beam Slowness Key.");
			slowness = -1;
		}
	}

	// pass to initialization function
	if (!initialize(site, tpick, pickId, pid, backAzimuth, slowness)) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CPick::CPick: Failed to initialize pick.");
		return;
	}

	std::lock_guard < std::recursive_mutex > guard(pickMutex);

	// remember input json for hypo message generation
	// note, move to init?
	jPick = pick;
}

// ---------------------------------------------------------~CPick
CPick::~CPick() {
	clear();
}

// ---------------------------------------------------------~clear
void CPick::clear() {
	std::lock_guard < std::recursive_mutex > guard(pickMutex);

	wpSite.reset();
	wpHypo.reset();
	jPick.reset();

	sAss = "";
	sPhs = "";
	sPid = "";
	tPick = 0;
	idPick = 0;
	dBackAzimuth = -1;
	dSlowness = -1;
}

bool CPick::initialize(std::shared_ptr<CSite> pickSite, double pickTime,
						int pickId, std::string pickIdString,
						double backAzimuth, double slowness) {
	std::lock_guard < std::recursive_mutex > guard(pickMutex);

	clear();

	// nullcheck
	if (pickSite == NULL) {
		return (false);
	}

	wpSite = pickSite;
	tPick = pickTime;
	idPick = pickId;
	sPid = pickIdString;
	dBackAzimuth = backAzimuth;
	dSlowness = slowness;

	/* glassutil::CLogit::log(
	 glassutil::log_level::debug,
	 "CPick::initialize: site:" + pickSite->getScnl() + "; tPick:"
	 + std::to_string(tPick) + "; idPick:"
	 + std::to_string(idPick) + "; sPid:" + sPid);
	 */

	return (true);
}

// ---------------------------------------------------------addHypo
void CPick::addHypo(std::shared_ptr<CHypo> hyp, std::string ass, bool force) {
	std::lock_guard < std::recursive_mutex > guard(pickMutex);

	// nullcheck
	if (hyp == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CPick::addHypo: NULL hypo provided.");
		return;
	}

	// Add hypo data reference to this pick
	if (force == true) {
		wpHypo = hyp;
		sAss = ass;
	} else if (wpHypo.expired() == true) {
		wpHypo = hyp;
		sAss = ass;
	}
}

// ---------------------------------------------------------remHypo
void CPick::remHypo(std::shared_ptr<CHypo> hyp) {
	// nullcheck
	if (hyp == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CPick::remHypo: NULL hypo provided.");
		return;
	}

	remHypo(hyp->getPid());
}

void CPick::remHypo(std::string pid) {
	std::lock_guard < std::recursive_mutex > guard(pickMutex);

	// is the pointer still valid
	if (auto pHypo = wpHypo.lock()) {
		// Remove hypo reference from this pick
		if (pHypo->getPid() == pid) {
			clearHypo();
		}
	} else {
		// remove invalid pointer
		clearHypo();
	}
}

void CPick::clearHypo() {
	std::lock_guard < std::recursive_mutex > guard(pickMutex);
	wpHypo.reset();
}

void CPick::setAss(std::string ass) {
	std::lock_guard < std::recursive_mutex > guard(pickMutex);

	sAss = ass;
}

// ---------------------------------------------------------Nucleate
bool CPick::nucleate() {
	// get the site shared_ptr
	std::shared_ptr<CSite> pickSite = wpSite.lock();

	// get CGlass pointer from site
	CGlass *pGlass = pickSite->getGlass();

	// nullcheck
	if (pGlass == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CPick::nucleate: NULL pGlass.");
		return (false);
	}

	std::string pt = glassutil::CDate::encodeDateTime(tPick);
	char sLog[1024];

	// check to see if the pick is currently associated to a hypo
	if (wpHypo.expired() == false) {
		// get the hypo and compute ratio
		std::shared_ptr<CHypo> pHypo = wpHypo.lock();
		if (pHypo != NULL) {
			double adBayesRatio = (pHypo->getBayes()) / (pHypo->getThresh());

			// check to see if the ratio is high enough to not
			// bother
			// NOTE: Hardcoded
			if (adBayesRatio > 2.0) {
				glassutil::CLogit::log(
						glassutil::log_level::debug,
						"CPick::nucleate: SKIPTRG due to large event association "
								+ pickSite->getScnl() + "; tPick:" + pt
								+ "; idPick:" + std::to_string(idPick)
								+ " associated with an event with stack twice threshold ("
								+ std::to_string(pHypo->getBayes()) + ")");
				return (false);
			}
		}
	}

	// Use site nucleate to scan all nodes
	// linked to this pick's site and calculate
	// the stacked agoric at each node.  If the threshold
	// is exceeded, the node is added to the site's trigger list
	std::vector < std::shared_ptr < CTrigger >> vTrigger = pickSite->nucleate(
			tPick);

	// if there were no triggers, we're done
	if (vTrigger.size() == 0) {
		glassutil::CLogit::log(
				glassutil::log_level::debug,
				"CPick::nucleate: NOTRG site:" + pickSite->getScnl()
						+ "; tPick:" + pt + "; idPick:" + std::to_string(idPick)
						+ "; sPid:" + sPid);

		return (false);
	}

	for (const auto &trigger : vTrigger) {
		if (trigger->getWeb() == NULL) {
			continue;
		}

		// check to see if the pick is currently associated to a hypo
		if (wpHypo.expired() == false) {
			// get the hypo and compute distance between it and the
			// current trigger
			std::shared_ptr<CHypo> pHypo = wpHypo.lock();
			if (pHypo != NULL) {
				glassutil::CGeo geoHypo = pHypo->getGeo();

				glassutil::CGeo trigHypo = trigger->getGeo();

				double dist = (geoHypo.delta(&trigHypo) / DEG2RAD) * 111.12;

				// is the associated hypo close enough to this trigger to skip
				// close enough means within the resolution of the trigger
				if (dist < trigger->getResolution()) {
					glassutil::CLogit::log(
							glassutil::log_level::debug,
							"CPick::nucleate: SKIPTRG because pick proximal hypo ("
									+ std::to_string(dist) + " < "
									+ std::to_string(trigger->getResolution())
									+ ")");
					continue;
				}
			}
		}

		// create the hypo using the node
		std::shared_ptr<CHypo> hypo = std::make_shared < CHypo
				> (trigger, pGlass->getTTT());

		// set hypo glass pointer and such
		hypo->setGlass(pGlass);
		hypo->setCutFactor(pGlass->getCutFactor());
		hypo->setCutPercentage(pGlass->getCutPercentage());
		hypo->setCutMin(pGlass->getCutMin());

		// add links to all the picks that support the hypo
		std::vector < std::shared_ptr < CPick >> vTriggerPicks = trigger
				->getVPick();

		for (auto pick : vTriggerPicks) {
			// they're not associated yet, just potentially
			pick->setAss("N");
			hypo->addPick(pick);
		}

		// use the hypo's nucleation threshold, which is really the
		// web's nucleation threshold
		int ncut = hypo->getCut();
		double thresh = hypo->getThresh();
		bool bad = false;

		// First localization attempt after nucleation
		// make 3 passes
		for (int ipass = 0; ipass < 3; ipass++) {
			// get an initial location via synthetic annealing,
			// which also prunes out any poorly fitting picks
			// the search is based on the grid resolution, and the how
			// far out the ot can change without losing the initial pick
			// this all assumes that the closest grid triggers
			// values derived from testing global event association
			double bayes = hypo->anneal(
					10000, trigger->getResolution() / 2.,
					trigger->getResolution() / 100.,
					std::max(trigger->getResolution() / 10.0, 5.0), .1);

			// get the number of picks we have now
			int npick = hypo->getVPickSize();

			snprintf(sLog, sizeof(sLog), "CPick::nucleate: -- Pass:%d; nPick:%d"
						"/nCut:%d; bayes:%f/thresh:%f; %s",
						ipass, npick, ncut, bayes, thresh,
						hypo->getPid().c_str());
			glassutil::CLogit::log(sLog);

			// check to see if we still have a high enough bayes value for this
			// hypo to survive.
			if (bayes < thresh) {
				// it isn't
				snprintf(sLog, sizeof(sLog),
							"CPick::nucleate: -- Abandoning solution %s "
							"due to low bayes value "
							"(bayes:%f/thresh:%f)",
							hypo->getPid().c_str(), bayes, thresh);
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
							hypo->getPid().c_str(), npick, ncut);
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
		std::string st = glassutil::CDate::encodeDateTime(hypo->getTOrg());
		glassutil::CLogit::log(
				glassutil::log_level::debug,
				"CPick::nucleate: TRG site:" + pickSite->getScnl() + "; tPick:"
						+ pt + "; idPick:" + std::to_string(idPick) + "; sPid:"
						+ sPid + " => web:" + hypo->getWebName() + "; hyp: "
						+ hypo->getPid() + "; lat:"
						+ std::to_string(hypo->getLat()) + "; lon:"
						+ std::to_string(hypo->getLon()) + "; z:"
						+ std::to_string(hypo->getZ()) + "; tOrg:" + st);

		// if we got this far, the hypo has enough supporting data to
		// merit adding it to the hypo list
		pGlass->getHypoList()->addHypo(hypo);
	}

	// done
	return (true);
}

double CPick::getBackAzimuth() const {
	return (dBackAzimuth);
}

double CPick::getSlowness() const {
	return (dSlowness);
}

int CPick::getIdPick() const {
	return (idPick);
}

const std::shared_ptr<json::Object>& CPick::getJPick() const {
	return (jPick);
}

const std::shared_ptr<CHypo> CPick::getHypo() const {
	std::lock_guard < std::recursive_mutex > pickGuard(pickMutex);
	return (wpHypo.lock());
}

const std::string CPick::getHypoPid() const {
	std::lock_guard < std::recursive_mutex > pickGuard(pickMutex);
	std::string hypoPid = "";

	// make sure we have a hypo,
	if (wpHypo.expired() == true) {
		return (hypoPid);
	}

	// get the hypo
	std::shared_ptr<CHypo> pHypo = getHypo();
	if (pHypo != NULL) {
		// get the hypo pid
		hypoPid = pHypo->getPid();
	}

	return (hypoPid);
}

const std::shared_ptr<CSite> CPick::getSite() const {
	std::lock_guard < std::recursive_mutex > pickGuard(pickMutex);
	return (wpSite.lock());
}

const std::string& CPick::getAss() const {
	std::lock_guard < std::recursive_mutex > pickGuard(pickMutex);
	return (sAss);
}

const std::string& CPick::getPhs() const {
	return (sPhs);
}

const std::string& CPick::getPid() const {
	return (sPid);
}

double CPick::getTPick() const {
	return (tPick);
}

}  // namespace glasscore

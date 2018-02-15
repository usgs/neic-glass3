#include <json.h>
#include <memory>
#include <string>
#include "Pid.h"
#include "Web.h"
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
CPick::CPick(json::Object *pick, int pickId, CSiteList *pSiteList) {
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
	if (!site->bUse) {
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

	std::lock_guard<std::recursive_mutex> guard(pickMutex);

	// remember input json for hypo message generation
	// note, move to init?
	std::shared_ptr<json::Object> jpck(new json::Object(*pick));
	jPick = jpck;
}

// ---------------------------------------------------------~CPick
CPick::~CPick() {
	clear();
}

// ---------------------------------------------------------~clear
void CPick::clear() {
	std::lock_guard<std::recursive_mutex> guard(pickMutex);

	pSite = NULL;
	pHypo = NULL;
	jPick = NULL;

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
	std::lock_guard<std::recursive_mutex> guard(pickMutex);

	clear();

	// nullcheck
	if (pickSite == NULL) {
		return (false);
	}

	pSite = pickSite;
	tPick = pickTime;
	idPick = pickId;
	sPid = pickIdString;
	dBackAzimuth = backAzimuth;
	dSlowness = slowness;

	glassutil::CLogit::log(
			glassutil::log_level::debug,
			"CPick::initialize: site:" + pSite->sScnl + "; tPick:"
					+ std::to_string(tPick) + "; idPick:"
					+ std::to_string(idPick) + "; sPid:" + sPid);

	return (true);
}

// ---------------------------------------------------------addHypo
void CPick::addHypo(std::shared_ptr<CHypo> hyp, std::string ass, bool force) {
	std::lock_guard<std::recursive_mutex> guard(pickMutex);

	// nullcheck
	if (hyp == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CPick::addHypo: NULL hypo provided.");
		return;
	}

	// Add hypo data reference to this pick
	if (force == true) {
		pHypo = hyp;
		sAss = ass;
	} else if (!pHypo) {
		pHypo = hyp;
		sAss = ass;
	}
}

// ---------------------------------------------------------remHypo
void CPick::remHypo(std::shared_ptr<CHypo> hyp) {
	std::lock_guard<std::recursive_mutex> guard(pickMutex);

	// nullcheck
	if (hyp == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CPick::remHypo: NULL hypo provided.");
		return;
	}

	// Remove hypo reference from this pick
	if (pHypo->getPid() == hyp->getPid()) {
		pHypo = NULL;
	}
}

void CPick::clearHypo() {
	std::lock_guard<std::recursive_mutex> guard(pickMutex);
	pHypo = NULL;
}

void CPick::setAss(std::string ass) {
	std::lock_guard<std::recursive_mutex> guard(pickMutex);

	sAss = ass;
}

// ---------------------------------------------------------Nucleate
bool CPick::nucleate() {
	// get CGlass pointer from site
	CGlass *pGlass = pSite->pGlass;

	// nullcheck
	if (pGlass == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CPick::nucleate: NULL pGlass.");
		return (false);
	}

	char sLog[1024];

	// Use site nucleate to scan all nodes
	// linked to this pick's site and calculate
	// the stacked agoric at each node.  If the threshold
	// is exceeded, the node is added to the site's trigger list
	pSite->nucleate(tPick);

	// Set glass values for debugging
	pGlass->sWeb = "";
	pGlass->nCount = 0;
	pGlass->dSum = 0.0;

	// for each node that triggered linked to this
	// pick's site
	pSite->vTriggerMutex.lock();
	for (auto node : pSite->vTrigger) {
		// Set glass values for debugging
		pGlass->sWeb = node->pWeb->sName;
		pGlass->nCount = node->nCount;
		pGlass->dSum = node->dSum;

		snprintf(sLog, sizeof(sLog), "CPick::nucleate: %s %d %.2f",
					pGlass->sWeb.c_str(), pGlass->nCount, pGlass->dSum);
		glassutil::CLogit::log(sLog);

		// nucleate at the node to build the
		// list of picks that support this node
		// tOrg was set during nucleation pass
		// NOTE: this did not used to check the nucleate return here
		// it seems that this should improve computational performance
		// but MIGHT have unintended consequences.
		if (!node->nucleate(node->tOrg, true)) {
			// didn't nucleate anything
			continue;
		}

		// create the hypo using the node
		pGlass->m_TTTMutex.lock();
		std::shared_ptr<CHypo> hypo = std::make_shared<CHypo>(node,
																pGlass->pTTT);
		pGlass->m_TTTMutex.unlock();

		// set hypo glass pointer and such
		hypo->setGlass(pGlass);
		hypo->setCutFactor(pGlass->dCutFactor);
		hypo->setCutPercentage(pGlass->dCutPercentage);
		hypo->setCutMin(pGlass->dCutMin);

		// add links to all the picks that support the hypo
		for (auto pick : node->vPick) {
			// they're not associated yet, just potentially
			pick->setAss("N");
			hypo->addPick(pick);

			// debug logging
			if (pGlass->bTrack) {
				snprintf(sLog, sizeof(sLog),
							"CPick::nucleate: Adding to hyp %s\n",
							pick->pSite->sScnl.c_str());
				glassutil::CLogit::log(sLog);
			}
		}

		// use the hypo's nucleation threshold, which is really the
		// web's nucleation threshold
		int ncut = hypo->getCut();
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

			hypo->anneal(10000, node->dResolution / 2., node->dResolution / 10.,
							node->dResolution / 10.0, .1);

			// get the number of picks we have now
			int npick = hypo->getVPickSize();

			snprintf(sLog, sizeof(sLog), "CPick::nucleate: -- Pass %d %d/%d %s",
						ipass, npick, ncut, hypo->getPid().c_str());
			glassutil::CLogit::log(sLog);

			// check to see if we still have enough picks for this hypo to
			// survive.
			// NOTE, in Node, ncut is used as a threshold for the number of
			// *stations* here it's used for the number of *picks*, which only
			// since we only nucleate on a single phase.
			if (npick < ncut) {
				// we don't
				snprintf(sLog, sizeof(sLog),
							"CPick::nucleate: -- Abandoning this solution %s",
							hypo->getPid().c_str());
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

		// if we got this far, the hypo has enough supporting data to
		// merit looking at it closer.  Process it using evolve
		if (pGlass->pHypoList->evolve(hypo, 1)) {
			// the hypo survived evolve,
			// log the hypo
			std::string st = glassutil::CDate::encodeDateTime(hypo->getTOrg());
			snprintf(sLog, sizeof(sLog),
						"CPick::nucleate: TRG hypo:%s %s %9.4f %10.4f %6.1f %d",
						hypo->getPid().c_str(), st.c_str(), hypo->getLat(),
						hypo->getLon(), hypo->getZ(),
						static_cast<int>(hypo->getVPickSize()));
			glassutil::CLogit::log(sLog);

			// log the triggering web
			snprintf(sLog, sizeof(sLog), "CPick::nucleate: WEB %s\n",
						node->sName.c_str());
			glassutil::CLogit::log(sLog);

			// add a link to the hypo for each pick
			// NOTE: Why is this done? Shouldn't evolve have already
			// linked the picks?
			// this hypo isn't added to the list yet, so this
			/* for (auto q : hypo->vPick) {
			 // if the pick hasn't been linked
			 if (q->pHypo == NULL) {
			 // link it
			 q->pHypo = hypo;
			 }
			 }*/

			// add new hypo to hypo list
			pGlass->pHypoList->addHypo(hypo);
		}
	}

	int triggerCount = pSite->vTrigger.size();
	pSite->vTriggerMutex.unlock();

	// If any webs triggered, return true to prevent further association
	if (triggerCount > 0) {
		return (true);
	}

	snprintf(sLog, sizeof(sLog), "CPick::nucleate: NOTRG idPick:%d sPid:%s",
				idPick, sPid.c_str());
	glassutil::CLogit::log(sLog);

	// done
	return (false);
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

const std::shared_ptr<CHypo>& CPick::getHypo() {
	std::lock_guard<std::recursive_mutex> pickGuard(pickMutex);
	return (pHypo);
}

const std::shared_ptr<CSite>& CPick::getSite() const {
	return (pSite);
}

const std::string& CPick::getAss() {
	std::lock_guard<std::recursive_mutex> pickGuard(pickMutex);
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

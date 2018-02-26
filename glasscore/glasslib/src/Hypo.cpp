#include <json.h>
#include <string>
#include <algorithm>
#include <memory>
#include <vector>
#include <mutex>
#include <random>
#include "Pid.h"
#include "Date.h"
#include "TTT.h"
#include "Brent.h"
#include "Hypo.h"
#include "Site.h"
#include "PickList.h"
#include "HypoList.h"
#include "Pick.h"
#include "Correlation.h"
#include "Trigger.h"
#include "Web.h"
#include "Glass.h"
#include "Logit.h"
#include "Taper.h"
#include <fstream>

namespace glasscore {

/**
 * \brief Pick sorting function used by CHypo, sorts by pick time
 */
static bool sortPick(std::shared_ptr<CPick> pck1, std::shared_ptr<CPick> pck2) {
	if (pck2->getTPick() > pck1->getTPick()) {
		return (true);
	}

	return (false);
}

// ---------------------------------------------------------CHypo
CHypo::CHypo() {
	// seed the random number generator
	std::random_device randomDevice;
	m_RandomGenerator.seed(randomDevice());

	clear();
}

// ---------------------------------------------------------CHypo
CHypo::CHypo(double lat, double lon, double z, double time, std::string pid,
				std::string web, double bayes, double thresh, int cut,
				std::shared_ptr<traveltime::CTravelTime> firstTrav,
				std::shared_ptr<traveltime::CTravelTime> secondTrav,
				std::shared_ptr<traveltime::CTTT> ttt, double resolution) {
	// seed the random number generator
	std::random_device randomDevice;
	m_RandomGenerator.seed(randomDevice());

	if (!initialize(lat, lon, z, time, pid, web, bayes, thresh, cut, firstTrav,
					secondTrav, ttt, resolution)) {
		clear();
	}
}

// ---------------------------------------------------------CHypo
CHypo::CHypo(std::shared_ptr<CTrigger> trigger,
				std::shared_ptr<traveltime::CTTT> ttt) {
	// seed the random number generator
	std::random_device randomDevice;
	m_RandomGenerator.seed(randomDevice());

	// null checks
	if (trigger == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::CHypo: NULL node.");

		clear();
		return;
	}

	if (trigger->getWeb() == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::CHypo: NULL node->getWeb().");

		clear();
		return;
	}

	if (!initialize(trigger->getLat(), trigger->getLon(), trigger->getZ(),
					trigger->getTOrg(), glassutil::CPid::pid(),
					trigger->getWeb()->getName(), trigger->getSum(),
					trigger->getWeb()->getThresh(),
					trigger->getWeb()->getNucleate(),
					trigger->getWeb()->getTrv1(), trigger->getWeb()->getTrv2(),
					ttt, trigger->getResolution())) {
		clear();
	}
}

// ---------------------------------------------------------CHypo
CHypo::CHypo(std::shared_ptr<CCorrelation> corr,
				std::shared_ptr<traveltime::CTravelTime> firstTrav,
				std::shared_ptr<traveltime::CTravelTime> secondTrav,
				std::shared_ptr<traveltime::CTTT> ttt) {
	// seed the random number generator
	std::random_device randomDevice;
	m_RandomGenerator.seed(randomDevice());
	pTTT = NULL;
	pTrv1 = NULL;
	pTrv2 = NULL;

	// null checks
	if (corr == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::CHypo: NULL correlation.");

		clear();
		return;
	}

	if (!initialize(corr->getLat(), corr->getLon(), corr->getZ(),
					corr->getTOrg(), glassutil::CPid::pid(), "Correlation", 0.0,
					0.0, 0.0, firstTrav, secondTrav, ttt, 0.0)) {
		clear();
	}
}

// ---------------------------------------------------------~CHypo
CHypo::~CHypo() {
	clear();
}

// ---------------------------------------------------------clear
void CHypo::clear() {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(hypoMutex);

	dLat = 0.0;
	dLon = 0.0;
	dZ = 0.0;
	tOrg = 0.0;
	sPid = "";
	nCut = 0;
	dThresh = 0.0;
	dBayes = 0.0;
	dBayesInitial = 0.0;
	iCycle = 0;
	nWts = 0;
	sWebName = "";
	dMed = 0;
	dMin = 0;
	dGap = 0;
	dSig = 0;
	dKrt = 0;
	dRes = 0;

	dCutFactor = 4.0;
	dCutPercentage = 0.4;
	dCutMin = 30.0;

	bFixed = false;
	bEvent = false;

	pGlass = NULL;

	pTTT.reset();
	pTrv1.reset();
	pTrv2.reset();

	bCorrAdded = false;

	clearPicks();
	clearCorrelations();
	vWts.clear();

	processCount = 0;
	reportCount = 0;
}

// ---------------------------------------------------------initialize
bool CHypo::initialize(double lat, double lon, double z, double time,
						std::string pid, std::string web, double bayes,
						double thresh, int cut,
						std::shared_ptr<traveltime::CTravelTime> firstTrav,
						std::shared_ptr<traveltime::CTravelTime> secondTrav,
						std::shared_ptr<traveltime::CTTT> ttt,
						double resolution) {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(hypoMutex);

	clear();

	dLat = lat;
	dLon = lon;
	dZ = z;
	tOrg = time;
	sPid = pid;
	sWebName = web;
	dBayes = bayes;
	dBayesInitial = bayes;

	dThresh = thresh;
	nCut = cut;
	dRes = resolution;
	if (dRes == 0.) {
		dRes = 100.;
	}

	glassutil::CLogit::log(
			glassutil::log_level::debug,
			"CHypo::initialize: lat:" + std::to_string(dLat) + "; lon:"
					+ std::to_string(dLon) + "; z:" + std::to_string(dZ)
					+ "; t:" + std::to_string(tOrg) + "; sPid:" + sPid
					+ "; web:" + sWebName + "; bayes:" + std::to_string(dBayes)
					+ "; thresh:" + std::to_string(dThresh) + "; cut:"
					+ std::to_string(nCut) + "; resolution:"
					+ std::to_string(dRes));

	// make local copies of the travel times so that we don't
	// have cross-thread contention for them between hypos
	if (firstTrav != NULL) {
		pTrv1 = std::make_shared<traveltime::CTravelTime>(
				traveltime::CTravelTime(*firstTrav));
	}

	if (secondTrav != NULL) {
		pTrv2 = std::make_shared<traveltime::CTravelTime>(
				traveltime::CTravelTime(*secondTrav));
	}

	if (ttt != NULL) {
		pTTT = std::make_shared<traveltime::CTTT>(traveltime::CTTT(*ttt));
	}
	tCreate = glassutil::CDate::now();

	return (true);
}

// ---------------------------------------------------------addPick
void CHypo::addPick(std::shared_ptr<CPick> pck) {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(hypoMutex);

	// null check
	if (pck == NULL) {
		glassutil::CLogit::log(glassutil::log_level::warn,
								"CHypo::addPick: NULL pck.");
		return;
	}

	// for each pick in the vector
	for (auto q : vPick) {
		// see if we have this same pick
		// NOTE: this only checks by ID, need to improve this to
		// an ID/Source check
		if (q->getPid() == pck->getPid()) {
			char sLog[1024];
			snprintf(sLog, sizeof(sLog), "CHypo::addPick: ** Duplicate pick %s",
						pck->getSite()->getScnl().c_str());
			glassutil::CLogit::log(sLog);

			// NOTE: shouldn't we not add this duplicate pick?
		}
	}

	// add the pick to the vector.
	vPick.push_back(pck);
}

// ---------------------------------------------------------remPick
void CHypo::remPick(std::shared_ptr<CPick> pck) {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(hypoMutex);

	// null check
	if (pck == NULL) {
		glassutil::CLogit::log(glassutil::log_level::warn,
								"CHypo::remPick: NULL pck.");
		return;
	}

	// get the pick id
	std::string pid = pck->getPid();

	// for each pick in the vector
	for (int i = 0; i < vPick.size(); i++) {
		// get the current pick
		auto pick = vPick[i];

		// is this pick a match?
		if (pick->getPid() == pid) {
			// remove pick from vector
			vPick.erase(vPick.cbegin() + i);

			return;
		}
	}
}

bool CHypo::hasPick(std::shared_ptr<CPick> pck) {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(hypoMutex);

	// null check
	if (pck == NULL) {
		glassutil::CLogit::log(glassutil::log_level::warn,
								"CHypo::hasPick: NULL pck.");
		return (false);
	}

	// for each pick in the vector
	for (const auto &q : vPick) {
		// is this pick a match?
		if (q->getPid() == pck->getPid()) {
			return (true);
		}
	}

	return (false);
}

void CHypo::clearPicks() {
	// lock the hypo since we're iterating through it's lists
	std::lock_guard<std::recursive_mutex> hypoGuard(hypoMutex);

	// go through all the picks linked to this hypo
	for (auto pck : vPick) {
		if (pck == NULL) {
			continue;
		}

		// if the current pick is linked to this hypo
		if ((pck->getHypo() != NULL) && (sPid == pck->getHypo()->getPid())) {
			// remove hypo link from this pick
			pck->clearHypo();
		}
	}

	// remove all pick links to this hypo
	vPick.clear();
}

// ---------------------------------------------------------addPick
void CHypo::addCorrelation(std::shared_ptr<CCorrelation> corr) {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(hypoMutex);

	// null check
	if (corr == NULL) {
		glassutil::CLogit::log(glassutil::log_level::warn,
								"CHypo::addCorrelation: NULL correlation.");
		return;
	}

	// for each correlation in the vector
	for (auto q : vCorr) {
		// see if we have this same correlation
		// NOTE: this only checks by ID, need to improve this to
		// an ID/Source check
		if (q->getPid() == corr->getPid()) {
			char sLog[1024];
			snprintf(sLog, sizeof(sLog),
						"CHypo::addCorrelation: ** Duplicate correlation %s",
						corr->getSite()->getScnl().c_str());
			glassutil::CLogit::log(sLog);

			return;
		}
	}

	// add the pick to the vector.
	vCorr.push_back(corr);
	bCorrAdded = true;
}

// ---------------------------------------------------------remPick
void CHypo::remCorrelation(std::shared_ptr<CCorrelation> corr) {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(hypoMutex);

	// null check
	if (corr == NULL) {
		glassutil::CLogit::log(glassutil::log_level::warn,
								"CHypo::remCorrelation: NULL correlation.");
		return;
	}

	// get the correlation id
	std::string pid = corr->getPid();

	// for each correlation in the vector
	for (int i = 0; i < vCorr.size(); i++) {
		// get the current correlation
		auto correlation = vCorr[i];

		// is this correlation a match?
		if (correlation->getPid() == pid) {
			// remove correlation from vector
			vCorr.erase(vCorr.cbegin() + i);
			return;
		}
	}
}

bool CHypo::hasCorrelation(std::shared_ptr<CCorrelation> corr) {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(hypoMutex);

	// null check
	if (corr == NULL) {
		glassutil::CLogit::log(glassutil::log_level::warn,
								"CHypo::hasCorrelation: NULL correlation.");
		return (false);
	}

	// for each corr in the vector
	for (const auto &q : vCorr) {
		// is this corr a match?
		if (q->getPid() == corr->getPid()) {
			return (true);
		}
	}

	return (false);
}

void CHypo::clearCorrelations() {
	// lock the hypo since we're iterating through it's lists
	std::lock_guard<std::recursive_mutex> hypoGuard(hypoMutex);

	// go through all the picks linked to this hypo
	for (auto corr : vCorr) {
		if (corr == NULL) {
			continue;
		}

		// if the current pick is linked to this hypo
		if ((corr->getHypo() != NULL) && (sPid == corr->getHypo()->getPid())) {
			// remove hypo link from this pick
			corr->clearHypo();
		}
	}

	// remove all correlation links to this hypo
	vCorr.clear();
}

// ---------------------------------------------------------Gaussian
double CHypo::gauss(double avg, double std) {
	// generate Gaussian pseudo-random number using the
	// polar form of the Box-Muller method
	// NOTE: Move to some glass math utility library?
	double rsq = 0;
	double v1 = 0;

	do {
		v1 = Rand(-1.0, 1.0);
		double v2 = Rand(-1.0, 1.0);
		rsq = v1 * v1 + v2 * v2;
	} while (rsq >= 1.0);

	double fac = sqrt(-2.0 * log(rsq) / rsq);
	double x = std * fac * v1 + avg;

	// return random normal deviate
	return (x);
}

// ---------------------------------------------------------Rand
double CHypo::Rand(double x, double y) {
	// double randNum = ((x) + ((y) - (x)) * rand() / (float) RAND_MAX);
	// return randNum;
	// NOTE: Move to some glass math utility library?
	std::uniform_real_distribution<double> distribution(x, y);
	double number = distribution(m_RandomGenerator);
	return (number);
}

// ---------------------------------------------------------Hypo
std::shared_ptr<json::Object> CHypo::hypo() {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(hypoMutex);

	std::shared_ptr<json::Object> hypo = std::make_shared<json::Object>(
			json::Object());

	// null check
	if (pGlass == NULL) {
		glassutil::CLogit::log(glassutil::log_level::warn,
								"CHypo::Hypo: NULL pGlass.");
		return (hypo);
	}
	if (pTTT == NULL) {
		glassutil::CLogit::log(glassutil::log_level::warn,
								"CHypo::Hypo: NULL pTTT.");
		return (hypo);
	}

	// make sure this event is still reportable
	if (reportCheck() == false) {
		glassutil::CLogit::log(
				glassutil::log_level::debug,
				"CHypo::Hypo: hypo:" + sPid + " is not reportable.");
		return (hypo);
	}

	glassutil::CLogit::log(
			glassutil::log_level::debug,
			"CHypo::Hypo: generating hypo message for sPid:" + sPid
					+ " sWebName:" + sWebName);

	// NOTE: Need to think about this format, currently it *almost*
	// creates a detection formats json, but doesn't use the library
	// maybe this should be it's own "glass" format? or just use
	// the quake format?

	// basic info
	(*hypo)["Cmd"] = "Hypo";
	(*hypo)["Type"] = "Hypo";
	(*hypo)["ID"] = sPid;

	// source
	// NOTE: THIS NEEDS TO BE NOT HARDCODED EVENTUALLY
	// or remove it? why should glasscore care about source?
	json::Object src;
	src["AgencyID"] = "US";
	src["Author"] = "glass";
	(*hypo)["Source"] = src;

	// time
	(*hypo)["T"] = glassutil::CDate::encodeDateTime(tOrg);
	(*hypo)["Time"] = glassutil::CDate::encodeISO8601Time(tOrg);

	// location
	(*hypo)["Latitude"] = dLat;
	(*hypo)["Longitude"] = dLon;
	(*hypo)["Depth"] = dZ;

	// supplementary info
	(*hypo)["MinimumDistance"] = dMin;
	(*hypo)["Gap"] = dGap;
	(*hypo)["Bayes"] = dBayes;
	(*hypo)["InitialBayes"] = dBayesInitial;
	(*hypo)["Web"] = sWebName;

	// generate data array for this hypo
	// set up traveltime object
	pTTT->setOrigin(dLat, dLon, dZ);

	// set up geo for distance calculations
	glassutil::CGeo geo;
	geo.setGeographic(dLat, dLon, 6371.0 - dZ);

	// array to hold data
	json::Array data;

	// for each pick
	for (auto pick : vPick) {
		// get basic pick values
		std::shared_ptr<CSite> site = pick->getSite();
		double tobs = pick->getTPick() - tOrg;
		double tcal = pTTT->T(&site->getGeo(), tobs);
		double tres = tobs - tcal;
		// should this be changed?
		double sig = pGlass->sig(tres, 1.0);

		// if we have it, use the shared pointer
		json::Object pickObj;
		std::shared_ptr<json::Object> jPick = pick->getJPick();
		if (jPick) {
			// start with a copy of json pick
			// which has the site, source, time, etc
			pickObj = json::Object(*jPick.get());

			// add the association info
			json::Object assocobj;
			assocobj["Phase"] = pTTT->sPhase;
			assocobj["Distance"] = geo.delta(&site->getGeo()) / DEG2RAD;
			assocobj["Azimuth"] = geo.azimuth(&site->getGeo()) / DEG2RAD;
			assocobj["Residual"] = tres;
			assocobj["Sigma"] = sig;
			pickObj["AssociationInfo"] = assocobj;
		} else {
			// we don't have a jpick, so fill in what we know
			pickObj["Site"] = site->getScnl();
			pickObj["Pid"] = pick->getPid();
			pickObj["T"] = glassutil::CDate::encodeDateTime(pick->getTPick());
			pickObj["Time"] = glassutil::CDate::encodeISO8601Time(
					pick->getTPick());
			pickObj["Distance"] = geo.delta(&site->getGeo()) / DEG2RAD;
			pickObj["Azimuth"] = geo.azimuth(&site->getGeo()) / DEG2RAD;
			pickObj["Residual"] = tres;
		}

		// add new pick to list
		data.push_back(pickObj);
	}

	// for each correlation
	for (auto correlation : vCorr) {
		// get basic pick values
		std::shared_ptr<CSite> site = correlation->getSite();
		double tobs = correlation->getTCorrelation() - tOrg;
		double tcal = pTTT->T(&site->getGeo(), tobs);
		double tres = tobs - tcal;
		// should this be changed?
		double sig = pGlass->sig(tres, 1.0);

		// if we have it, use the shared pointer
		json::Object correlationObj;
		std::shared_ptr<json::Object> jCorrelation = correlation
				->getJCorrelation();
		if (jCorrelation) {
			// start with a copy of json pick
			// which has the site, source, time, etc
			correlationObj = json::Object(*jCorrelation.get());

			// add the association info
			json::Object assocobj;
			assocobj["Phase"] = pTTT->sPhase;
			assocobj["Distance"] = geo.delta(&site->getGeo()) / DEG2RAD;
			assocobj["Azimuth"] = geo.azimuth(&site->getGeo()) / DEG2RAD;
			assocobj["Residual"] = tres;
			assocobj["Sigma"] = sig;
			correlationObj["AssociationInfo"] = assocobj;
		} else {
			// we don't have a jCorrelation, so fill in what we know
			correlationObj["Site"] = site->getScnl();
			correlationObj["Pid"] = correlation->getPid();
			correlationObj["Time"] = glassutil::CDate::encodeISO8601Time(
					correlation->getTCorrelation());
			correlationObj["Latitude"] = correlation->getLat();
			correlationObj["Longitude"] = correlation->getLon();
			correlationObj["Depth"] = correlation->getZ();
			correlationObj["Distance"] = geo.delta(&site->getGeo()) / DEG2RAD;
			correlationObj["Azimuth"] = geo.azimuth(&site->getGeo()) / DEG2RAD;
			correlationObj["Residual"] = tres;
			correlationObj["Correlation"] = correlation->getCorrelation();
		}

		// add new pick to list
		data.push_back(correlationObj);
	}

	// add data array to object
	(*hypo)["Data"] = data;

	if (pGlass) {
		pGlass->send(hypo);
	}

	// done
	return (hypo);
}

// ---------------------------------------------------------Event
void CHypo::event() {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(hypoMutex);

	bEvent = true;
	reportCount++;
	std::shared_ptr<json::Object> event = std::make_shared<json::Object>(
			json::Object());

	// fill in Event command from current hypocenter
	(*event)["Cmd"] = "Event";
	(*event)["Pid"] = sPid;
	(*event)["CreateTime"] = glassutil::CDate::encodeISO8601Time(tCreate);
	(*event)["ReportTime"] = glassutil::CDate::encodeISO8601Time(
			glassutil::CDate::now());
	(*event)["Version"] = reportCount;

	// basic hypo information
	(*event)["Latitude"] = dLat;
	(*event)["Longitude"] = dLon;
	(*event)["Depth"] = dZ;
	(*event)["Time"] = glassutil::CDate::encodeISO8601Time(tOrg);
	(*event)["Bayes"] = dBayes;
	(*event)["Ndata"] = static_cast<int>(vPick.size())
			+ static_cast<int>(vCorr.size());

	// send it
	if (pGlass) {
		pGlass->send(event);
	}
}

// ---------------------------------------------------------associate
bool CHypo::associate(std::shared_ptr<CPick> pick, double sigma,
						double sdassoc) {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(hypoMutex);

	// null check
	if (pick == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::associate: NULL pick.");
		return (false);
	}
	if (pGlass == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::associate: NULL pGlass.");
		return (false);
	}
	if (pTTT == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::associate: NULL pTTT.");
		return (false);
	}

	double dAzimuthRange = pGlass->getBeamMatchingAzimuthWindow();
	// double dDistanceRange = pGlass->getBeamMatchingDistanceWindow();

	// set up a geographic object for this hypo
	glassutil::CGeo hypoGeo;
	hypoGeo.setGeographic(dLat, dLon, 6371.0 - dZ);

	// setup traveltime interface for this hypo
	pTTT->setOrigin(dLat, dLon, dZ);

	// get site
	std::shared_ptr<CSite> site = pick->getSite();

	// compute distance
	double siteDistance = hypoGeo.delta(&site->getGeo()) / DEG2RAD;

	// check if distance is beyond cutoff
	if (siteDistance > dCut) {
		// it is, don't associated
		return (false);
	}

	// compute observed traveltime
	double tObs = pick->getTPick() - tOrg;

	// get expected travel time
	double tCal = pTTT->T(&site->getGeo(), tObs);

	// Check if pick has an invalid travel time,
	if (tCal < 0.0) {
		// it does, don't associated
		return (false);
	}

	// check backazimuth if present
	if (pick->getBackAzimuth() > 0) {
		// compute azimith from the site to the node
		double siteAzimuth = site->getGeo().azimuth(&hypoGeo);

		// check to see if pick's backazimuth is within the
		// valid range
		if ((pick->getBackAzimuth() < (siteAzimuth - dAzimuthRange))
				|| (pick->getBackAzimuth() > (siteAzimuth + dAzimuthRange))) {
			// it is not, do not associate
			return (false);
		}
	}

	// check slowness if present
	// Need modify travel time libraries to support getting distance
	// from slowness, and it's of limited value compared to the back
	// azimuth check
	/* if (pick->dSlowness > 0) {
	 // compute observed distance from slowness (1/velocity)
	 // and tObs (distance = velocity * time)
	 double obsDistance = (1 / pick->dSlowness) * tObs;

	 // check to see if the observed distance is within the
	 // valid range
	 if ((obsDistance < (siteDistance - dDistanceRange))
	 || (obsDistance > (siteDistance + dDistanceRange))) {
	 // it is not, do not associate
	 return (false);
	 }
	 } */

	// compute absolute residual
	double tRes = tObs - tCal;
	if (tRes < 0.0) {
		tRes = -tRes;
	}

	// compute pick standard deviation from residual
	// and sigma
	double stdev = tRes / sigma;

	char sLog[1024];

	// check if pick standard deviation is greater than cutoff
	if (stdev > sdassoc) {
		/*
		 snprintf(
		 sLog, sizeof(sLog),
		 "CHypo::associate: NOASSOC Hypo:%s Time:%s Station:%s Pick:%s"
		 " stdev:%.2f>sdassoc:%.2f)",
		 sPid.c_str(),
		 glassutil::CDate::encodeDateTime(pick->getTPick()).c_str(),
		 pick->getSite()->getScnl().c_str(), pick->getPid().c_str(), stdev, sdassoc);
		 glassutil::CLogit::log(sLog);
		 */

		// it is, don't associate
		return (false);
	}
	/*
	 snprintf(sLog, sizeof(sLog),
	 "CHypo::associate: ASSOC Hypo:%s Time:%s Station:%s Pick:%s"
	 " stdev:%.2f>sdassoc:%.2f)",
	 sPid.c_str(),
	 glassutil::CDate::encodeDateTime(pick->getTPick()).c_str(),
	 pick->getSite()->getScnl().c_str(), pick->getPid().c_str(), stdev, sdassoc);
	 glassutil::CLogit::log(sLog);
	 */

	// trimming criteria are met, associate
	return (true);
}

// ---------------------------------------------------------associate
bool CHypo::associate(std::shared_ptr<CCorrelation> corr, double tWindow,
						double xWindow) {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(hypoMutex);

	// NOTE: this is a simple time/distance check for association
	// wiser heads than mine may come up with a more robust approach JMP
	// null check
	if (corr == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::associate: NULL correlation.");
		return (false);
	}
	if (pGlass == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::associate: NULL pGlass.");
		return (false);
	}

	char sLog[1024];

	double tDist = 0;
	double xDist = 0;

	// check if time difference is within window
	tDist = std::abs(tOrg - corr->getTCorrelation());
	if (tDist < tWindow) {
		glassutil::CGeo geo1;
		geo1.setGeographic(dLat, dLon, 6371.0 - dZ);
		glassutil::CGeo geo2;
		geo2.setGeographic(corr->getLat(), corr->getLon(),
							6371.0 - corr->getZ());
		xDist = RAD2DEG * geo1.delta(&geo2);

		// check if distance difference is within window
		if (xDist < xWindow) {
			snprintf(
					sLog,
					sizeof(sLog),
					"CHypo::associate: C-ASSOC Hypo:%s Time:%s Station:%s"
					" Corr:%s tDist:%.2f<tWindow:%.2f"
					" xDist:%.2f>xWindow:%.2f)",
					sPid.c_str(),
					glassutil::CDate::encodeDateTime(corr->getTCorrelation())
							.c_str(),
					corr->getSite()->getScnl().c_str(), corr->getPid().c_str(),
					tDist, tWindow, xDist, xWindow);
			glassutil::CLogit::log(sLog);

			return (true);
		}
	}

	if (xDist == 0) {
		snprintf(
				sLog,
				sizeof(sLog),
				"CHypo::associate: C-NOASSOC Hypo:%s Time:%s Station:%s Corr:%s"
				" tDist:%.2f>tWindow:%.2f",
				sPid.c_str(),
				glassutil::CDate::encodeDateTime(corr->getTCorrelation()).c_str(),
				corr->getSite()->getScnl().c_str(), corr->getPid().c_str(),
				tDist, tWindow);
	} else {
		snprintf(
				sLog,
				sizeof(sLog),
				"CHypo::associate: C-NOASSOC Hypo:%s Time:%s Station:%s Corr:%s"
				" tDist:%.2f<tWindow:%.2f xDist:%.2f>xWindow:%.2f)",
				sPid.c_str(),
				glassutil::CDate::encodeDateTime(corr->getTCorrelation()).c_str(),
				corr->getSite()->getScnl().c_str(), corr->getPid().c_str(),
				tDist, tWindow, xDist, xWindow);
	}
	glassutil::CLogit::log(sLog);

	// it is, don't associate
	return (false);
}

// ---------------------------------------------------------affinity
double CHypo::affinity(std::shared_ptr<CPick> pck) {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(hypoMutex);

	// null checks
	if (pck == NULL) {
		return (0.0);
	}
	if (pGlass == NULL) {
		return (0.0);
	}

	// get the site from the pick
	std::shared_ptr<CSite> site = pck->getSite();

	// get various global parameters from the glass pointer
	// get the standard deviation allowed for association
	double sdassoc = pGlass->getSdAssociate();

	// get the affinity factor
	double expaff = pGlass->getExpAffinity();

	// check to see if this pick can  associate with this hypo using
	// the given association standard deviation
	if (!associate(pck, 1.0, sdassoc)) {
		// the pick did not associate
		return (0.0);
	}

	// The pick can associate,
	// check if the hypo is fixed
	if (bFixed) {
		// just return the hypo's bayesian value
		return (dBayes);
	}

	// now compute the gap factor using a taper
	glassutil::CTaper gap;
	gap = glassutil::CTaper(0.0, 0.0, 270.0, 360.0);
	double gapfac = gap.Val(dGap);

	// compute the affinity of this pick to this hypo by multiplying
	// the gap factor to the hypocenter's current baysian statistic to
	// the affinity power factor
	double aff = gapfac * pow(dBayes, expaff);

	// return the affinity
	return (aff);
}

// ---------------------------------------------------------affinity
double CHypo::affinity(std::shared_ptr<CCorrelation> corr) {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(hypoMutex);

	// NOTE: I'm just combining time/distance into a made up affinity
	// wiser heads than mine may come up with a more robust approach JMP
	// null checks
	if (corr == NULL) {
		return (0.0);
	}
	if (pGlass == NULL) {
		return (0.0);
	}

	// get various global parameters from the glass pointer
	double tWindow = pGlass->getCorrelationMatchingTWindow();
	double xWindow = pGlass->getCorrelationMatchingXWindow();

	// check to see if this correlation can associate with this hypo
	if (!associate(corr, tWindow, xWindow)) {
		// the correlation did not associate
		return (0.0);
	}

	// compute time factor, multiply by 10 to make time factor
	// have equal weight to the distance factor
	double tFactor = std::abs(tOrg - corr->getTCorrelation()) * 10;

	// hypo is in geographic coordinates
	glassutil::CGeo geo1;
	geo1.setGeographic(dLat, dLon, 6371.0 - dZ);

	// correlation is in geographic coordinates
	glassutil::CGeo geo2;
	geo2.setGeographic(corr->getLat(), corr->getLon(), 6371.0 - corr->getZ());

	// compute distance factor
	double xFactor = RAD2DEG * geo1.delta(&geo2);

	// compute the affinity of this hypo to this hypo by taking the inverse
	// of multiplying the time factor to the distance filter
	// note the idea is a higher number the closer the correlation is
	// to the hypo
	double aff = 1 / (tFactor * xFactor);

	// return the affinity
	return (aff);
}

// ---------------------------------------------------------prune
bool CHypo::prune() {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(hypoMutex);

	// nullcheck
	if (pGlass == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::prune: NULL pGlass.");
		return (false);
	}

	char sLog[1024];

	glassutil::CLogit::log(glassutil::log_level::debug,
							"CHypo::prune. " + sPid);

	// set up local vector to track picks to remove
	std::vector<std::shared_ptr<CPick>> vremove;

	// set up a geographic object for this hypo
	glassutil::CGeo geo;
	geo.setGeographic(dLat, dLon, 6371.0);

	// get the standard deviation allowed for pruning
	double sdprune = pGlass->getSdPrune();

	// for each pick in this hypo
	for (auto pck : vPick) {
		// check to see if it can still be associated
		if (!associate(pck, 1.0, sdprune)) {
			// pick no longer associates, add to remove list
			vremove.push_back(pck);

			snprintf(
					sLog, sizeof(sLog), "CHypo::prune: CUL %s %s (%.2f)",
					glassutil::CDate::encodeDateTime(pck->getTPick()).c_str(),
					pck->getSite()->getScnl().c_str(), sdprune);
			glassutil::CLogit::log(sLog);

			// on to the next pick
			continue;
		}

		// Trim whiskers
		// compute delta between site and hypo
		double delta = geo.delta(&pck->getSite()->getGeo());

		// check if delta is beyond distance limit
		if (delta > dCut) {
			snprintf(
					sLog, sizeof(sLog), "CHypo::prune: CUL %s %s (%.2f > %.2f)",
					glassutil::CDate::encodeDateTime(pck->getTPick()).c_str(),
					pck->getSite()->getScnl().c_str(), delta, dCut);
			glassutil::CLogit::log(sLog);

			// add pick to remove list
			vremove.push_back(pck);

			// on to the next pick
			continue;
		}
	}

	int pruneCount = 0;
	for (auto pck : vremove) {
		pruneCount++;
		pck->clearHypo();
		remPick(pck);
	}

	glassutil::CLogit::log(
			glassutil::log_level::debug,
			"CHypo::prune pick pruneCount:" + std::to_string(pruneCount));

	// set up local vector to track correlations to remove
	std::vector<std::shared_ptr<CCorrelation>> vcremove;

	// get the correlation windows
	double tWindow = pGlass->getCorrelationMatchingTWindow();
	double xWindow = pGlass->getCorrelationMatchingXWindow();

	// for each correlation in this hypo
	for (auto cor : vCorr) {
		// check to see if it can still be associated
		if (!associate(cor, tWindow, xWindow)) {
			// correlation no longer associates, add to remove list
			vcremove.push_back(cor);

			snprintf(
					sLog,
					sizeof(sLog),
					"CHypo::prune: C-CUL %s %s",
					glassutil::CDate::encodeDateTime(cor->getTCorrelation())
							.c_str(),
					cor->getSite()->getScnl().c_str());
			glassutil::CLogit::log(sLog);

			// on to the next correlation
			continue;
		}
	}

	for (auto cor : vcremove) {
		cor->clearHypo();
		remCorrelation(cor);
	}

	// if we didn't find any data to remove, just
	// return
	if ((vremove.size() < 1) && (vcremove.size() < 1)) {
		return (false);
	}

	// we've removed at least one data
	return (true);
}

// ---------------------------------------------------------cancel
bool CHypo::cancel() {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(hypoMutex);

	// nullcheck
	if (pGlass == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::cancel: NULL pGlass.");
		return (false);
	}

	// can't cancel fixed hypos
	// NOTE: What implication does this have for "seed hypos" like twitter
	// detections
	if (bFixed) {
		return (false);
	}

	char sLog[2048];
	char sHypo[1024];

	glassutil::CDate dt = glassutil::CDate(tOrg);
	snprintf(sHypo, sizeof(sHypo), "CHypo::cancel: %s tOrg:%s; dLat:%9.4f; "
				"dLon:%10.4f; dZ:%6.1f; bayes:%.2f; nPick:%d; nCorr:%d",
				sPid.c_str(), dt.dateTime().c_str(), dLat, dLon, dZ, dBayes,
				static_cast<int>(vPick.size()), static_cast<int>(vCorr.size()));

	// check to see if there is enough supporting data for this hypocenter
	// NOTE, in Node, ncut is used as a threshold for the number of
	// *stations* here it's used for the number of *picks*, not sure
	// what implication this has
	int ncut = nCut;

	// check correlations, we want a hypo created from a correlation
	// to stick around awhile to have a chance to associate picks.
	if (vCorr.size() > 0) {
		// get the current time
		double now = glassutil::CDate::now();
		int cancelAge = pGlass->getCorrelationCancelAge();

		// check correlations
		int expireCount = 0;
		for (auto cor : vCorr) {
			// count correlation as expired if it's creation time is older than
			// the cancel age
			if ((cor->getTGlassCreate() + cancelAge) < now) {
				snprintf(sLog, sizeof(sLog),
							"CHypo::cancel: Correlation:%s created: %f "
							"limit:%d now:%f",
							sPid.c_str(), cor->getTGlassCreate(), cancelAge,
							now);
				glassutil::CLogit::log(sLog);
				expireCount++;
			}
		}

		// only prevent cancellation if all correlations haven't expired
		if (expireCount == vCorr.size()) {
			snprintf(sLog, sizeof(sLog),
						"CHypo::cancel: $$Could cancel %s due to associated "
						"correlations (%d) older than %d seconds",
						sPid.c_str(),
						(static_cast<int>(vCorr.size()) - expireCount),
						cancelAge);
			glassutil::CLogit::log(sLog);
		} else {
			snprintf(sLog, sizeof(sLog),
						"CHypo::cancel: $$Will not cancel %s due to associated "
						"correlations (%d) younger than %d seconds",
						sPid.c_str(),
						(static_cast<int>(vCorr.size()) - expireCount),
						cancelAge);
			glassutil::CLogit::log(sLog);

			// Hypo is still viable, for now...
			return (false);
		}
	}

	// check data
	// NOTE: Is this a good idea?? JMP
	if ((vPick.size() + vCorr.size()) < ncut) {
		// there isn't
		snprintf(sLog, sizeof(sLog), "CHypo::cancel: Insufficient data "
					"((%d + %d) < %d) Hypo: %s",
					static_cast<int>(vPick.size()),
					static_cast<int>(vCorr.size()), ncut, sHypo);
		glassutil::CLogit::log(sLog);

		// this hypo can be canceled
		return (true);
	}

	// baysian threshold check
	double thresh = dThresh;
	if (dBayes < thresh) {
		// failure
		snprintf(sLog, sizeof(sLog),
					"CHypo::cancel: Below threshold (%.1f < %.1f) Hypo: %s",
					dBayes, thresh, sHypo);
		glassutil::CLogit::log(sLog);

		// this hypo can be canceled
		return (true);
	}

	/**
	 // Whispy check (does the quake have a gap greater than 180 while being
	 // shallower than 400km
	 // NOTE: Hardcoded
	 if ((dZ > 400.0) && (dGap > 180.0)) {
	 // failure
	 snprintf(sLog, sizeof(sLog),
	 "CHypo::cancel: Whispie trap (%.1f>400, %.1f>180) Hypo: %s",
	 dZ, dGap, sHypo);
	 glassutil::CLogit::log(sLog);

	 // this hypo can be canceled
	 return (true);
	 }
	 **/
	// Hypo is still viable
	return (false);
}

// ---------------------------------------------------------cancel
bool CHypo::reportCheck() {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(hypoMutex);

	// nullcheck
	if (pGlass == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::cancel: NULL pGlass.");
		return (false);
	}

	char sLog[2048];
	char sHypo[1024];

	glassutil::CDate dt = glassutil::CDate(tOrg);
	snprintf(sHypo, sizeof(sHypo), "%s %s%9.4f%10.4f%6.1f %d", sPid.c_str(),
				dt.dateTime().c_str(), dLat, dLon, dZ,
				static_cast<int>(vPick.size()));

	int nReportCut = pGlass->getReportCut();

	// check data count
	if ((vPick.size() + vCorr.size()) < nReportCut) {
		// there isn't
		snprintf(sLog, sizeof(sLog),
					"CHypo::reportCheck: Below data count threshold "
					"((%d + %d) < %d) Hypo: %s",
					static_cast<int>(vPick.size()),
					static_cast<int>(vCorr.size()), nReportCut, sHypo);
		glassutil::CLogit::log(sLog);

		// this hypo cannot be reported
		return (false);
	}

	// rms check? other checks?

	// baysian threshold check
	double dReportThresh = pGlass->getReportThresh();
	if (dBayes < dReportThresh) {
		// failure
		snprintf(
				sLog, sizeof(sLog),
				"CHypo::reportCheck: Below bayesian threshold (%.1f < %.1f) "
				"Hypo: %s",
				dBayes, dReportThresh, sHypo);
		glassutil::CLogit::log(sLog);

		// this hypo cannot be reported
		return (false);
	}

	// Hypo can be reported
	return (true);
}

// ---------------------------------------------------------stats
void CHypo::stats() {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(hypoMutex);

	// Calculate the statistical distribution of distance
	// histogram for culling purposes. The actual values are
	// reflected to give 0 mean, which is, of course, completely
	// wierd. =). Consequently, the mean is not of interest.
	// don't bother with calculations if
	// there are no picks
	if (vPick.size() < 1) {
		dSig = 0.0;
		dKrt = 0.0;
		dMed = 0.0;
		dMin = 0.0;
		dCut = 0.0;
		dGap = 360.0;
		return;
	}

	// set up a geographic object for this hypo
	glassutil::CGeo geo;
	geo.setGeographic(dLat, dLon, 6371.0 - dZ);

	// create and populate vectors containing the
	// pick distances and azimuths
	std::vector<double> dis;
	std::vector<double> azm;
	for (auto pick : vPick) {
		// get the site
		std::shared_ptr<CSite> site = pick->getSite();

		// compute the distance delta
		double delta = geo.delta(&site->getGeo()) / DEG2RAD;

		// add to distance vactor
		dis.push_back(delta);

		// compute the azimuth
		double azimuth = geo.azimuth(&site->getGeo()) / DEG2RAD;

		// add to azimuth vector
		azm.push_back(azimuth);
	}

	// Calculate distance standard deviation. Note that the denominator is N
	// and not N-1, since a mean of 0 is pre-ordained.
	// The skewness is also 0, since the distribution
	// is exactly symmetric
	double sum = 0.0;
	int ndis = dis.size();
	for (int i = 0; i < ndis; i++) {
		double d = dis[i];
		sum += d * d;
	}
	double var = sum / ndis;
	dSig = sqrt(var);

	// calculate the sample excess kurtosis value
	sum = 0.0;
	for (int i = 0; i < ndis; i++) {
		double arg = dis[i] / dSig;
		sum += arg * arg * arg * arg;
	}
	dKrt = sum / ndis - 3.0;

	// sort the distances
	sort(dis.begin(), dis.end());

	// get minimum distance
	dMin = dis.front();

	// get median distance
	dMed = dis[ndis / 2];

	// compute distance cutoff
	// NOTE: Harley want's both the original .4 and 4.0 values to be
	// configurable via glass_init.
	// In the long term, he wants to replace this with a more statistics
	// based algorithm
	int icut = static_cast<int>((dCutPercentage * ndis));
	dCut = dCutFactor * dis[icut];

	// make sure our calculated dCut is not below the minimum allowed
	if (dCut < dCutMin) {
		dCut = dCutMin;
	}

	// sort the azimuths
	sort(azm.begin(), azm.end());

	// add the first (smallest) azimuth to the end by adding 360
	azm.push_back(azm.front() + 360.0);

	// compute gap
	dGap = 0.0;
	for (int i = 0; i < ndis; i++) {
		double gap = azm[i + 1] - azm[i];
		if (gap > dGap) {
			dGap = gap;
		}
	}
}

// ---------------------------------------------------------summary
void CHypo::summary() {
	// get time string
	glassutil::CDate dt = glassutil::CDate(tOrg);
	std::string sorg = dt.dateTime();

	// compute current stats
	stats();
	char sLog[1024];
	snprintf(sLog, sizeof(sLog),
				"CHypo::summary: %s%7.2f%8.2f%6.1f%6.1f%6.1f%5d (%5.1f) %s",
				sorg.c_str(), dLat, dLon, dZ, dMin, dGap,
				static_cast<int>(vPick.size()), dBayes, sWebName.c_str());
	glassutil::CLogit::log(sLog);
}

// ---------------------------------------------------------list
void CHypo::list(std::string src) {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(hypoMutex);

	if (pGlass == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::list: NULL pGlass.");
		return;
	}
	if (pTTT == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::list: NULL pTTT.");
		return;
	}
	if (vWts.size() == 0) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::list: empty vWts.");
		return;
	}

	char sLog[1024];

	// generate time string
	glassutil::CDate dt = glassutil::CDate(tOrg);
	std::string sorg = dt.dateTime();

	snprintf(sLog, sizeof(sLog),
				"CHypo::list: ** %s **\nPid:%s\n%s %.2f %.2f %.2f (%.2f)",
				sorg.c_str(), sPid.c_str(), sorg.c_str(), dLat, dLon, dZ,
				dBayes);
	glassutil::CLogit::log(sLog);

	// compute current stats
	stats();

	snprintf(
			sLog, sizeof(sLog),
			"CHypo::list: ** %s **\\nPid:%s\nGap:%.1f Dmin:%.1f Dmedian:%.1f"
			"\nSigma:%.2f Kurtosis:%.2f\nWeb:%s",
			sorg.c_str(), sPid.c_str(), dGap, dMin, dMed, dSig, dKrt,
			sWebName.c_str());
	glassutil::CLogit::log(sLog);

	// setup traveltime interface for this hypo
	pTTT->setOrigin(dLat, dLon, dZ);

	// set up a geographic object for this hypo
	glassutil::CGeo geo;
	geo.setGeographic(dLat, dLon, 6371.0 - dZ);

	// create local pick vector
	std::vector<std::shared_ptr<CPick>> vpick;

	// copy picks to the local pick vector
	for (auto pick : vPick) {
		vpick.push_back(pick);
	}

	// generate list of rogue picks
	std::vector<std::shared_ptr<CPick>> pickRogues = pGlass->getPickList()
			->rogues(sPid, tOrg);

	// add rogue picks to the local pick vector
	for (auto pick : pickRogues) {
		vpick.push_back(pick);
	}

	// sort local pick vector
	sort(vpick.begin(), vpick.end(), sortPick);

	// for each pick in the local pick vector
	for (int ipk = 0; ipk < vPick.size(); ipk++) {
		// get the pick
		auto pick = vPick[ipk];

		// get the site
		std::shared_ptr<CSite> site = pick->getSite();

		// compute observed traveltime
		double tobs = pick->getTPick() - tOrg;

		// get expected travel time
		double tcal = pTTT->T(&site->getGeo(), tobs);

		// compute residual
		double tres = tobs - tcal;

		// make sure the residual is reasonable
		if (tres > 20 || tres < -20) {
			// skip if it isn't
			continue;
		}

		// compute distance
		double dis = geo.delta(&site->getGeo()) / DEG2RAD;

		// compute azimuth
		double azm = geo.azimuth(&site->getGeo()) / DEG2RAD;

		// get the association string
		std::string sass = pick->getAss();

		// if the hypo link is valid
		if ((pick->getHypo() != NULL) && (pick->getHypo()->getPid() != sPid)) {
			// set association string
			sass = "*";
		} else {
			// set association string
			sass = "-";
		}
		// if the hypo is fixed, print without a weight
		if (bFixed) {
			snprintf(sLog, sizeof(sLog),
						"CHypo::list: ** %s **\nPid:%s\n%s  %s %s %.2f Dis:%.1f"
						" Azm:%.1f Wt:NA",
						sorg.c_str(), sPid.c_str(), sass.c_str(),
						site->getScnl().c_str(), pTTT->sPhase.c_str(), tres,
						dis, azm);
			glassutil::CLogit::log(sLog);

		} else {
			snprintf(sLog, sizeof(sLog),
						"CHypo::list: ** %s **\nPid:%s\n%s  %s %s %.2f Dis:%.1f"
						" Azm:%.1f Wt:%.2f",
						sorg.c_str(), sPid.c_str(), sass.c_str(),
						site->getScnl().c_str(), pTTT->sPhase.c_str(), tres,
						dis, azm, vWts[ipk]);
			glassutil::CLogit::log(sLog);
		}
	}
}

// ---------------------------------------------------------anneal
double CHypo::anneal(int nIter, double dStart, double dStop, double tStart,
						double tStop) {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(hypoMutex);

	// This is essentially a faster algorithmic implementation of iterate
	// nullcheck
	if (pGlass == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::anneal: NULL pGlass.");
		return (0);
	}

	glassutil::CLogit::log(glassutil::log_level::debug,
							"CHypo::anneal. " + sPid);

	// locate using new function
	if (pGlass->getMinimizeTtLocator() == false) {
		annealingLocate(nIter, dStart, dStop, tStart, tStop, 1);
	} else {
		annealingLocateResidual(nIter, dStart, dStop, tStart, tStop, 1);
	}
	// grid_search();
	// *** Second, based on the new location/depth/time, remove ill fitting
	// picks ***

	// compute current stats
	stats();

	// create pick delete vector
	std::vector<std::shared_ptr<CPick>> vkill;

	// set the traveltime for the current hypo
	if (pTrv1 != NULL) {
		pTrv1->setOrigin(dLat, dLon, dZ);
	}
	if (pTrv2 != NULL) {
		pTrv2->setOrigin(dLat, dLon, dZ);
	}

	// get number of picks
	int npick = vPick.size();

	// for each pick in the pick vector
	for (int ipick = 0; ipick < npick; ipick++) {
		// get the pick
		auto pick = vPick[ipick];

		// calculate the travel times
		double tCal1 = -1;
		if (pTrv1 != NULL) {
			tCal1 = pTrv1->T(&pick->getSite()->getGeo());
		}
		double tCal2 = -1;
		if (pTrv2 != NULL) {
			tCal2 = pTrv2->T(&pick->getSite()->getGeo());
		}

		// calculate absolute residuals
		double tRes1 = 0;
		if (tCal1 > 0) {
			tRes1 = std::abs(pick->getTPick() - tCal1 - tOrg);
		}
		double tRes2 = 0;
		if (tCal2 > 0) {
			tRes2 = std::abs(pick->getTPick() - tCal2 - tOrg);
		}

		// get best (smallest) residual
		double tResBest = 0;
		if ((tRes1 != 0) && (tRes1 < tRes2)) {
			tResBest = tRes1;
		}
		if ((tRes2 != 0) && (tRes1 > tRes2)) {
			tResBest = tRes2;
		} else {
			tResBest = tRes1;
		}

		// make sure the pick residual is within the residual limits
		// NOTE: The residual limit is hard coded
		if (tResBest > 4.0) {
			vkill.push_back(pick);
		}
	}

	// for each pick to remove
	for (auto pick : vkill) {
		// remove the pick hypo link
		pick->clearHypo();

		// remove the pick from this hypo
		remPick(pick);
	}

	// generate weights (for evaluate)
	weights();

	// return the final bayesian value
	return (dBayes);
}

// ---------------------------------------------------------localize
double CHypo::localize() {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(hypoMutex);

	// Localize this hypo
	// nullcheck
	if (pGlass == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::localize: NULL pGlass.");
		return (0);
	}
	char sLog[1024];

	glassutil::CLogit::log(glassutil::log_level::debug,
							"CHypo::localize. " + sPid);

	// if hypo is fixed, just return current bayesian value
	// NOTE: What implication does this have for "seed hypos" like twitter
	// detections
	if (bFixed) {
		return dBayes;
	}

	// get the number of picks
	int npick = vPick.size();

	// based on the number of picks, call relocate
	// if there are already a large number of picks, only do it
	// do often...

	// create taper using the number of picks to define the
	// search distance in localize. Smaller search with more picks
	// the search radius starts at 1/4 the node resolution and
	// shrinks to 1/16 the node radius as picks are added
	// note that the locator can extend past the input search radius
	// these values were chosen by testing specific events

	glassutil::CTaper taper;
	taper = glassutil::CTaper(-0.0001, -0.0001, -0.0001, 30 + 0.0001);
	double searchR = (dRes / 4. + taper.Val(vPick.size()) * .75 * dRes) / 4.;

	// This should be the default
	if (pGlass->getMinimizeTtLocator() == false) {
		if (npick < 50) {
			annealingLocate(5000, searchR, 1., searchR / 30.0, .1);
		} else if (npick < 150 && (npick % 10) == 0) {
			annealingLocate(1250, searchR, 1., searchR / 30.0, .1);
		} else if ((npick % 25) == 0) {
			annealingLocate(500, searchR, 1., searchR / 30.0, .1);
		} else {
			snprintf(sLog, sizeof(sLog),
						"CHypo::localize: Skipping localize with %d picks",
						npick);
			glassutil::CLogit::log(sLog);
		}
	} else {
		if (npick < 25) {
			annealingLocateResidual(10000, searchR, 1., searchR / 10.0, .1);
		} else if (npick < 50 && (npick % 5) == 0) {
			annealingLocateResidual(5000, searchR, 1., searchR / 10.0, .1);
		} else if (npick < 150 && (npick % 10) == 0) {
			annealingLocateResidual(1000, searchR / 2., 1., searchR / 10.0, .1);
		} else if ((npick % 25) == 0) {
			annealingLocateResidual(500, searchR / 2., 1., searchR / 10.0, .1);
		} else {
			snprintf(sLog, sizeof(sLog),
						"CHypo::localize: Skipping localize with %d picks",
						npick);
			glassutil::CLogit::log(sLog);
		}
	}

	// log
	glassutil::CDate dt = glassutil::CDate(tOrg);
	snprintf(sLog, sizeof(sLog),
				"CHypo::localize: HYP %s %s%9.4f%10.4f%6.1f %d", sPid.c_str(),
				dt.dateTime().c_str(), dLat, dLon, dZ,
				static_cast<int>(vPick.size()));
	glassutil::CLogit::log(sLog);

	// return the final maximum bayesian fit
	return (dBayes);
}

// ---------------------------------------------------------annealingLocate
void CHypo::annealingLocate(int nIter, double dStart, double dStop,
							double tStart, double tStop, int nucleate) {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(hypoMutex);

	// don't locate if the location is fixed
	if (bFixed) {
		return;
	}

	// these hold the values of the initial, current, and best stack location
	double valStart = 0;
	double valBest = 0;
	// calculate the value of the stack at the current location
	valStart = getBayes(dLat, dLon, dZ, tOrg, nucleate);

	char sLog[1024];

	// if testing locator, setup output file
	std::ofstream outfile;
	if (pGlass->getTestLocator()) {
		std::string filename = "./locatorTest/" + sPid + ".txt";
		outfile.open(filename, std::ios::out | std::ios::app);
		outfile << std::to_string(dLat) << " " << std::to_string(dLon) << " "
				<< std::to_string(dZ) << " " << std::to_string(tOrg) << " "
				<< std::to_string(vPick.size()) << " "
				<< std::to_string(valStart) << " 0 0 0 \n";
	}

	// set the starting location as the current best stack value
	valBest = valStart;

	// Save total movement
	double ddx = 0.0;
	double ddy = 0.0;
	double ddz = 0.0;
	double ddt = 0.0;

	// create taper using the number of iterations to define the
	// end point. As we iterate through trial locations this makes
	// the search space decrease.
	glassutil::CTaper taper;
	taper = glassutil::CTaper(-0.0001, -0.0001, -0.0001, nIter + 0.0001);

	// for the number of requested iterations
	for (int iter = 0; iter < nIter; iter++) {
		// compute the current step distance from the current iteration and
		// starting and stopping values, use the taper to make the step distance
		// slowly decrease
		double dkm = dStart * taper.Val(static_cast<double>(iter)) + dStop;
		double dOt = tStart * taper.Val(static_cast<double>(iter)) + tStop;

		// init x, y, and z gaussian step distances
		double dx = gauss(0.0, dkm * 2);
		double dy = gauss(0.0, dkm * 2);
		double dz = gauss(0.0, dkm);
		double dt = gauss(0.0, dOt);

		// compute current location using the hypo location and the x and y
		// Gaussian step distances
		double xlon = dLon + cos(DEG2RAD * dLat) * dx / 111.1;
		double xlat = dLat + dy / 111.1;

		// compute current depth using the hypo depth and the z Gaussian step
		// distance
		double xz = dZ + dz;

		// don't let depth go below 1 km
		if (xz < 1.0) {
			xz = 1.0;
		}

		// don't let depth exceed 800 km
		if (xz > 800.0) {
			xz = 800.0;
		}

		// compute current origin time
		double oT = tOrg + dt;

		// get the stack value for this hypocenter
		double val = getBayes(xlat, xlon, xz, oT, nucleate);

		// if testing locator print iteration
		if (pGlass->getTestLocator()) {
			outfile << std::to_string(xlat) << " " << std::to_string(xlon)
					<< " " << std::to_string(xz) << " " << std::to_string(oT)
					<< " " << std::to_string(vPick.size()) << " "
					<< std::to_string(val) << " " << std::to_string(dkm * 2)
					<< " " << std::to_string(dkm) << " " << std::to_string(dOt)
					<< "\n";
		}

		// is this stacked bayesian value better than the previous one?
		if (val > valBest
				|| (val > dThresh
						&& (valBest - val)
								< (pow(gauss(0, .2), 2) / (500. / dkm)))) {
			// then this is the new best value
			valBest = val;
			// set the hypo location/depth/time from the new best
			// locaton/depth/time
			dLat = xlat;
			dLon = xlon;
			dZ = xz;
			tOrg = oT;
			// save this perturbation to the overall change
			ddx += dx;
			ddy += dy;
			ddz += dz;
			ddt += dt;
		}
	}

	// set dBayes to current value
	dBayes = valBest;
	if (nucleate == 1) {
		dBayesInitial = valBest;
	}

	snprintf(
			sLog, sizeof(sLog),
			"CHypo::annealingLocate: total movement (%.4f,%.4f,%.4f,%.4f)"
			" (%.4f,%.4f,%.4f,%.4f) sPid:%s; new bayes value:%.4f; old bayes"
			" value:%.4f",
			dLat, dLon, dZ, tOrg, ddx, ddy, ddz, ddt, sPid.c_str(), valBest,
			valStart);
	glassutil::CLogit::log(sLog);

	if (pGlass->getGraphicsOut() == true) {
		graphicsOutput();
	}

	// if testing the locator close the file
	if (pGlass->getTestLocator()) {
		outfile.close();
	}

	return;
}

// ---------------------------------------------------getBayesStack
double CHypo::getBayes(double xlat, double xlon, double xZ, double oT,
						int nucleate) {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(hypoMutex);

	if ((!pTrv1) && (!pTrv2)) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::getBayes: NULL pTrv1 and pTrv2.");
		return (0);
	}

	if (!pTTT) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::getBayes: NULL pTTT.");
		return (0);
	}

	glassutil::CGeo geo;
	double value = 0.;
	double tcal;
	char sLog[1024];

	// define a taper for sigma, makes close in readings have higher weight
	// ranges from 0.75-3.0 from 0-2 degrees, than 3.0 after that (see loop)
	glassutil::CTaper tap;
	tap = glassutil::CTaper(-0.0001, 2.0, 999.0, 999.0);

	// geo is used for calculating distances to stations for determining sigma
	geo.setGeographic(xlat, xlon, 6371.0 - xZ);

	// This sets the travel-time look up location
	if (pTrv1) {
		pTrv1->setOrigin(xlat, xlon, xZ);
	}
	if (pTrv2) {
		pTrv2->setOrigin(xlat, xlon, xZ);
	}

	pTTT->setOrigin(xlat, xlon, xZ);

	// The number of picks associated with the hypocenter
	int npick = vPick.size();

	// Loop through each pick and find the residual, calculate
	// the significance, and add to the stacks.
	// Currently only P, S, and nucleation phases added to stack.
	for (int ipick = 0; ipick < npick; ipick++) {
		auto pick = vPick[ipick];
		double resi = 99999999;

		// calculate residual
		double tobs = pick->getTPick() - oT;
		std::shared_ptr<CSite> site = pick->getSite();
		glassutil::CGeo siteGeo = site->getGeo();

		// only use nucleation phases if on nucleation branch
		if (nucleate == 1) {
			if ((pTrv1) && (pTrv2)) {
				// we have both nucleation phases
				// first nucleation phase
				// calculate the residual using the phase name
				double tcal1 = pTrv1->T(&siteGeo);
				double resi1 = getResidual(pTrv1->sPhase, tobs, tcal1);

				// second nucleation phase
				// calculate the residual using the phase name
				double tcal2 = pTrv2->T(&siteGeo);
				double resi2 = getResidual(pTrv2->sPhase, tobs, tcal2);

				// use the smallest residual
				if (resi1 < resi2) {
					tcal = tcal1;
					resi = resi1;
				} else {
					tcal = tcal2;
					resi = resi2;
				}
			} else if ((pTrv1) && (!pTrv2)) {
				// we have just the first nucleation phase
				tcal = pTrv1->T(&siteGeo);
				resi = getResidual(pTrv1->sPhase, tobs, tcal);
			} else if ((!pTrv1) && (pTrv2)) {
				// we have just the second ducleation phase
				tcal = pTrv2->T(&siteGeo);
				resi = getResidual(pTrv2->sPhase, tobs, tcal);
			}
		} else {
			// use all available association phases
			// take whichever phase has the smallest residual
			tcal = pTTT->T(&siteGeo, tobs);

			// calculate the residual using the phase name
			resi = getResidual(pTTT->sPhase, tobs, tcal);
		}

		// calculate distance to station to get sigma
		double delta = RAD2DEG * geo.delta(&site->getGeo());
		double sigma = (tap.Val(delta) * 2.25) + 0.75;

		// calculate and add to the stack
		value += pGlass->sig(resi, sigma);
	}
	return value;
}

// ---------------------------------------------------------getResidual
double CHypo::getResidual(std::string sPhase, double tObs, double tCal) {
	if (sPhase == "P") {
		return (tObs - tCal);
	} else if (sPhase == "S") {
		// Effectively halving the weight of S
		// this value was selected by testing specific
		// events with issues
		// NOTE: Hard Coded
		return ((tObs - tCal) * 2.0);
	} else {
		// Down weighting all other phases
		// Value was chosen so that other phases would
		// still contribute (reducing instabilities)
		// but remain insignificant
		// NOTE: Hard Coded
		return ((tObs - tCal) * 10.0);
	}
}

// ---------------------------------------------------------annealingLocate
void CHypo::annealingLocateResidual(int nIter, double dStart, double dStop,
									double tStart, double tStop, int nucleate) {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(hypoMutex);

	if (pTTT == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::annealingLocateResidual: NULL pTTT.");
		return;
	}

	if (bFixed) {
		return;
	}
	char sLog[1024];

	double delta;
	double sigma;

	pTTT->setOrigin(dLat, dLon, dZ);

	double val = 0;
	double valStart = 0;

	valStart = getSumAbsResidual(dLat, dLon, dZ, tOrg, nucleate);
	dBayes = getBayes(dLat, dLon, dZ, tOrg, nucleate);
	snprintf(sLog, sizeof(sLog), "CHypo::annealingLocate: old bayes value %.4f",
				dBayes);
	glassutil::CLogit::log(sLog);

	snprintf(
			sLog, sizeof(sLog),
			"CHypo::annealingLocateResidual: old sum abs residual value %.4f",
			valStart);
	glassutil::CLogit::log(sLog);

	double valBest = valStart;

	// Save total movement
	double ddx = 0.0;
	double ddy = 0.0;
	double ddz = 0.0;
	double ddt = 0.0;

	// create taper using the number of iterations to define the
	// end point
	glassutil::CTaper taper;
	taper = glassutil::CTaper(-0.0001, -0.0001, -0.0001, nIter + 0.0001);

	// for the number of requested iterations
	for (int iter = 0; iter < nIter; iter++) {
		// compute the current step distance from the current iteration and
		// starting and stopping values, use the taper to make the step distance
		// slowly decrease
		double dkm = dStart * taper.Val(static_cast<double>(iter)) + dStop;
		double dOt = tStart * taper.Val(static_cast<double>(iter)) + tStop;

		// init x, y, and z gaussian step distances
		double dx = gauss(0.0, dkm * 2);
		double dy = gauss(0.0, dkm * 2);
		double dz = gauss(0.0, dkm);
		double dt = gauss(0.0, dOt);

		// compute current location using the hypo location and the x and y
		// Gaussian step distances
		double xlon = dLon + cos(DEG2RAD * dLat) * dx / 111.1;
		double xlat = dLat + dy / 111.1;

		// compute current depth using the hypo depth and the z Gaussian step
		// distance
		double xz = dZ + dz;

		// don't let depth go below 1 km
		if (xz < 1.0) {
			xz = 1.0;
		}

		// don't let depth exceed 800 km
		if (xz > 800.0) {
			xz = 800.0;
		}

		// compute current origin time
		double oT = tOrg + dt;

		pTTT->setOrigin(xlat, xlon, xz);
		val = getSumAbsResidual(xlat, xlon, xz, oT, nucleate);
		// geo.setGeographic(dLat, dLon, 6371.0 - dZ);

		// is this stacked bayesian value better than the previous one
		if (val < valBest) {
			// this is the new minimized residual
			valBest = val;
			// set the hypo location/depth/time from the new best
			// locaton/depth/time
			dLat = xlat;
			dLon = xlon;
			dZ = xz;
			tOrg = oT;
			ddx += dx;
			ddy += dy;
			ddz += dz;
			ddt += dt;
		}
	}

	dBayes = getBayes(dLat, dLon, dZ, tOrg, nucleate);
	snprintf(sLog, sizeof(sLog), "CHypo::annealingLocate: old bayes value %.4f",
				dBayes);
	glassutil::CLogit::log(sLog);
	snprintf(sLog, sizeof(sLog),
				"CHypo::annealingLocate: total movement (%.4f,%.4f,%.4f,%.4f)"
				" (%.4f,%.4f,%.4f,%.4f)",
				dLat, dLon, dZ, tOrg, ddx, ddy, ddz, ddt);
	glassutil::CLogit::log(sLog);

	snprintf(sLog, sizeof(sLog),
				"CHypo::annealingLocate: new sum abs residual %.4f", valBest);
	glassutil::CLogit::log(sLog);

	if (pGlass->getGraphicsOut() == true) {
		graphicsOutput();
	}

	return;
}

// ---------------------------------------------------getSumAbsResidual
double CHypo::getSumAbsResidual(double xlat, double xlon, double xZ, double oT,
								int nucleate) {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(hypoMutex);

	if (pTTT == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::getSumAbsResidual: NULL pTTT.");
		return (0);
	}

	double sigma;
	double value = 0.;
	char sLog[1024];

	// This sets the travel-time look up location
	if (pTrv1 != NULL) {
		pTrv1->setOrigin(xlat, xlat, xZ);
	}
	if (pTrv2 != NULL) {
		pTrv2->setOrigin(xlat, xlat, xZ);
	}

	pTTT->setOrigin(xlat, xlon, xZ);

	// The number of picks associated with the hypocenter
	int npick = vPick.size();

	// Loop through each pick and find the residual, calculate
	// the resiudal, and sum.
	// Currently only P, S, and nucleation phases added to stack.
	// If residual is greater than 10, make it 10.
	for (int ipick = 0; ipick < npick; ipick++) {
		auto pick = vPick[ipick];
		double resi = 99999999;

		// calculate residual
		double tobs = pick->getTPick() - oT;
		double tcal;
		std::shared_ptr<CSite> site = pick->getSite();

		// only use nucleation phase if on nucleation branch
		if (nucleate == 1 && pTrv2 == NULL) {
			tcal = pTrv1->T(&site->getGeo());
			resi = tobs - tcal;
		} else if (nucleate == 1 && pTrv1 == NULL) {
			tcal = pTrv2->T(&site->getGeo());
			resi = tobs - tcal;
		} else {
			// take whichever has the smallest residual, P or S
			tcal = pTTT->T(&site->getGeo(), tobs);
			if (pTTT->sPhase == "P" || pTTT->sPhase == "S") {
				resi = tobs - tcal;
			}
		}
		resi = std::abs(resi);
		if (resi > 10.) {
			resi = 10.;
		}
		value += resi;
	}
	return value;
}

// ---------------------------------------------------graphicsOutput
void CHypo::graphicsOutput() {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(hypoMutex);

	if (pTTT == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::graphicsOutput: NULL pTTT.");
		return;
	}

	std::ofstream outfile;
	std::string filename = pGlass->getGraphicsOutFolder() + sPid + ".txt";
	outfile.open(filename, std::ios::out);
	outfile << "hypocenter: " << std::to_string(dLat) << " "
			<< std::to_string(dLon) << " " << std::to_string(dZ) << " "
			<< std::to_string(tOrg) << "\n";
	double stack = 0;
	double tcal = 0;
	double delta = 0;
	double sigma = 0;
	glassutil::CGeo geo;
	int npick = vPick.size();
	for (int y = -1 * pGlass->getGraphicsSteps();
			y <= pGlass->getGraphicsSteps(); y++) {
		double xlat = dLat + (y * pGlass->getGraphicsStepKm()) / 111.1;
		for (int x = -1 * pGlass->getGraphicsSteps();
				x <= pGlass->getGraphicsSteps(); x++) {
			double xlon = dLon
					+ cos(DEG2RAD * xlat) * (x * pGlass->getGraphicsStepKm())
							/ 111.1;
			pTTT->setOrigin(xlat, xlon, dZ);
			stack = 0;
			for (int ipick = 0; ipick < npick; ipick++) {
				auto pick = vPick[ipick];
				double tobs = pick->getTPick() - tOrg;
				std::shared_ptr<CSite> site = pick->getSite();
				tcal = pTTT->T(&site->getGeo(), tobs);
				delta = RAD2DEG * geo.delta(&site->getGeo());

				// This should be a function.
				if (delta < 1.5) {
					sigma = .75;
				} else if ((delta >= 1.5) && (delta < 30.0)) {
					sigma = 1.5;
				} else {
					sigma = 3;
				}
				stack += pGlass->sig_laplace_pdf(tobs - tcal, sigma);
				outfile << std::to_string(xlat) << " " << std::to_string(xlon)
						<< " " << std::to_string(stack) << "\n";
			}
		}
	}

	outfile.close();
}

// ---------------------------------------------------------trap
void CHypo::trap() {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(hypoMutex);

	char sLog[1024];

	// for each pick in this hypocenter
	for (const auto &q : vPick) {
		if (q == NULL) {
			continue;
		}

		// get the pick's hypo pointer
		std::shared_ptr<CHypo> hyp = q->getHypo();

		// check pointer
		if (hyp == NULL) {
			// bad hypo pointer
			snprintf(sLog, sizeof(sLog),
						"CHypo::trap: sPid %s Pick %s has no back link to hypo",
						sPid.c_str(), q->getPid().c_str());
			glassutil::CLogit::log(glassutil::log_level::warn, sLog);

			continue;
		}

		// check sPid
		if (hyp->getPid() != sPid) {
			// sPid is for a different hypo
			snprintf(
					sLog, sizeof(sLog),
					"CHypo::trap: sPid %s Pick: %s linked to another hypo: %s",
					sPid.c_str(), q->getPid().c_str(), hyp->getPid().c_str());
			glassutil::CLogit::log(glassutil::log_level::warn, sLog);
		}
	}
}

// ---------------------------------------------------------weights
bool CHypo::weights() {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(hypoMutex);

	// Calculate station weight from distance and nearby stations
	// to reduce biases induced by network density
	// nullchecks
	if (pGlass == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::weights: NULL pGlass.");
		return (false);
	}

	// get average delta and sigma from glass
	double avgDelta = pGlass->getAvgDelta();
	double avgSigma = pGlass->getAvgSigma();

	// create taper
	glassutil::CTaper tap;
	tap = glassutil::CTaper(0.0, avgDelta, 180.0, 360.0);

	// get number of picks
	int npick = vPick.size();

	// check to see if we have enough data to continue
	if (npick < 1) {
		return (false);
	}

	// setup vWts for newdata
	vWts.clear();
	nWts = 0;

	// for each pick in this hypo
	for (int ipick = 0; ipick < npick; ipick++) {
		// get the pick
		auto pick = vPick[ipick];

		// get the site
		std::shared_ptr<CSite> site = pick->getSite();

		// set up a geo object for this hypo
		glassutil::CGeo geo;
		geo.setGeographic(dLat, dLon, 6371.0 - dZ);

		// get distance between this pick and the hypo
		double del = RAD2DEG * geo.delta(&site->getGeo());

		// compute sigma for this pick-hypo distance from the average sigma and
		// the distance to this site using the taper
		double sig = avgSigma * tap.Val(del);

		// compute the overall sum by using the distance to each other
		// station
		double sum = 0.0;
		for (int j = 0; j < npick; j++) {
			// get the pick
			auto pickj = vPick[j];

			// get the site
			std::shared_ptr<CSite> sitej = pickj->getSite();

			// get the distance between this pick and the pick being weighted
			double delj = RAD2DEG * site->getDelta(&sitej->getGeo());

			// if the distance is within 6 sig
			if (delj < (6.0 * sig)) {
				// add the significance function for this distance and sigma
				// to the sum
				sum += pGlass->sig(delj, sig);
			}
		}

		// add the inverse of the sum to vector as the weight
		vWts.push_back(1.0 / sum);
		nWts++;
	}

	// success
	return (true);
}

bool CHypo::resolve(std::shared_ptr<CHypo> hyp) {
	// lock the hypo since we're iterating through it's lists
	std::lock_guard<std::recursive_mutex> hypoGuard(hypoMutex);

	// nullchecks
	if (pGlass == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::resolve: NULL pGlass.");
		return (false);
	}
	if (pGlass->getHypoList() == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::resolve: NULL pGlass pHypoList.");
		return (false);
	}

	glassutil::CLogit::log(glassutil::log_level::debug,
							"CHypo::resolve. " + sPid);

	bool bAss = false;
	char sLog[1024];

	// handle picks
	// for each pick in this hypo
	int nPck = vPick.size();

	int addCount = 0;
	int removeCount = 0;

	// NOTE: Why are we moving backwards through the list?
	for (int iPck = nPck - 1; iPck >= 0; iPck--) {
		// get the pick
		std::shared_ptr<CPick> pck = vPick[iPck];

		// get the pick's hypo pointer
		std::shared_ptr<CHypo> pickHyp = pck->getHypo();

		// if this pick isn't linked to a hypo
		if (pickHyp == NULL) {
			// link to this hypo and move on
			pck->addHypo(hyp);

			continue;
		}

		std::string sOtherPid = pickHyp->getPid();

		// if this pick is linked to this hypo
		if (sOtherPid == sPid) {
			// nothing else to do
			continue;
		}

		// get the current pick's affinity to the provided hypo
		double aff1 = affinity(pck);

		// get the current pick's affinity to the hypo it's linked to
		double aff2 = pickHyp->affinity(pck);

		snprintf(sLog, sizeof(sLog),
					"CHypo::resolve: SCV COMPARE %s %s %s %s (%.2f, %.2f)",
					sPid.c_str(), sOtherPid.c_str(),
					glassutil::CDate::encodeDateTime(pck->getTPick()).c_str(),
					pck->getSite()->getScnl().c_str(), aff1, aff2);
		glassutil::CLogit::log(sLog);

		// check which affinity is better
		if (aff1 > aff2) {
			// this pick has a higher affinity with the provided hypo
			/*
			 snprintf(
			 sLog,
			 sizeof(sLog),
			 "CHypo::resolve: SCV %s %s %s %s (%.2f)",
			 sPid.c_str(),
			 sOtherPid.c_str(),
			 glassutil::CDate::encodeDateTime(pck->getTPick()).c_str(),
			 pck->getSite()->getScnl().c_str(), aff1);
			 glassutil::CLogit::log(sLog);
			 */

			// remove the pick from it's original hypo
			pickHyp->remPick(pck);

			// link pick to the provided hypo
			pck->addHypo(hyp, "S", true);

			// add provided hypo to the processing queue
			// NOTE: this puts provided hypo before original hypo in FIFO,
			// we want this hypo to keep this pick, rather than the original
			// just stealing it back, which is why we do it here
			// NOTE: why add it at all? we're gonna locate before we finish
			// is it to see if we can get more next time
			pGlass->getHypoList()->pushFifo(hyp);

			// add the original hypo the pick was linked to the processing queue
			pGlass->getHypoList()->pushFifo(pickHyp);

			// we've made a change to the hypo (grabbed a pick)
			bAss = true;
			addCount++;
		} else {
			// this pick has higher affinity with the original hypo
			// remove pick from provided hypo
			remPick(pck);

			// we've made a change to the hypo (got rid of a pick)
			bAss = true;
			removeCount++;
		}
	}

	glassutil::CLogit::log(
			glassutil::log_level::debug,
			"CHypo::resolve " + sPid + " added:" + std::to_string(addCount)
					+ " removed:" + std::to_string(removeCount));

	// handle correlations
	// for each correlation in this hypo
	int nCorr = vCorr.size();

	// NOTE: Why are we moving backwards through the list?
	for (int iCorr = nCorr - 1; iCorr >= 0; iCorr--) {
		// get the correlation
		auto corr = vCorr[iCorr];

		// get the correlation's hypo pointer
		std::shared_ptr<CHypo> corrHyp = corr->getHypo();

		// if this correlation isn't linked to a hypo
		if (corrHyp == NULL) {
			// link to this hypo and move on
			corr->addHypo(hyp);
			continue;
		}

		std::string sOtherPid = corrHyp->getPid();

		// if this corr is linked to this hypo
		if (sOtherPid == sPid) {
			// nothing else to do
			continue;
		}

		// get the current correlation's affinity to the provided hypo
		double aff1 = affinity(corr);
		double aff2 = corrHyp->affinity(corr);

		snprintf(
				sLog,
				sizeof(sLog),
				"CHypo::resolve: C SCV COMPARE %s %s %s %s (%.2f, %.2f )",
				sPid.c_str(),
				sOtherPid.c_str(),
				glassutil::CDate::encodeDateTime(corr->getTCorrelation()).c_str(),
				corr->getSite()->getScnl().c_str(), aff1, aff2);
		glassutil::CLogit::log(sLog);

		// check which affinity is better
		if (aff1 > aff2) {
			// this pick has a higher affinity with the provided hypo
			/*
			 snprintf(
			 sLog,
			 sizeof(sLog),
			 "CHypo::resolve: C SCV %s %s %s %s (%.2f)\n",
			 sPid.c_str(),
			 sOtherPid.c_str(),
			 glassutil::CDate::encodeDateTime(
			 corr->getTCorrelation()).c_str(),
			 corr->getSite()->getScnl().c_str(), aff1);
			 glassutil::CLogit::log(sLog);
			 */

			// remove the correlation from it's original hypo
			corrHyp->remCorrelation(corr);

			// link pick to the provided hypo
			corr->addHypo(hyp, "C-S", true);

			// add provided hypo to the processing queue
			// NOTE: this puts provided hypo before original hypo in FIFO,
			// we want this hypo to keep this pick, rather than the original
			// just stealing it back, which is why we do it here
			pGlass->getHypoList()->pushFifo(hyp);

			// add the original hypo the pick was linked to the processing queue
			pGlass->getHypoList()->pushFifo(corrHyp);

			// we've made a change to the hypo (grabbed a pick)
			bAss = true;
		} else {
			// this pick has higher affinity with the original hypo
			// remove pick from provided hypo
			remCorrelation(corr);

			// we've made a change to the hypo (got rid of a pick)
			bAss = true;
		}
	}

	return (bAss);
}

double CHypo::getLat() const {
	std::lock_guard<std::recursive_mutex> hypoGuard(hypoMutex);
	return (dLat);
}

double CHypo::getLon() const {
	std::lock_guard<std::recursive_mutex> hypoGuard(hypoMutex);
	return (dLon);
}

double CHypo::getZ() const {
	std::lock_guard<std::recursive_mutex> hypoGuard(hypoMutex);
	return (dZ);
}

double CHypo::getTOrg() const {
	std::lock_guard<std::recursive_mutex> hypoGuard(hypoMutex);
	return (tOrg);
}

double CHypo::getBayes() const {
	std::lock_guard<std::recursive_mutex> hypoGuard(hypoMutex);
	return (dBayes);
}

bool CHypo::getCorrAdded() const {
	std::lock_guard<std::recursive_mutex> hypoGuard(hypoMutex);
	return (bCorrAdded);
}

void CHypo::setCorrAdded(bool corrAdded) {
	std::lock_guard<std::recursive_mutex> hypoGuard(hypoMutex);
	bCorrAdded = corrAdded;
}

bool CHypo::getEvent() const {
	std::lock_guard<std::recursive_mutex> hypoGuard(hypoMutex);
	return (bEvent);
}

bool CHypo::getFixed() const {
	std::lock_guard<std::recursive_mutex> hypoGuard(hypoMutex);
	return (bFixed);
}

void CHypo::setFixed(bool fixed) {
	std::lock_guard<std::recursive_mutex> hypoGuard(hypoMutex);
	bFixed = fixed;
}

double CHypo::getBayesInitial() const {
	std::lock_guard<std::recursive_mutex> hypoGuard(hypoMutex);
	return (dBayesInitial);
}

double CHypo::getCutFactor() const {
	std::lock_guard<std::recursive_mutex> hypoGuard(hypoMutex);
	return (dCutFactor);
}

void CHypo::setCutFactor(double cutFactor) {
	std::lock_guard<std::recursive_mutex> hypoGuard(hypoMutex);
	dCutFactor = cutFactor;
}

double CHypo::getCutMin() const {
	std::lock_guard<std::recursive_mutex> hypoGuard(hypoMutex);
	return (dCutMin);
}

void CHypo::setCutMin(double cutMin) {
	std::lock_guard<std::recursive_mutex> hypoGuard(hypoMutex);
	dCutMin = cutMin;
}

double CHypo::getCutPercentage() const {
	std::lock_guard<std::recursive_mutex> hypoGuard(hypoMutex);
	return (dCutPercentage);
}

void CHypo::setCutPercentage(double cutPercentage) {
	std::lock_guard<std::recursive_mutex> hypoGuard(hypoMutex);
	dCutPercentage = cutPercentage;
}

double CHypo::getGap() const {
	std::lock_guard<std::recursive_mutex> hypoGuard(hypoMutex);
	return (dGap);
}

double CHypo::getKrt() const {
	std::lock_guard<std::recursive_mutex> hypoGuard(hypoMutex);
	return (dKrt);
}

double CHypo::getMed() const {
	std::lock_guard<std::recursive_mutex> hypoGuard(hypoMutex);
	return (dMed);
}

double CHypo::getMin() const {
	std::lock_guard<std::recursive_mutex> hypoGuard(hypoMutex);
	return (dMin);
}

double CHypo::getRes() const {
	std::lock_guard<std::recursive_mutex> hypoGuard(hypoMutex);
	return (dRes);
}

double CHypo::getSig() const {
	std::lock_guard<std::recursive_mutex> hypoGuard(hypoMutex);
	return (dSig);
}

double CHypo::getThresh() const {
	std::lock_guard<std::recursive_mutex> hypoGuard(hypoMutex);
	return (dThresh);
}

void CHypo::setThresh(double thresh) {
	std::lock_guard<std::recursive_mutex> hypoGuard(hypoMutex);
	dThresh = thresh;
}

int CHypo::getCycle() const {
	std::lock_guard<std::recursive_mutex> hypoGuard(hypoMutex);
	return (iCycle);
}

int CHypo::setCycle(int newCycle) {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(hypoMutex);

	iCycle = newCycle;

	return (iCycle);
}

int CHypo::getCut() const {
	std::lock_guard<std::recursive_mutex> hypoGuard(hypoMutex);
	return (nCut);
}

void CHypo::setCut(double cut) {
	std::lock_guard<std::recursive_mutex> hypoGuard(hypoMutex);
	nCut = cut;
}

const CGlass* CHypo::getGlass() const {
	std::lock_guard<std::recursive_mutex> hypoGuard(hypoMutex);
	return (pGlass);
}

void CHypo::setGlass(CGlass* glass) {
	std::lock_guard<std::recursive_mutex> hypoGuard(hypoMutex);
	pGlass = glass;
}

int CHypo::getProcessCount() const {
	std::lock_guard<std::recursive_mutex> guard(hypoMutex);
	return (processCount);
}

int CHypo::incrementProcessCount() {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(hypoMutex);

	processCount++;

	return (processCount);
}

const std::string& CHypo::getPid() const {
	return (sPid);
}

const std::string& CHypo::getWebName() const {
	return (sWebName);
}

int CHypo::getVPickSize() const {
	std::lock_guard<std::recursive_mutex> hypoGuard(hypoMutex);
	return (vPick.size());
}
int CHypo::getVCorrSize() const {
	std::lock_guard<std::recursive_mutex> hypoGuard(hypoMutex);
	return (vCorr.size());
}

double CHypo::getTCreate() const {
	return (tCreate);
}

int CHypo::getReportCount() const {
	std::lock_guard<std::recursive_mutex> hypoGuard(hypoMutex);
	return (reportCount);
}

bool CHypo::isLockedForProcessing() {
	if (processingMutex.try_lock() == false) {
		return (true);
	}

	processingMutex.unlock();
	return (false);
}

void CHypo::lockForProcessing() {
	processingMutex.lock();
}

void CHypo::unlockAfterProcessing() {
	processingMutex.unlock();
}

}  // namespace glasscore

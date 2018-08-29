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
#include <limits>

namespace glasscore {

#define MAXLOCDEPTH 800.0  // The maximum allowed locator depth

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
				std::shared_ptr<traveltime::CTTT> ttt, double resolution,
				double aziTap, double maxDep) {
	// seed the random number generator
	std::random_device randomDevice;
	m_RandomGenerator.seed(randomDevice());

	if (!initialize(lat, lon, z, time, pid, web, bayes, thresh, cut, firstTrav,
					secondTrav, ttt, resolution, aziTap, maxDep)) {
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
								"CHypo::CHypo: NULL trigger->getWeb().");

		clear();
		return;
	}

	if (!initialize(trigger->getLatitude(), trigger->getLongitude(),
					trigger->getDepth(), trigger->getTOrigin(),
					glassutil::CPid::pid(), trigger->getWeb()->getName(),
					trigger->getBayesValue(),
					trigger->getWeb()->getNucleationStackThreshold(),
					trigger->getWeb()->getNucleationDataThreshold(),
					trigger->getWeb()->getNucleationTravelTime1(),
					trigger->getWeb()->getNucleationTravelTime2(), ttt,
					trigger->getResolution(),
					trigger->getWeb()->getAzimuthTaper(),
					trigger->getWeb()->getMaxDepth())) {
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
	m_pTravelTimeTables = NULL;
	m_pNucleationTravelTime1 = NULL;
	m_pNucleationTravelTime2 = NULL;

	// null checks
	if (corr == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::CHypo: NULL correlation.");

		clear();
		return;
	}

	if (!initialize(corr->getLatitude(), corr->getLongitude(), corr->getDepth(),
					corr->getTOrigin(), glassutil::CPid::pid(), "Correlation",
					0.0, 0.0, 0.0, firstTrav, secondTrav, ttt, 0.0)) {
		clear();
	}
}

// ---------------------------------------------------------~CHypo
CHypo::~CHypo() {
	clear();
}

// ---------------------------------------------------------addCorrelation
void CHypo::addCorrelation(std::shared_ptr<CCorrelation> corr) {
	// null check
	if (corr == NULL) {
		glassutil::CLogit::log(glassutil::log_level::warn,
								"CHypo::addCorrelation: NULL correlation.");
		return;
	}

	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(m_HypoMutex);

	// for each correlation in the vector
	for (auto q : m_vCorrelationData) {
		// see if we have this same correlation
		// NOTE: this only checks by ID, need to improve this to
		// an ID/Source check
		if (q->getID() == corr->getID()) {
			char sLog[1024];
			snprintf(sLog, sizeof(sLog),
						"CHypo::addCorrelation: ** Duplicate correlation %s",
						corr->getSite()->getSCNL().c_str());
			glassutil::CLogit::log(sLog);

			return;
		}
	}

	// add the pick to the vector.
	m_vCorrelationData.push_back(corr);
	m_bCorrelationAdded = true;
}

// ---------------------------------------------------------addPick
void CHypo::addPick(std::shared_ptr<CPick> pck) {
	// null check
	if (pck == NULL) {
		glassutil::CLogit::log(glassutil::log_level::warn,
								"CHypo::addPick: NULL pck.");
		return;
	}

	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(m_HypoMutex);

	// for each pick in the vector
	for (auto q : m_vPickData) {
		// see if we have this same pick
		if (q->getID() == pck->getID()) {
			// Don't add this duplicate pick
			return;
		}
	}

	// add the pick to the vector.
	m_vPickData.push_back(pck);
}

// ---------------------------------------------------------affinity
double CHypo::affinity(std::shared_ptr<CPick> pck) {
	// null checks
	if (pck == NULL) {
		return (0.0);
	}
	if (m_pGlass == NULL) {
		return (0.0);
	}

	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(m_HypoMutex);

	// get the site from the pick
	std::shared_ptr<CSite> site = pck->getSite();

	// get various global parameters from the glass pointer
	// get the standard deviation allowed for association
	double sdassoc = m_pGlass->getAssociationSDCutoff();

	// get the affinity factor
	double expaff = m_pGlass->getPickAffinityExpFactor();

	// check to see if this pick can  associate with this hypo using
	// the given association standard deviation
	if (!associate(pck, 1.0, sdassoc)) {
		// the pick did not associate
		return (0.0);
	}

	// get pick residual
	double tRes = getResidual(pck);

	// get absolute residual
	if (tRes < 0.0) {
		tRes = -tRes;
	}

	// WLY - I removed this because it was leading to small
	// events trading picks and canceling....
	// stack accounts for residual anyways
	// compute a weight factor based on residual
	// weight goes from 0.25 at a residual of sdassoc to 1.
	// glassutil::CTaper resWeight;
	// resWeight = glassutil::CTaper(-1., -1., sdassoc/2., sdassoc);
	// double resWeightFactor = (resWeight.Val(tRes) * 0.75) + 0.25;

	// now compute the gap factor using a taper
	glassutil::CTaper gap;
	gap = glassutil::CTaper(0.0, 0.0, 270.0, 360.0);
	double gapfac = gap.Val(m_dGap);

	// compute the affinity of this pick to this hypo by multiplying
	// the gap factor to the hypocenter's current baysian statistic to
	// the affinity power factor
	double aff = gapfac * pow(m_dBayesValue, expaff);

	// return the affinity
	return (aff);
}

// ---------------------------------------------------------affinity
double CHypo::affinity(std::shared_ptr<CCorrelation> corr) {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(m_HypoMutex);

	// NOTE: I'm just combining time/distance into a made up affinity
	// wiser heads than mine may come up with a more robust approach JMP
	// null checks
	if (corr == NULL) {
		return (0.0);
	}
	if (m_pGlass == NULL) {
		return (0.0);
	}

	// get various global parameters from the glass pointer
	double tWindow = m_pGlass->getCorrelationMatchingTimeWindow();
	double xWindow = m_pGlass->getCorrelationMatchingDistanceWindow();

	// check to see if this correlation can associate with this hypo
	if (!associate(corr, tWindow, xWindow)) {
		// the correlation did not associate
		return (0.0);
	}

	// compute time factor, multiply by 10 to make time factor
	// have equal weight to the distance factor
	double tFactor = std::abs(m_tOrigin - corr->getTCorrelation()) * 10;

	// hypo is in geographic coordinates
	glassutil::CGeo geo1;
	geo1.setGeographic(m_dLatitude, m_dLongitude, EARTHRADIUSKM - m_dDepth);

	// correlation is in geographic coordinates
	glassutil::CGeo geo2;
	geo2.setGeographic(corr->getLatitude(), corr->getLongitude(),
	EARTHRADIUSKM - corr->getDepth());

	// compute distance factor
	double xFactor = RAD2DEG * geo1.delta(&geo2);

	// compute the affinity of this correlation to this hypo by taking the
	// inverse of multiplying the time factor to the distance filter
	// note the idea is a higher number the closer the correlation is
	// to the hypo
	double aff = 1 / (tFactor * xFactor);

	// return the affinity
	return (aff);
}

// ---------------------------------------------------------anneal
double CHypo::anneal(int nIter, double dStart, double dStop, double tStart,
						double tStop) {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(m_HypoMutex);

	// This is essentially a faster algorithmic implementation of iterate
	// nullcheck
	if (m_pGlass == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::anneal: NULL pGlass.");
		return (0);
	}

	glassutil::CLogit::log(glassutil::log_level::debug,
							"CHypo::anneal. " + m_sID);

	// locate using new function
	if (m_pGlass->getMinimizeTTLocator() == false) {
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
	if (m_pNucleationTravelTime1 != NULL) {
		m_pNucleationTravelTime1->setOrigin(m_dLatitude, m_dLongitude,
											m_dDepth);
	}
	if (m_pNucleationTravelTime2 != NULL) {
		m_pNucleationTravelTime2->setOrigin(m_dLatitude, m_dLongitude,
											m_dDepth);
	}

	// get number of picks
	int npick = m_vPickData.size();

	// for each pick in the pick vector
	for (int ipick = 0; ipick < npick; ipick++) {
		// get the pick
		auto pick = m_vPickData[ipick];

		// calculate the travel times
		double tCal1 = -1;
		if (m_pNucleationTravelTime1 != NULL) {
			tCal1 = m_pNucleationTravelTime1->T(&pick->getSite()->getGeo());
		}
		double tCal2 = -1;
		if (m_pNucleationTravelTime2 != NULL) {
			tCal2 = m_pNucleationTravelTime2->T(&pick->getSite()->getGeo());
		}

		// calculate absolute residuals
		double tRes1 = 0;
		if (tCal1 > 0) {
			tRes1 = std::abs(pick->getTPick() - tCal1 - m_tOrigin);
		}
		double tRes2 = 0;
		if (tCal2 > 0) {
			tRes2 = std::abs(pick->getTPick() - tCal2 - m_tOrigin);
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
		if (tResBest > m_pGlass->getAssociationSDCutoff()) {
			vkill.push_back(pick);
		}
	}

	// for each pick to remove
	for (auto pick : vkill) {
		// remove the pick hypo link
		std::shared_ptr<CHypo> pickHyp = pick->getHypo();
		if (pickHyp != NULL) {
			if (pickHyp->getID() == getID()) {
				pick->clearHypo();
			}
		}

		// remove the pick from this hypo
		removePick(pick);
	}

	// return the final bayesian value
	return (m_dBayesValue);
}

// ---------------------------------------------------------annealingLocate
void CHypo::annealingLocate(int nIter, double dStart, double dStop,
							double tStart, double tStop, int nucleate) {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(m_HypoMutex);

	// don't locate if the location is fixed
	if (m_bFixed) {
		return;
	}

	// taper to lower val if large azimuthal gap
	glassutil::CTaper taperGap;
	taperGap = glassutil::CTaper(0., 0., m_dAzimuthTaper, 360.);

	// these hold the values of the initial, current, and best stack location
	double valStart = 0;
	double valBest = 0;
	// calculate the value of the stack at the current location
	valStart = getBayes(m_dLatitude, m_dLongitude, m_dDepth, m_tOrigin,
						nucleate)
			* taperGap.Val(gap(m_dLatitude, m_dLongitude, m_dDepth));

	char sLog[1024];

	// if testing locator, setup output file
	std::ofstream outfile;
	if (m_pGlass->getTestLocator()) {
		std::string filename = "./locatorTest/" + m_sID + ".txt";
		outfile.open(filename, std::ios::out | std::ios::app);
		outfile << std::to_string(m_dLatitude) << " "
				<< std::to_string(m_dLongitude) << " "
				<< std::to_string(m_dDepth) << " " << std::to_string(m_tOrigin)
				<< " " << std::to_string(m_vPickData.size()) << " "
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
		double xlon = m_dLongitude + cos(DEG2RAD * m_dLatitude) * dx / DEG2KM;
		double xlat = m_dLatitude + dy / DEG2KM;

		// compute current depth using the hypo depth and the z Gaussian step
		// distance
		double xz = m_dDepth + dz;

		// don't let depth go below 1 km
		if (xz < 1.0) {
			xz = 1.0;
		}

		// don't let depth exceed maximum
		if (xz > m_dMaxDepth) {
			xz = m_dDepth;
		}

		// compute current origin time
		double oT = m_tOrigin + dt;

		// get the stack value for this hypocenter
		double val = getBayes(xlat, xlon, xz, oT, nucleate)
				* taperGap.Val(gap(xlat, xlon, xz));

		// if testing locator print iteration
		if (m_pGlass->getTestLocator()) {
			outfile << std::to_string(xlat) << " " << std::to_string(xlon)
					<< " " << std::to_string(xz) << " " << std::to_string(oT)
					<< " " << std::to_string(m_vPickData.size()) << " "
					<< std::to_string(val) << " " << std::to_string(dkm * 2)
					<< " " << std::to_string(dkm) << " " << std::to_string(dOt)
					<< "\n";
		}

		// is this stacked bayesian value (val) better than the previous best
		// (valBest)

		if (val > valBest
				|| (val > m_dNucleationStackThreshold
						&& (valBest - val)
								< (pow(gauss(0, .2), 2) / (500. / dkm)))) {
			// then this is the new best value
			valBest = val;
			// set the hypo location/depth/time from the new best
			// locaton/depth/time
			setLatitude(xlat);
			setLongitude(xlon);
			setDepth(xz);
			setTOrigin(oT);

			// save this perturbation to the overall change
			ddx += dx;
			ddy += dy;
			ddz += dz;
			ddt += dt;
		}
	}

	// set dBayes to current value
	m_dBayesValue = valBest;
	if (nucleate == 1) {
		m_dInitialBayesValue = valBest;
	}

	snprintf(
			sLog, sizeof(sLog),
			"CHypo::annealingLocate: total movement (%.4f,%.4f,%.4f,%.4f)"
			" (%.4f,%.4f,%.4f,%.4f) sPid:%s; new bayes value:%.4f; old bayes"
			" value:%.4f",
			getLatitude(), getLongitude(), getDepth(), getTOrigin(), ddx, ddy,
			ddz, ddt, m_sID.c_str(), valBest, valStart);
	glassutil::CLogit::log(sLog);

	if (m_pGlass->getGraphicsOut() == true) {
		graphicsOutput();
	}

	// if testing the locator close the file
	if (m_pGlass->getTestLocator()) {
		outfile.close();
	}

	return;
}

// ---------------------------------------------------------annealingLocate
void CHypo::annealingLocateResidual(int nIter, double dStart, double dStop,
									double tStart, double tStop, int nucleate) {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(m_HypoMutex);

	if (m_pTravelTimeTables == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::annealingLocateResidual: NULL pTTT.");
		return;
	}

	if (m_bFixed) {
		return;
	}
	char sLog[1024];

	double delta;
	double sigma;

	m_pTravelTimeTables->setOrigin(m_dLatitude, m_dLongitude, m_dDepth);

	double val = 0;
	double valStart = 0;

	valStart = getSumAbsResidual(m_dLatitude, m_dLongitude, m_dDepth, m_tOrigin,
									nucleate);
	m_dBayesValue = getBayes(m_dLatitude, m_dLongitude, m_dDepth, m_tOrigin,
								nucleate);
	snprintf(sLog, sizeof(sLog), "CHypo::annealingLocate: old bayes value %.4f",
				getBayesValue());
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
		double xlon = m_dLongitude + cos(DEG2RAD * m_dLatitude) * dx / DEG2KM;
		double xlat = m_dLatitude + dy / DEG2KM;

		// compute current depth using the hypo depth and the z Gaussian step
		// distance
		double xz = m_dDepth + dz;

		// don't let depth go below 1 km
		if (xz < 1.0) {
			xz = 1.0;
		}

		// don't let depth exceed maximum
		if (xz > m_dMaxDepth) {
			xz = m_dDepth;
		}

		// compute current origin time
		double oT = m_tOrigin + dt;

		m_pTravelTimeTables->setOrigin(xlat, xlon, xz);
		val = getSumAbsResidual(xlat, xlon, xz, oT, nucleate);
		// geo.setGeographic(dLat, dLon, EARTHRADIUSKM - dZ);

		// is this stacked bayesian value better than the previous one
		if (val < valBest) {
			// this is the new minimized residual
			valBest = val;
			// set the hypo location/depth/time from the new best
			// locaton/depth/time
			setLatitude(xlat);
			setLongitude(xlon);
			setDepth(xz);
			setTOrigin(oT);
			ddx += dx;
			ddy += dy;
			ddz += dz;
			ddt += dt;
		}
	}

	m_dBayesValue = getBayes(m_dLatitude, m_dLongitude, m_dDepth, m_tOrigin,
								nucleate);
	snprintf(sLog, sizeof(sLog), "CHypo::annealingLocate: old bayes value %.4f",
				getBayesValue());
	glassutil::CLogit::log(sLog);
	snprintf(sLog, sizeof(sLog),
				"CHypo::annealingLocate: total movement (%.4f,%.4f,%.4f,%.4f)"
				" (%.4f,%.4f,%.4f,%.4f)",
				getLatitude(), getLongitude(), getDepth(), getTOrigin(), ddx,
				ddy, ddz, ddt);
	glassutil::CLogit::log(sLog);

	snprintf(sLog, sizeof(sLog),
				"CHypo::annealingLocate: new sum abs residual %.4f", valBest);
	glassutil::CLogit::log(sLog);

	if (m_pGlass->getGraphicsOut() == true) {
		graphicsOutput();
	}

	return;
}

// ---------------------------------------------------------associate
bool CHypo::associate(std::shared_ptr<CPick> pick, double sigma,
						double sdassoc) {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(m_HypoMutex);

	// null check
	if (pick == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::associate: NULL pick.");
		return (false);
	}
	if (m_pGlass == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::associate: NULL pGlass.");
		return (false);
	}
	if (m_pTravelTimeTables == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::associate: NULL pTTT.");
		return (false);
	}

	double dAzimuthRange = m_pGlass->getBeamMatchingAzimuthWindow();
	// double dDistanceRange = pGlass->getBeamMatchingDistanceWindow();

	// get site
	std::shared_ptr<CSite> site = pick->getSite();

	// set up a geographic object for this hypo
	glassutil::CGeo hypoGeo;
	hypoGeo.setGeographic(m_dLatitude, m_dLongitude, EARTHRADIUSKM - m_dDepth);

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

	// compute distance
	double siteDistance = hypoGeo.delta(&site->getGeo()) / DEG2RAD;

	// check if distance is beyond cutoff
	if (siteDistance > m_dDistanceCutoff) {
		// it is, don't associated
		return (false);
	}

	double tRes = getResidual(pick);

	if (std::isnan(tRes) == true) {
		return (false);
	}

	// compute absolute residual
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
	std::lock_guard<std::recursive_mutex> guard(m_HypoMutex);

	// NOTE: this is a simple time/distance check for association
	// wiser heads than mine may come up with a more robust approach JMP
	// null check
	if (corr == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::associate: NULL correlation.");
		return (false);
	}
	if (m_pGlass == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::associate: NULL pGlass.");
		return (false);
	}

	char sLog[1024];

	double tDist = 0;
	double xDist = 0;

	// check if time difference is within window
	tDist = std::abs(m_tOrigin - corr->getTCorrelation());
	if (tDist < tWindow) {
		glassutil::CGeo geo1;
		geo1.setGeographic(m_dLatitude, m_dLongitude, EARTHRADIUSKM - m_dDepth);
		glassutil::CGeo geo2;
		geo2.setGeographic(corr->getLatitude(), corr->getLongitude(),
		EARTHRADIUSKM - corr->getDepth());
		xDist = RAD2DEG * geo1.delta(&geo2);

		// check if distance difference is within window
		if (xDist < xWindow) {
			snprintf(
					sLog,
					sizeof(sLog),
					"CHypo::associate: C-ASSOC Hypo:%s Time:%s Station:%s"
					" Corr:%s tDist:%.2f<tWindow:%.2f"
					" xDist:%.2f>xWindow:%.2f)",
					m_sID.c_str(),
					glassutil::CDate::encodeDateTime(corr->getTCorrelation())
							.c_str(),
					corr->getSite()->getSCNL().c_str(), corr->getID().c_str(),
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
				m_sID.c_str(),
				glassutil::CDate::encodeDateTime(corr->getTCorrelation()).c_str(),
				corr->getSite()->getSCNL().c_str(), corr->getID().c_str(),
				tDist, tWindow);
	} else {
		snprintf(
				sLog,
				sizeof(sLog),
				"CHypo::associate: C-NOASSOC Hypo:%s Time:%s Station:%s Corr:%s"
				" tDist:%.2f<tWindow:%.2f xDist:%.2f>xWindow:%.2f)",
				m_sID.c_str(),
				glassutil::CDate::encodeDateTime(corr->getTCorrelation()).c_str(),
				corr->getSite()->getSCNL().c_str(), corr->getID().c_str(),
				tDist, tWindow, xDist, xWindow);
	}
	glassutil::CLogit::log(sLog);

	// it is, don't associate
	return (false);
}

// ---------------------------------------------------------cancel
std::shared_ptr<json::Object> CHypo::cancel(bool send) {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(m_HypoMutex);

	std::shared_ptr<json::Object> cancel = std::make_shared<json::Object>(
			json::Object());

	// fill in cancel command from current hypocenter
	(*cancel)["Cmd"] = "Cancel";
	(*cancel)["Pid"] = m_sID;

	// log it
	glassutil::CLogit::log(
			glassutil::log_level::debug,
			"CHypo::event: Created cancel message:" + json::Serialize(*cancel));

	// send it
	if ((send == true) && (m_pGlass != NULL)) {
		m_pGlass->send(cancel);
	}

	return (cancel);
}

// ---------------------------------------------------------cancel
bool CHypo::cancelCheck() {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(m_HypoMutex);

	// nullcheck
	if (m_pGlass == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::cancel: NULL pGlass.");
		return (false);
	}

	// can't cancel fixed hypos
	// NOTE: What implication does this have for "seed hypos" like twitter
	// detections
	if (m_bFixed) {
		return (false);
	}

	char sLog[2048];
	char sHypo[1024];

	glassutil::CDate dt = glassutil::CDate(m_tOrigin);
	snprintf(sHypo, sizeof(sHypo), "CHypo::cancel: %s tOrg:%s; dLat:%9.4f; "
				"dLon:%10.4f; dZ:%6.1f; bayes:%.2f; nPick:%d; nCorr:%d",
				m_sID.c_str(), dt.dateTime().c_str(), getLatitude(),
				getLongitude(), getDepth(), getBayesValue(),
				static_cast<int>(m_vPickData.size()),
				static_cast<int>(m_vCorrelationData.size()));

	// check to see if there is enough supporting data for this hypocenter
	// NOTE, in Node, ncut is used as a threshold for the number of
	// *stations* here it's used for the number of *picks*, not sure
	// what implication this has
	int ncut = m_iNucleationDataThreshold;

	// check correlations, we want a hypo created from a correlation
	// to stick around awhile to have a chance to associate picks.
	if (m_vCorrelationData.size() > 0) {
		// get the current time
		double now = glassutil::CDate::now();
		int cancelAge = m_pGlass->getCorrelationCancelAge();

		// check correlations
		int expireCount = 0;
		for (auto cor : m_vCorrelationData) {
			// count correlation as expired if it's creation time is older than
			// the cancel age
			if ((cor->getTCreate() + cancelAge) < now) {
				snprintf(sLog, sizeof(sLog),
							"CHypo::cancel: Correlation:%s created: %f "
							"limit:%d now:%f",
							m_sID.c_str(), cor->getTCreate(), cancelAge, now);
				glassutil::CLogit::log(sLog);
				expireCount++;
			}
		}

		// only prevent cancellation if all correlations haven't expired
		if (expireCount == m_vCorrelationData.size()) {
			snprintf(
					sLog, sizeof(sLog),
					"CHypo::cancel: $$Could cancel %s due to associated "
					"correlations (%d) older than %d seconds",
					m_sID.c_str(),
					(static_cast<int>(m_vCorrelationData.size()) - expireCount),
					cancelAge);
			glassutil::CLogit::log(sLog);
		} else {
			snprintf(
					sLog, sizeof(sLog),
					"CHypo::cancel: $$Will not cancel %s due to associated "
					"correlations (%d) younger than %d seconds",
					m_sID.c_str(),
					(static_cast<int>(m_vCorrelationData.size()) - expireCount),
					cancelAge);
			glassutil::CLogit::log(sLog);

			// Hypo is still viable, for now...
			return (false);
		}
	}

	// check data
	// NOTE: Is this a good idea?? JMP
	if ((m_vPickData.size() + m_vCorrelationData.size()) < ncut) {
		// there isn't
		snprintf(sLog, sizeof(sLog), "CHypo::cancel: Insufficient data "
					"((%d + %d) < %d) Hypo: %s",
					static_cast<int>(m_vPickData.size()),
					static_cast<int>(m_vCorrelationData.size()), ncut, sHypo);
		glassutil::CLogit::log(sLog);

		// this hypo can be canceled
		return (true);
	}

	// baysian threshold check
	double thresh = m_dNucleationStackThreshold;
	if (m_dBayesValue < thresh) {
		// failure
		snprintf(sLog, sizeof(sLog),
					"CHypo::cancel: Below threshold (%.1f < %.1f) Hypo: %s",
					getBayesValue(), thresh, sHypo);
		glassutil::CLogit::log(sLog);

		// this hypo can be canceled
		return (true);
	}

	// Whispy (event fragment) check (does the quake have a gap greater than 270
	// while being deeper than 550)
	// NOTE: Hardcoded
	if ((m_dDepth > 550.0) && (m_dGap > 270.0)) {
		// failure
		snprintf(sLog, sizeof(sLog),
					"CHypo::cancel: Whispie trap (%.1f>550, %.1f>270) Hypo: %s",
					getDepth(), getGap(), sHypo);
		glassutil::CLogit::log(sLog);

		// this hypo can be canceled
		return (true);
	}
	// Hypo is still viable
	return (false);
}

// ---------------------------------------------------------clear
void CHypo::clear() {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(m_HypoMutex);

	setLatitude(0.0);
	setLongitude(0.0);
	setDepth(0.0);
	setTOrigin(0.0);
	m_sID = "";
	m_iNucleationDataThreshold = 0;
	m_dNucleationStackThreshold = 0.0;
	m_dBayesValue = 0.0;
	m_dInitialBayesValue = 0.0;
	m_iCycleCount = 0;
	m_sWebName = "";
	m_dMedianDistance = 0;
	m_dMinDistance = 0;
	m_dGap = 0;
	m_dDistanceSD = 0;
	m_dKurtosisValue = 0;
	m_dWebResolution = 0;

	m_dDistanceCutoffFactor = 4.0;
	m_dDistanceCutoffPercentage = 0.4;
	m_dMinDistanceCutoff = 30.0;

	m_bFixed = false;
	m_bEventSent = false;

	m_pGlass = NULL;

	m_pTravelTimeTables.reset();
	m_pNucleationTravelTime1.reset();
	m_pNucleationTravelTime2.reset();

	m_bCorrelationAdded = false;

	clearPicks();
	clearCorrelations();

	m_iProcessCount = 0;
	m_iReportCount = 0;
}

// ---------------------------------------------------------clearCorrelations
void CHypo::clearCorrelations() {
	// lock the hypo since we're iterating through it's lists
	std::lock_guard<std::recursive_mutex> hypoGuard(m_HypoMutex);

	// go through all the corrs linked to this hypo
	for (auto corr : m_vCorrelationData) {
		if (corr == NULL) {
			continue;
		}

		// remove the hypo from the corr
		// note only removes hypo if corr
		// is linked
		corr->removeHypo(m_sID);
	}

	// remove all correlation links to this hypo
	m_vCorrelationData.clear();
}

// ---------------------------------------------------------clearPicks
void CHypo::clearPicks() {
	// lock the hypo since we're iterating through it's lists
	std::lock_guard<std::recursive_mutex> hypoGuard(m_HypoMutex);

	// go through all the picks linked to this hypo
	for (auto pck : m_vPickData) {
		if (pck == NULL) {
			continue;
		}

		// remove the hypo from the pick
		// note only removes hypo if pick
		// is linked
		pck->removeHypo(m_sID);
	}

	// remove all pick links to this hypo
	m_vPickData.clear();
}

// ---------------------------------------------------------event
std::shared_ptr<json::Object> CHypo::event(bool send) {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(m_HypoMutex);

	m_bEventSent = true;
	m_iReportCount++;
	std::shared_ptr<json::Object> event = std::make_shared<json::Object>(
			json::Object());

	// fill in Event command from current hypocenter
	(*event)["Cmd"] = "Event";
	(*event)["Pid"] = m_sID;
	(*event)["CreateTime"] = glassutil::CDate::encodeISO8601Time(m_tCreate);
	(*event)["ReportTime"] = glassutil::CDate::encodeISO8601Time(
			glassutil::CDate::now());
	(*event)["Version"] = getReportCount();

	// basic hypo information
	(*event)["Latitude"] = getLatitude();
	(*event)["Longitude"] = getLongitude();
	(*event)["Depth"] = getDepth();
	(*event)["Time"] = glassutil::CDate::encodeISO8601Time(m_tOrigin);
	(*event)["Bayes"] = getBayesValue();
	(*event)["Ndata"] = static_cast<int>(m_vPickData.size())
			+ static_cast<int>(m_vCorrelationData.size());

	// log it
	glassutil::CLogit::log(
			glassutil::log_level::debug,
			"CHypo::event: Created event message:" + json::Serialize(*event));

	// send it
	if ((send == true) && (m_pGlass != NULL)) {
		m_pGlass->send(event);
	}

	return (event);
}

// ---------------------------------------------------------expire
std::shared_ptr<json::Object> CHypo::expire(bool send) {
	std::shared_ptr<json::Object> expire = std::make_shared<json::Object>(
			json::Object());
	(*expire)["Cmd"] = "Expire";
	(*expire)["Pid"] = m_sID;

	// add a copy of the expiring hypo to the message
	// if we CAN report
	if (reportCheck() == true) {
		std::shared_ptr<json::Object> expireHypo = hypo(false);
		(*expire)["Hypo"] = (*expireHypo);
	}
	// log it
	glassutil::CLogit::log(
			glassutil::log_level::debug,
			"CHypo::event: Created expire message:" + json::Serialize(*expire));

	// send message
	if ((send == true) && (m_pGlass != NULL)) {
		m_pGlass->send(expire);
	}

	return (expire);
}

// ---------------------------------------------------------Gap
double CHypo::gap(double lat, double lon, double z) {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(m_HypoMutex);

	// set up a geographic object for this hypo
	glassutil::CGeo geo;
	geo.setGeographic(lat, lon, EARTHRADIUSKM - z);

	// create and populate vectors containing the
	// pick distances and azimuths
	std::vector<double> azm;

	for (auto pick : m_vPickData) {
		// get the site
		std::shared_ptr<CSite> site = pick->getSite();

		// compute the azimuth
		double azimuth = geo.azimuth(&site->getGeo()) / DEG2RAD;

		// add to azimuth vector
		azm.push_back(azimuth);
	}

	int nazm = azm.size();

	if (nazm <= 1) {
		return 360.;
	}

	// sort the azimuths
	sort(azm.begin(), azm.end());

	// add the first (smallest) azimuth to the end by adding 360
	azm.push_back(azm.front() + 360.0);

	// compute gap
	double tempGap = 0.0;

	for (int i = 0; i < nazm; i++) {
		double gap = azm[i + 1] - azm[i];
		if (gap > tempGap) {
			tempGap = gap;
		}
	}

	return tempGap;
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

// --------------------------------------------------------getResidual
double CHypo::getResidual(std::shared_ptr<CPick> pick) {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(m_HypoMutex);

	// setup traveltime interface for this hypo
	m_pTravelTimeTables->setOrigin(m_dLatitude, m_dLongitude, m_dDepth);

	// get site
	std::shared_ptr<CSite> site = pick->getSite();

	// compute observed traveltime
	double tObs = pick->getTPick() - m_tOrigin;

	// get expected travel time
	double tCal = m_pTravelTimeTables->T(&site->getGeo(), tObs);

	// Check if pick has an invalid travel time,
	if (tCal < 0.0) {
		// it does, don't associated
		return (std::numeric_limits<double>::quiet_NaN());
	}

	double tRes = tObs - tCal;
	return tRes;
}

double CHypo::getAzimuthTaper() const {
	return (m_dAzimuthTaper);
}

double CHypo::getMaxDepth() const {
	return (m_dMaxDepth);
}

double CHypo::getBayesValue() const {
	return (m_dBayesValue);
}

// ---------------------------------------------------getBayesStack
double CHypo::getBayes(double xlat, double xlon, double xZ, double oT,
						int nucleate) {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(m_HypoMutex);

	if ((!m_pNucleationTravelTime1) && (!m_pNucleationTravelTime2)) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::getBayes: NULL pTrv1 and pTrv2.");
		return (0);
	}

	if (!m_pTravelTimeTables) {
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
	geo.setGeographic(xlat, xlon, EARTHRADIUSKM - xZ);

	// This sets the travel-time look up location
	if (m_pNucleationTravelTime1) {
		m_pNucleationTravelTime1->setOrigin(xlat, xlon, xZ);
	}
	if (m_pNucleationTravelTime2) {
		m_pNucleationTravelTime2->setOrigin(xlat, xlon, xZ);
	}

	m_pTravelTimeTables->setOrigin(xlat, xlon, xZ);

	// The number of picks associated with the hypocenter
	int npick = m_vPickData.size();

	// Loop through each pick and find the residual, calculate
	// the significance, and add to the stacks.
	// Currently only P, S, and nucleation phases added to stack.
	for (int ipick = 0; ipick < npick; ipick++) {
		auto pick = m_vPickData[ipick];
		double resi = 99999999;

		// calculate residual
		double tobs = pick->getTPick() - oT;
		std::shared_ptr<CSite> site = pick->getSite();
		glassutil::CGeo siteGeo = site->getGeo();

		// only use nucleation phases if on nucleation branch
		if (nucleate == 1) {
			if ((m_pNucleationTravelTime1) && (m_pNucleationTravelTime2)) {
				// we have both nucleation phases
				// first nucleation phase
				// calculate the residual using the phase name
				double tcal1 = m_pNucleationTravelTime1->T(&siteGeo);
				double resi1 = getWeightedResidual(
						m_pNucleationTravelTime1->sPhase, tobs, tcal1);

				// second nucleation phase
				// calculate the residual using the phase name
				double tcal2 = m_pNucleationTravelTime2->T(&siteGeo);
				double resi2 = getWeightedResidual(
						m_pNucleationTravelTime2->sPhase, tobs, tcal2);

				// use the smallest residual
				if (abs(resi1) < abs(resi2)) {
					tcal = tcal1;
					resi = resi1;
				} else {
					tcal = tcal2;
					resi = resi2;
				}
			} else if ((m_pNucleationTravelTime1)
					&& (!m_pNucleationTravelTime2)) {
				// we have just the first nucleation phase
				tcal = m_pNucleationTravelTime1->T(&siteGeo);
				resi = getWeightedResidual(m_pNucleationTravelTime1->sPhase,
											tobs, tcal);
			} else if ((!m_pNucleationTravelTime1)
					&& (m_pNucleationTravelTime2)) {
				// we have just the second ducleation phase
				tcal = m_pNucleationTravelTime2->T(&siteGeo);
				resi = getWeightedResidual(m_pNucleationTravelTime2->sPhase,
											tobs, tcal);
			}
		} else {
			// use all available association phases
			// take whichever phase has the smallest residual
			tcal = m_pTravelTimeTables->T(&siteGeo, tobs);

			// calculate the residual using the phase name
			resi = getWeightedResidual(m_pTravelTimeTables->sPhase, tobs, tcal);
		}

		// calculate distance to station to get sigma
		double delta = RAD2DEG * geo.delta(&site->getGeo());
		double sigma = (tap.Val(delta) * 2.25) + 0.75;

		// calculate and add to the stack
		value += m_pGlass->sig(resi, sigma);
	}
	return value;
}

double CHypo::getInitialBayesValue() const {
	return (m_dInitialBayesValue);
}

bool CHypo::getCorrelationAdded() const {
	return (m_bCorrelationAdded);
}

int CHypo::getNucleationDataThreshold() const {
	return (m_iNucleationDataThreshold);
}

double CHypo::getDistanceCutoff() const {
	return (m_dDistanceCutoff);
}

double CHypo::getDistanceCutoffFactor() const {
	return (m_dDistanceCutoffFactor);
}

double CHypo::getMinDistanceCutoff() const {
	return (m_dMinDistanceCutoff);
}

double CHypo::getDistanceCutoffPercentage() const {
	return (m_dDistanceCutoffPercentage);
}

int CHypo::getCycleCount() const {
	return (m_iCycleCount);
}

bool CHypo::getEventSent() const {
	return (m_bEventSent);
}

bool CHypo::getFixed() const {
	return (m_bFixed);
}

double CHypo::getGap() const {
	return (m_dGap);
}

glassutil::CGeo CHypo::getGeo() const {
	std::lock_guard<std::recursive_mutex> hypoGuard(m_HypoMutex);
	glassutil::CGeo geoHypo;
	geoHypo.setGeographic(m_dLatitude, m_dLongitude, EARTHRADIUSKM - m_dDepth);
	return (geoHypo);
}

const CGlass* CHypo::getGlass() const {
	std::lock_guard<std::recursive_mutex> hypoGuard(m_HypoMutex);
	return (m_pGlass);
}

double CHypo::getKurtosisValue() const {
	return (m_dKurtosisValue);
}

double CHypo::getLatitude() const {
	return (m_dLatitude);
}

double CHypo::getLongitude() const {
	return (m_dLongitude);
}

double CHypo::getMedianDistance() const {
	return (m_dMedianDistance);
}

double CHypo::getMinDistance() const {
	return (m_dMinDistance);
}

std::mutex & CHypo::getProcessingMutex() {
	return (m_ProcessingMutex);
}

const std::string& CHypo::getID() const {
	return (m_sID);
}

int CHypo::getProcessCount() const {
	return (m_iProcessCount);
}

int CHypo::getReportCount() const {
	return (m_iReportCount);
}

double CHypo::getWebResolution() const {
	return (m_dWebResolution);
}

double CHypo::getDistanceSD() const {
	return (m_dDistanceSD);
}

double CHypo::getTCreate() const {
	return (m_tCreate);
}

double CHypo::getNucleationStackThreshold() const {
	return (m_dNucleationStackThreshold);
}

std::shared_ptr<traveltime::CTravelTime> CHypo::getNucleationTravelTime1() const {  // NOLINT
	std::lock_guard<std::recursive_mutex> hypoGuard(m_HypoMutex);
	return (m_pNucleationTravelTime1);
}

std::shared_ptr<traveltime::CTravelTime> CHypo::getNucleationTravelTime2() const {  // NOLINT
	std::lock_guard<std::recursive_mutex> hypoGuard(m_HypoMutex);
	return (m_pNucleationTravelTime2);
}

std::shared_ptr<traveltime::CTTT> CHypo::getTravelTimeTables() const {
	std::lock_guard<std::recursive_mutex> hypoGuard(m_HypoMutex);
	return (m_pTravelTimeTables);
}

int CHypo::getPickDataSize() const {
	std::lock_guard<std::recursive_mutex> hypoGuard(m_HypoMutex);
	return (m_vPickData.size());
}

std::vector<std::shared_ptr<CPick>> CHypo::getPickData() const {
	std::lock_guard<std::recursive_mutex> hypoGuard(m_HypoMutex);
	return (m_vPickData);
}

int CHypo::getCorrelationDataSize() const {
	std::lock_guard<std::recursive_mutex> hypoGuard(m_HypoMutex);
	return (m_vCorrelationData.size());
}

const std::string& CHypo::getWebName() const {
	return (m_sWebName);
}

// ---------------------------------------------------getSumAbsResidual
double CHypo::getSumAbsResidual(double xlat, double xlon, double xZ, double oT,
								int nucleate) {
	if (m_pTravelTimeTables == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::getSumAbsResidual: NULL pTTT.");
		return (0);
	}

	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(m_HypoMutex);

	double sigma;
	double value = 0.;
	char sLog[1024];

	// This sets the travel-time look up location
	if (m_pNucleationTravelTime1 != NULL) {
		m_pNucleationTravelTime1->setOrigin(xlat, xlat, xZ);
	}
	if (m_pNucleationTravelTime2 != NULL) {
		m_pNucleationTravelTime2->setOrigin(xlat, xlat, xZ);
	}

	m_pTravelTimeTables->setOrigin(xlat, xlon, xZ);

	// The number of picks associated with the hypocenter
	int npick = m_vPickData.size();

	// Loop through each pick and find the residual, calculate
	// the resiudal, and sum.
	// Currently only P, S, and nucleation phases added to stack.
	// If residual is greater than 10, make it 10.
	for (int ipick = 0; ipick < npick; ipick++) {
		auto pick = m_vPickData[ipick];
		double resi = 99999999;

		// calculate residual
		double tobs = pick->getTPick() - oT;
		double tcal;
		std::shared_ptr<CSite> site = pick->getSite();

		// only use nucleation phase if on nucleation branch
		if (nucleate == 1 && m_pNucleationTravelTime2 == NULL) {
			tcal = m_pNucleationTravelTime1->T(&site->getGeo());
			resi = tobs - tcal;
		} else if (nucleate == 1 && m_pNucleationTravelTime1 == NULL) {
			tcal = m_pNucleationTravelTime2->T(&site->getGeo());
			resi = tobs - tcal;
		} else {
			// take whichever has the smallest residual, P or S
			tcal = m_pTravelTimeTables->T(&site->getGeo(), tobs);
			if (m_pTravelTimeTables->sPhase == "P"
					|| m_pTravelTimeTables->sPhase == "S") {
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

double CHypo::getTOrigin() const {
	return (m_tOrigin);
}

// ---------------------------------------------------------getWeightedResidual
double CHypo::getWeightedResidual(std::string sPhase, double tObs,
									double tCal) {
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

double CHypo::getDepth() const {
	return (m_dDepth);
}

// ---------------------------------------------------graphicsOutput
void CHypo::graphicsOutput() {
	if (m_pTravelTimeTables == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::graphicsOutput: NULL pTTT.");
		return;
	}
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(m_HypoMutex);

	// create and open file
	std::ofstream outfile;
	std::string filename = m_pGlass->getGraphicsOutFolder() + m_sID + ".txt";
	outfile.open(filename, std::ios::out);

	// header
	outfile << "hypocenter: " << std::to_string(m_dLatitude) << " "
			<< std::to_string(m_dLongitude) << " " << std::to_string(m_dDepth)
			<< " " << std::to_string(m_tOrigin) << "\n";

	double stack = 0;
	double tcal = 0;
	double delta = 0;
	double sigma = 0;
	glassutil::CGeo geo;

	int npick = m_vPickData.size();
	// for each step in the y axis
	for (int y = -1 * m_pGlass->getGraphicsSteps();
			y <= m_pGlass->getGraphicsSteps(); y++) {
		// compute lat
		double xlat = m_dLatitude + (y * m_pGlass->getGraphicsStepKm()) / DEG2KM;

		// for each step in the x axis
		for (int x = -1 * m_pGlass->getGraphicsSteps();
				x <= m_pGlass->getGraphicsSteps(); x++) {
			// compute lon
			double xlon = m_dLongitude
					+ cos(DEG2RAD * xlat) * (x * m_pGlass->getGraphicsStepKm())
							/ DEG2KM;

			// set up traveltimes
			m_pTravelTimeTables->setOrigin(xlat, xlon, m_dDepth);
			stack = 0;

			// for each pick
			for (int ipick = 0; ipick < npick; ipick++) {
				auto pick = m_vPickData[ipick];
				double tobs = pick->getTPick() - m_tOrigin;
				std::shared_ptr<CSite> site = pick->getSite();
				tcal = m_pTravelTimeTables->T(&site->getGeo(), tobs);
				delta = RAD2DEG * geo.delta(&site->getGeo());

				// This should be a function.
				if (delta < 1.5) {
					sigma = .75;
				} else if ((delta >= 1.5) && (delta < 30.0)) {
					sigma = 1.5;
				} else {
					sigma = 3;
				}

				// compute stack value
				stack += m_pGlass->sig_laplace_pdf(tobs - tcal, sigma);

				// write to fiel
				outfile << std::to_string(xlat) << " " << std::to_string(xlon)
						<< " " << std::to_string(stack) << "\n";
			}
		}
	}

	outfile.close();
}

// ---------------------------------------------------------hasCorrelation
bool CHypo::hasCorrelation(std::shared_ptr<CCorrelation> corr) {
	// null check
	if (corr == NULL) {
		glassutil::CLogit::log(glassutil::log_level::warn,
								"CHypo::hasCorrelation: NULL correlation.");
		return (false);
	}

	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(m_HypoMutex);

	// for each corr in the vector
	for (const auto &q : m_vCorrelationData) {
		// is this corr a match?
		if (q->getID() == corr->getID()) {
			return (true);
		}
	}

	return (false);
}

// ---------------------------------------------------------hasPick
bool CHypo::hasPick(std::shared_ptr<CPick> pck) {
	// null check
	if (pck == NULL) {
		glassutil::CLogit::log(glassutil::log_level::warn,
								"CHypo::hasPick: NULL pck.");
		return (false);
	}

	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(m_HypoMutex);

	// for each pick in the vector
	for (const auto &q : m_vPickData) {
		// is this pick a match?
		if (q->getID() == pck->getID()) {
			return (true);
		}
	}

	return (false);
}

// ---------------------------------------------------------Hypo
std::shared_ptr<json::Object> CHypo::hypo(bool send) {
	std::shared_ptr<json::Object> hypo = std::make_shared<json::Object>(
			json::Object());

	// null check
	if (m_pGlass == NULL) {
		glassutil::CLogit::log(glassutil::log_level::warn,
								"CHypo::Hypo: NULL pGlass.");
		return (hypo);
	}
	if (m_pTravelTimeTables == NULL) {
		glassutil::CLogit::log(glassutil::log_level::warn,
								"CHypo::Hypo: NULL pTTT.");
		return (hypo);
	}

	// make sure this event is still reportable
	if (reportCheck() == false) {
		glassutil::CLogit::log(
				glassutil::log_level::debug,
				"CHypo::Hypo: hypo:" + m_sID + " is not reportable.");
		return (hypo);
	}

	glassutil::CLogit::log(
			glassutil::log_level::debug,
			"CHypo::Hypo: generating hypo message for sPid:" + m_sID
					+ " sWebName:" + m_sWebName);

	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(m_HypoMutex);

	// NOTE: Need to think about this format, currently it *almost*
	// creates a detection formats json, but doesn't use the library
	// maybe this should be it's own "glass" format? or just use
	// the quake format?

	// basic info
	(*hypo)["Cmd"] = "Hypo";
	(*hypo)["Type"] = "Hypo";
	(*hypo)["ID"] = m_sID;

	// source
	// NOTE: THIS NEEDS TO BE NOT HARDCODED EVENTUALLY
	// or remove it? why should glasscore care about source?
	json::Object src;
	src["AgencyID"] = "US";
	src["Author"] = "glass";
	(*hypo)["Source"] = src;

	// time
	(*hypo)["T"] = glassutil::CDate::encodeDateTime(m_tOrigin);
	(*hypo)["Time"] = glassutil::CDate::encodeISO8601Time(m_tOrigin);

	// location
	(*hypo)["Latitude"] = getLatitude();
	(*hypo)["Longitude"] = getLongitude();
	(*hypo)["Depth"] = getDepth();

	// supplementary info
	(*hypo)["MinimumDistance"] = getMinDistance();
	(*hypo)["Gap"] = getGap();
	(*hypo)["Bayes"] = getBayesValue();
	(*hypo)["InitialBayes"] = getInitialBayesValue();
	(*hypo)["Web"] = m_sWebName;

	// generate data array for this hypo
	// set up traveltime object
	m_pTravelTimeTables->setOrigin(m_dLatitude, m_dLongitude, m_dDepth);

	// set up geo for distance calculations
	glassutil::CGeo geo;
	geo.setGeographic(m_dLatitude, m_dLongitude, EARTHRADIUSKM - m_dDepth);

	// array to hold data
	json::Array data;

	// for each pick
	for (auto pick : m_vPickData) {
		// get basic pick values
		std::shared_ptr<CSite> site = pick->getSite();
		double tobs = pick->getTPick() - m_tOrigin;
		double tcal = m_pTravelTimeTables->T(&site->getGeo(), tobs);
		double tres = tobs - tcal;
		// should this be changed?
		double sig = m_pGlass->sig(tres, 1.0);

		// if we have it, use the shared pointer
		json::Object pickObj;
		std::shared_ptr<json::Object> jPick = pick->getJSONPick();
		if (jPick) {
			// start with a copy of json pick
			// which has the site, source, time, etc
			pickObj = json::Object(*jPick.get());

			// add the association info
			json::Object assocobj;
			assocobj["Phase"] = m_pTravelTimeTables->sPhase;
			assocobj["Distance"] = geo.delta(&site->getGeo()) / DEG2RAD;
			assocobj["Azimuth"] = geo.azimuth(&site->getGeo()) / DEG2RAD;
			assocobj["Residual"] = tres;
			assocobj["Sigma"] = sig;
			pickObj["AssociationInfo"] = assocobj;
		} else {
			// we don't have a jpick, so fill in what we know
			pickObj["Site"] = site->getSCNL();
			pickObj["Pid"] = pick->getID();
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
	for (auto correlation : m_vCorrelationData) {
		// get basic pick values
		std::shared_ptr<CSite> site = correlation->getSite();
		double tobs = correlation->getTCorrelation() - m_tOrigin;
		double tcal = m_pTravelTimeTables->T(&site->getGeo(), tobs);
		double tres = tobs - tcal;
		// should this be changed?
		double sig = m_pGlass->sig(tres, 1.0);

		// if we have it, use the shared pointer
		json::Object correlationObj;
		std::shared_ptr<json::Object> jCorrelation = correlation
				->getJSONCorrelation();
		if (jCorrelation) {
			// start with a copy of json pick
			// which has the site, source, time, etc
			correlationObj = json::Object(*jCorrelation.get());

			// add the association info
			json::Object assocobj;
			assocobj["Phase"] = m_pTravelTimeTables->sPhase;
			assocobj["Distance"] = geo.delta(&site->getGeo()) / DEG2RAD;
			assocobj["Azimuth"] = geo.azimuth(&site->getGeo()) / DEG2RAD;
			assocobj["Residual"] = tres;
			assocobj["Sigma"] = sig;
			correlationObj["AssociationInfo"] = assocobj;
		} else {
			// we don't have a jCorrelation, so fill in what we know
			correlationObj["Site"] = site->getSCNL();
			correlationObj["Pid"] = correlation->getID();
			correlationObj["Time"] = glassutil::CDate::encodeISO8601Time(
					correlation->getTCorrelation());
			correlationObj["Latitude"] = correlation->getLatitude();
			correlationObj["Longitude"] = correlation->getLongitude();
			correlationObj["Depth"] = correlation->getDepth();
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

	if ((send == true) && (m_pGlass != NULL)) {
		m_pGlass->send(hypo);
	}

	// done
	return (hypo);
}

int CHypo::incrementProcessCount() {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(m_HypoMutex);

	m_iProcessCount++;

	return (m_iProcessCount);
}

// ---------------------------------------------------------initialize
bool CHypo::initialize(double lat, double lon, double z, double time,
						std::string pid, std::string web, double bayes,
						double thresh, int cut,
						std::shared_ptr<traveltime::CTravelTime> firstTrav,
						std::shared_ptr<traveltime::CTravelTime> secondTrav,
						std::shared_ptr<traveltime::CTTT> ttt,
						double resolution, double aziTap, double maxDep) {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(m_HypoMutex);

	clear();

	setLatitude(lat);
	setLongitude(lon);
	setDepth(z);
	setTOrigin(time);
	m_sID = pid;
	m_sWebName = web;
	m_dBayesValue = bayes;
	m_dInitialBayesValue = bayes;
	m_dAzimuthTaper = aziTap;
	m_dMaxDepth = maxDep;
	m_dNucleationStackThreshold = thresh;
	m_iNucleationDataThreshold = cut;
	m_dWebResolution = resolution;
	if (m_dWebResolution == 0.) {
		m_dWebResolution = 100.;
	}

	glassutil::CLogit::log(
			glassutil::log_level::debug,
			"CHypo::initialize: lat:" + std::to_string(m_dLatitude) + "; lon:"
					+ std::to_string(m_dLongitude) + "; z:"
					+ std::to_string(m_dDepth) + "; t:"
					+ std::to_string(m_tOrigin) + "; sPid:" + m_sID + "; web:"
					+ m_sWebName + "; bayes:" + std::to_string(m_dBayesValue)
					+ "; thresh:" + std::to_string(m_dNucleationStackThreshold)
					+ "; cut:" + std::to_string(m_iNucleationDataThreshold)
					+ "; resolution:" + std::to_string(m_dWebResolution));

	// make local copies of the travel times so that we don't
	// have cross-thread contention for them between hypos
	if (firstTrav != NULL) {
		m_pNucleationTravelTime1 = std::make_shared<traveltime::CTravelTime>(
				traveltime::CTravelTime(*firstTrav));
	}

	if (secondTrav != NULL) {
		m_pNucleationTravelTime2 = std::make_shared<traveltime::CTravelTime>(
				traveltime::CTravelTime(*secondTrav));
	}

	if (ttt != NULL) {
		m_pTravelTimeTables = std::make_shared<traveltime::CTTT>(
				traveltime::CTTT(*ttt));
	}
	m_tCreate = glassutil::CDate::now();

	return (true);
}

bool CHypo::isLockedForProcessing() {
	if (m_ProcessingMutex.try_lock() == false) {
		return (true);
	}

	m_ProcessingMutex.unlock();
	return (false);
}

// ---------------------------------------------------------localize
double CHypo::localize() {
	// Localize this hypo
	// nullcheck
	if (m_pGlass == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::localize: NULL pGlass.");
		return (0);
	}
	char sLog[1024];

	glassutil::CLogit::log(glassutil::log_level::debug,
							"CHypo::localize. " + m_sID);

	// if hypo is fixed, just return current bayesian value
	// NOTE: What implication does this have for "seed hypos" like twitter
	// detections
	if (m_bFixed) {
		return (m_dBayesValue);
	}

	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(m_HypoMutex);

	// get the number of picks
	int npick = m_vPickData.size();

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
	double searchR = (m_dWebResolution / 4.
			+ taper.Val(m_vPickData.size()) * .75 * m_dWebResolution) / 2.;

	// This should be the default
	if (m_pGlass->getMinimizeTTLocator() == false) {
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
	glassutil::CDate dt = glassutil::CDate(m_tOrigin);
	snprintf(sLog, sizeof(sLog),
				"CHypo::localize: HYP %s %s%9.4f%10.4f%6.1f %d", m_sID.c_str(),
				dt.dateTime().c_str(), getLatitude(), getLongitude(),
				getDepth(), static_cast<int>(m_vPickData.size()));
	glassutil::CLogit::log(sLog);

	// return the final maximum bayesian fit
	return (m_dBayesValue);
}

void CHypo::lockForProcessing() {
	m_ProcessingMutex.lock();
}

// ---------------------------------------------------------prune
bool CHypo::prune() {
	// nullcheck
	if (m_pGlass == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::prune: NULL pGlass.");
		return (false);
	}

	char sLog[1024];

	glassutil::CLogit::log(glassutil::log_level::debug,
							"CHypo::prune. " + m_sID);

	// set up local vector to track picks to remove
	std::vector<std::shared_ptr<CPick>> vremove;

	// set up a geographic object for this hypo
	glassutil::CGeo geo;
	geo.setGeographic(m_dLatitude, m_dLongitude, EARTHRADIUSKM);

	// get the standard deviation allowed for pruning
	double sdprune = m_pGlass->getPruningSDCutoff();

	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(m_HypoMutex);

	// for each pick in this hypo
	for (auto pck : m_vPickData) {
		// check to see if it can still be associated
		if (!associate(pck, 1.0, sdprune)) {
			// pick no longer associates, add to remove list
			vremove.push_back(pck);

			snprintf(
					sLog, sizeof(sLog), "CHypo::prune: CUL %s %s (%.2f)",
					glassutil::CDate::encodeDateTime(pck->getTPick()).c_str(),
					pck->getSite()->getSCNL().c_str(), sdprune);
			glassutil::CLogit::log(sLog);

			// on to the next pick
			continue;
		}

		// Trim whiskers
		// compute delta between site and hypo
		// THIS NEEDS TO BE CONVERTER DO DEG BUT NEED TO TEST LATER - WY
		double delta = geo.delta(&pck->getSite()->getGeo());
		// check if delta is beyond distance limit
		if (delta > m_dDistanceCutoff) {
			snprintf(
					sLog, sizeof(sLog), "CHypo::prune: CUL %s %s (%.2f > %.2f)",
					glassutil::CDate::encodeDateTime(pck->getTPick()).c_str(),
					pck->getSite()->getSCNL().c_str(), delta,
					getDistanceCutoff());
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
		removePick(pck);
	}

	glassutil::CLogit::log(
			glassutil::log_level::debug,
			"CHypo::prune pick pruneCount:" + std::to_string(pruneCount));

	// set up local vector to track correlations to remove
	std::vector<std::shared_ptr<CCorrelation>> vcremove;

	// get the correlation windows
	double tWindow = m_pGlass->getCorrelationMatchingTimeWindow();
	double xWindow = m_pGlass->getCorrelationMatchingDistanceWindow();

	// for each correlation in this hypo
	for (auto cor : m_vCorrelationData) {
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
					cor->getSite()->getSCNL().c_str());
			glassutil::CLogit::log(sLog);

			// on to the next correlation
			continue;
		}
	}

	for (auto cor : vcremove) {
		cor->clearHypo();
		removeCorrelation(cor);
	}

	// if we didn't find any data to remove, just
	// return
	if ((vremove.size() < 1) && (vcremove.size() < 1)) {
		return (false);
	}

	// we've removed at least one data
	return (true);
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

// ---------------------------------------------------------remCorrelation
void CHypo::removeCorrelation(std::shared_ptr<CCorrelation> corr) {
	// null check
	if (corr == NULL) {
		glassutil::CLogit::log(glassutil::log_level::warn,
								"CHypo::remCorrelation: NULL correlation.");
		return;
	}

	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(m_HypoMutex);

	// get the correlation id
	std::string pid = corr->getID();

	// for each correlation in the vector
	for (int i = 0; i < m_vCorrelationData.size(); i++) {
		// get the current correlation
		auto correlation = m_vCorrelationData[i];

		// is this correlation a match?
		if (correlation->getID() == pid) {
			// remove correlation from vector
			m_vCorrelationData.erase(m_vCorrelationData.cbegin() + i);
			return;
		}
	}
}

// ---------------------------------------------------------remPick
void CHypo::removePick(std::shared_ptr<CPick> pck) {
	// null check
	if (pck == NULL) {
		glassutil::CLogit::log(glassutil::log_level::warn,
								"CHypo::remPick: NULL pck.");
		return;
	}

	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(m_HypoMutex);

	// get the pick id
	std::string pid = pck->getID();

	// for each pick in the vector
	for (int i = 0; i < m_vPickData.size(); i++) {
		// get the current pick
		auto pick = m_vPickData[i];

		// is this pick a match?
		if (pick->getID() == pid) {
			// remove pick from vector
			m_vPickData.erase(m_vPickData.cbegin() + i);

			return;
		}
	}
}

// ---------------------------------------------------------cancel
bool CHypo::reportCheck() {
	// nullcheck
	if (m_pGlass == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::cancel: NULL pGlass.");
		return (false);
	}

	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(m_HypoMutex);

	char sLog[2048];
	char sHypo[1024];

	glassutil::CDate dt = glassutil::CDate(m_tOrigin);
	snprintf(sHypo, sizeof(sHypo), "%s %s%9.4f%10.4f%6.1f %d", m_sID.c_str(),
				dt.dateTime().c_str(), getLatitude(), getLongitude(),
				getDepth(), static_cast<int>(m_vPickData.size()));

	int nReportCut = m_pGlass->getReportingDataThreshold();

	// check data count
	if ((m_vPickData.size() + m_vCorrelationData.size()) < nReportCut) {
		// there isn't
		snprintf(sLog, sizeof(sLog),
					"CHypo::reportCheck: Below data count threshold "
					"((%d + %d) < %d) Hypo: %s",
					static_cast<int>(m_vPickData.size()),
					static_cast<int>(m_vCorrelationData.size()), nReportCut,
					sHypo);
		glassutil::CLogit::log(sLog);

		// this hypo cannot be reported
		return (false);
	}

	// rms check? other checks?

	// baysian threshold check
	double dReportThresh = m_pGlass->getReportingStackThreshold();
	if (m_dBayesValue < dReportThresh) {
		// failure
		snprintf(
				sLog, sizeof(sLog),
				"CHypo::reportCheck: Below bayesian threshold (%.1f < %.1f) "
				"Hypo: %s",
				getBayesValue(), dReportThresh, sHypo);
		glassutil::CLogit::log(sLog);

		// this hypo cannot be reported
		return (false);
	}

	// Hypo can be reported
	return (true);
}

bool CHypo::resolve(std::shared_ptr<CHypo> hyp) {
	// nullchecks
	if (m_pGlass == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::resolve: NULL pGlass.");
		return (false);
	}
	if (m_pGlass->getHypoList() == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypo::resolve: NULL pGlass pHypoList.");
		return (false);
	}

	glassutil::CLogit::log(glassutil::log_level::debug,
							"CHypo::resolve. " + m_sID);

	// lock the hypo since we're iterating through it's lists
	std::lock_guard<std::recursive_mutex> hypoGuard(m_HypoMutex);

	bool bAss = false;
	char sLog[1024];

	// handle picks
	// for each pick in this hypo
	int nPck = m_vPickData.size();

	int keptCount = 0;
	int removeCount = 0;

	// NOTE: Why are we moving backwards through the list?
	for (int iPck = nPck - 1; iPck >= 0; iPck--) {
		// get the pick
		std::shared_ptr<CPick> pck = m_vPickData[iPck];

		// get the pick's hypo pointer
		std::shared_ptr<CHypo> pickHyp = pck->getHypo();

		// if this pick isn't linked to a hypo
		if (pickHyp == NULL) {
			// link to this hypo and move on
			pck->addHypo(hyp);

			continue;
		}

		std::string sOtherPid = pickHyp->getID();

		// if this pick is linked to this hypo
		if (sOtherPid == m_sID) {
			// nothing else to do
			continue;
		}

		// get the current pick's affinity to the provided hypo
		double aff1 = affinity(pck);

		// get the current pick's affinity to the hypo it's linked to
		double aff2 = pickHyp->affinity(pck);

		snprintf(sLog, sizeof(sLog),
					"CHypo::resolve: SCV COMPARE %s %s %s %s (%.2f, %.2f)",
					m_sID.c_str(), sOtherPid.c_str(),
					glassutil::CDate::encodeDateTime(pck->getTPick()).c_str(),
					pck->getSite()->getSCNL().c_str(), aff1, aff2);
		glassutil::CLogit::log(sLog);

		// check which affinity is better
		if (aff1 > aff2) {
			// this pick has a higher affinity with the provided hypo
			// remove the pick from it's original hypo
			pickHyp->removePick(pck);

			// link pick to the provided hypo
			pck->addHypo(hyp, true);

			// add provided hypo to the processing queue
			// NOTE: this puts provided hypo before original hypo in FIFO,
			// we want this hypo to keep this pick, rather than the original
			// just stealing it back, which is why we do it here
			// NOTE: why add it at all? we're gonna locate before we finish
			// is it to see if we can get more next time
			m_pGlass->getHypoList()->addHypoToProcess(hyp);

			// add the original hypo the pick was linked to the processing queue
			m_pGlass->getHypoList()->addHypoToProcess(pickHyp);

			// we've made a change to the hypo (grabbed a pick)
			bAss = true;
			keptCount++;
		} else {
			// this pick has higher affinity with the original hypo
			// remove pick from provided hypo
			removePick(pck);

			// we've made a change to the hypo (got rid of a pick)
			bAss = true;
			removeCount++;
		}
	}

	glassutil::CLogit::log(
			glassutil::log_level::debug,
			"CHypo::resolve " + m_sID + " kept:" + std::to_string(keptCount)
					+ " removed:" + std::to_string(removeCount));

	// handle correlations
	// for each correlation in this hypo
	int nCorr = m_vCorrelationData.size();

	// NOTE: Why are we moving backwards through the list?
	for (int iCorr = nCorr - 1; iCorr >= 0; iCorr--) {
		// get the correlation
		auto corr = m_vCorrelationData[iCorr];

		// get the correlation's hypo pointer
		std::shared_ptr<CHypo> corrHyp = corr->getHypo();

		// if this correlation isn't linked to a hypo
		if (corrHyp == NULL) {
			// link to this hypo and move on
			corr->addHypo(hyp);
			continue;
		}

		std::string sOtherPid = corrHyp->getID();

		// if this corr is linked to this hypo
		if (sOtherPid == m_sID) {
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
				m_sID.c_str(),
				sOtherPid.c_str(),
				glassutil::CDate::encodeDateTime(corr->getTCorrelation()).c_str(),
				corr->getSite()->getSCNL().c_str(), aff1, aff2);
		glassutil::CLogit::log(sLog);

		// check which affinity is better
		if (aff1 > aff2) {
			// this correlation has a higher affinity with the provided hypo
			// remove the correlation from it's original hypo
			corrHyp->removeCorrelation(corr);

			// link pick to the provided hypo
			corr->addHypo(hyp, true);

			// add provided hypo to the processing queue
			// NOTE: this puts provided hypo before original hypo in FIFO,
			// we want this hypo to keep this pick, rather than the original
			// just stealing it back, which is why we do it here
			m_pGlass->getHypoList()->addHypoToProcess(hyp);

			// add the original hypo the pick was linked to the processing queue
			m_pGlass->getHypoList()->addHypoToProcess(corrHyp);

			// we've made a change to the hypo (grabbed a pick)
			bAss = true;
		} else {
			// this pick has higher affinity with the original hypo
			// remove pick from provided hypo
			removeCorrelation(corr);

			// we've made a change to the hypo (got rid of a pick)
			bAss = true;
		}
	}

	return (bAss);
}

// ---------------------------------------------------------stats
void CHypo::stats() {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(m_HypoMutex);

	// Calculate the statistical distribution of distance
	// histogram for culling purposes. The actual values are
	// reflected to give 0 mean, which is, of course, completely
	// wierd. =). Consequently, the mean is not of interest.
	// don't bother with calculations if
	// there are no picks
	if (m_vPickData.size() < 1) {
		m_dDistanceSD = 0.0;
		m_dKurtosisValue = 0.0;
		m_dMedianDistance = 0.0;
		m_dMinDistance = 0.0;
		m_dDistanceCutoff = 0.0;
		m_dGap = 360.0;
		return;
	}

	// set up a geographic object for this hypo
	glassutil::CGeo geo;
	geo.setGeographic(m_dLatitude, m_dLongitude, EARTHRADIUSKM - m_dDepth);

	// create and populate vectors containing the
	// pick distances and azimuths
	std::vector<double> dis;
	std::vector<double> azm;
	for (auto pick : m_vPickData) {
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
	m_dDistanceSD = sqrt(var);

	// calculate the sample excess kurtosis value
	sum = 0.0;
	for (int i = 0; i < ndis; i++) {
		double arg = dis[i] / m_dDistanceSD;
		sum += arg * arg * arg * arg;
	}
	m_dKurtosisValue = sum / ndis - 3.0;

	// sort the distances
	sort(dis.begin(), dis.end());

	// get minimum distance
	m_dMinDistance = dis.front();

	// get median distance
	m_dMedianDistance = dis[ndis / 2];

	// compute distance cutoff
	// NOTE: Harley want's both the original .4 and 4.0 values to be
	// configurable via glass_init.
	// In the long term, he wants to replace this with a more statistics
	// based algorithm
	int icut = static_cast<int>((m_dDistanceCutoffPercentage * ndis));
	m_dDistanceCutoff = m_dDistanceCutoffFactor * dis[icut];

	// make sure our calculated dCut is not below the minimum allowed
	if (m_dDistanceCutoff < getMinDistanceCutoff()) {
		m_dDistanceCutoff = getMinDistanceCutoff();
	}

	// sort the azimuths
	sort(azm.begin(), azm.end());

	// add the first (smallest) azimuth to the end by adding 360
	azm.push_back(azm.front() + 360.0);

	// compute gap
	m_dGap = 0.0;
	for (int i = 0; i < ndis; i++) {
		double gap = azm[i + 1] - azm[i];
		if (gap > m_dGap) {
			m_dGap = gap;
		}
	}
}

// ---------------------------------------------------------trap
void CHypo::trap() {
	// lock mutex for this scope
	std::lock_guard<std::recursive_mutex> guard(m_HypoMutex);

	char sLog[1024];

	// for each pick in this hypocenter
	for (const auto &q : m_vPickData) {
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
						m_sID.c_str(), q->getID().c_str());
			glassutil::CLogit::log(glassutil::log_level::warn, sLog);

			continue;
		}

		// check sPid
		if (hyp->getID() != m_sID) {
			// sPid is for a different hypo
			snprintf(
					sLog, sizeof(sLog),
					"CHypo::trap: sPid %s Pick: %s linked to another hypo: %s",
					m_sID.c_str(), q->getID().c_str(), hyp->getID().c_str());
			glassutil::CLogit::log(glassutil::log_level::warn, sLog);
		}
	}
}

int CHypo::setCycleCount(int newCycle) {
	m_iCycleCount = newCycle;

	return (m_iCycleCount);
}

void CHypo::setNucleationDataThreshold(int cut) {
	m_iNucleationDataThreshold = cut;
}

void CHypo::setDistanceCutoffFactor(double cutFactor) {
	m_dDistanceCutoffFactor = cutFactor;
}

void CHypo::setMinDistanceCutoff(double cutMin) {
	m_dMinDistanceCutoff = cutMin;
}

void CHypo::setDistanceCutoffPercentage(double cutPercentage) {
	m_dDistanceCutoffPercentage = cutPercentage;
}

void CHypo::setCorrelationAdded(bool corrAdded) {
	m_bCorrelationAdded = corrAdded;
}

void CHypo::setFixed(bool fixed) {
	m_bFixed = fixed;
}

void CHypo::setGlass(CGlass* glass) {
	m_pGlass = glass;
}

void CHypo::setLatitude(double lat) {
	m_dLatitude = lat;
}

void CHypo::setLongitude(double lon) {
	// longitude wrap check
	if (lon > 180.0) {
		// lon is greater than 180
		m_dLongitude = lon - 360.0;
	} else if (lon < -180.0) {
		// lon is less than -180
		m_dLongitude = lon + 360.0;
	} else {
		m_dLongitude = lon;
	}
}

void CHypo::setDepth(double z) {
	m_dDepth = z;
}

void CHypo::setTOrigin(double newTOrg) {
	m_tOrigin = newTOrg;
}

void CHypo::setNucleationStackThreshold(double thresh) {
	m_dNucleationStackThreshold = thresh;
}

void CHypo::unlockAfterProcessing() {
	m_ProcessingMutex.unlock();
}

}  // namespace glasscore

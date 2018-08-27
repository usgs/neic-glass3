#include <json.h>
#include <string>
#include <utility>
#include <memory>
#include <cmath>
#include <vector>
#include "Date.h"
#include "Pid.h"
#include "Web.h"
#include "Node.h"
#include "PickList.h"
#include "HypoList.h"
#include "Hypo.h"
#include "Correlation.h"
#include "CorrelationList.h"
#include "Site.h"
#include "Glass.h"
#include "Logit.h"

#define RAD2DEG  57.29577951308
namespace glasscore {

// ---------------------------------------------------------CCorrelationList
CCorrelationList::CCorrelationList() {
	clear();
}

// ---------------------------------------------------------~CCorrelationList
CCorrelationList::~CCorrelationList() {
	clear();
}

// ---------------------------------------------------------clear
void CCorrelationList::clear() {
	std::lock_guard<std::recursive_mutex> corrListGuard(m_CorrelationListMutex);

	pGlass = NULL;
	pSiteList = NULL;

	// clear correlations
	clearCorrelations();
}

// ---------------------------------------------------------~clear
void CCorrelationList::clearCorrelations() {
	std::lock_guard<std::recursive_mutex> listGuard(m_vCorrelationMutex);

	// clear the vector and map
	vCorrelation.clear();
	mCorrelation.clear();

	// reset nCorrelation
	nCorrelationTotal = 0;
	nCorrelationMax = 10000;
}

// ---------------------------------------------------------dispatch
bool CCorrelationList::dispatch(std::shared_ptr<json::Object> com) {
	// null check json
	if (com == NULL) {
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"CCorrelationList::dispatch: NULL json communication.");
		return (false);
	}

	// check for a command
	if (com->HasKey("Cmd")
			&& ((*com)["Cmd"].GetType() == json::ValueType::StringVal)) {
		// dispatch to appropriate function based on Cmd value
		json::Value v = (*com)["Cmd"].ToString();

		// clear all data
		if (v == "ClearGlass") {
			// ClearGlass is also relevant to other glass
			// components, return false so they also get a
			// chance to process it
			return (false);
		}
	}

	// Input data can have Type keys
	if (com->HasKey("Type")
			&& ((*com)["Type"].GetType() == json::ValueType::StringVal)) {
		// dispatch to appropriate function based on Cmd value
		json::Value v = (*com)["Type"].ToString();

		// add a detection
		if (v == "Correlation") {
			return (addCorrelation(com));
		}
	}

	// this communication was not handled
	return (false);
}

// ---------------------------------------------------------addCorrelation
bool CCorrelationList::addCorrelation(
		std::shared_ptr<json::Object> correlation) {
	std::lock_guard<std::recursive_mutex> listGuard(m_vCorrelationMutex);

	// null check json
	if (correlation == NULL) {
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"CCorrelationList::addCorrelation: NULL json correlation.");
		return (false);
	}

	// null check pSiteList
	if (pSiteList == NULL) {
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"CCorrelationList::addCorrelation: NULL pSiteList.");
		return (false);
	}

	// check cmd or type
	if (correlation->HasKey("Type")
			&& ((*correlation)["Type"].GetType() == json::ValueType::StringVal)) {
		std::string type = (*correlation)["Type"].ToString();

		if (type != "Correlation") {
			glassutil::CLogit::log(
					glassutil::log_level::warn,
					"CCorrelationList::addCorrelation: Non-Correlation message "
					"passed in.");
			return (false);
		}
	} else {
		// no command or type
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"CCorrelationList::addCorrelation: Missing required Type Key.");
		return (false);
	}

	// create new correlation from json message
	CCorrelation * newCorrelation = new CCorrelation(correlation,
														nCorrelationTotal + 1,
														pSiteList);

	// check to see if we got a valid correlation
	if ((newCorrelation->getSite() == NULL)
			|| (newCorrelation->getTCorrelation() == 0)
			|| (newCorrelation->getPid() == "")) {
		// cleanup
		delete (newCorrelation);
		// message was processed
		return (true);
	}

	// check if correlation is duplicate, if pGlass exists
	if (pGlass) {
		bool duplicate = checkDuplicate(
				newCorrelation, pGlass->getCorrelationMatchingTWindow(),
				pGlass->getCorrelationMatchingXWindow());

		// it is a duplicate, log and don't add correlation
		if (duplicate) {
			glassutil::CLogit::log(
					glassutil::log_level::warn,
					"CCorrelationList::addCorrelation: Duplicate correlation "
					"not passed in.");
			delete (newCorrelation);
			// message was processed
			return (true);
		}
	}

	// create new shared pointer to this correlation
	std::shared_ptr<CCorrelation> corr(newCorrelation);

	// Add correlation to cache (mCorrelation) and time sorted
	// index (vCorrelation). If vCorrelation has reached its
	// maximum capacity (nCorrelationMax), then the
	// first correlation in the vector is removed and the
	// corresponding entry in the cache is erased.
	// It should be noted, that since what is cached
	// is a smart pointer, if it is currently part
	// of an active event, the actual correlation will not
	// be removed until either it is pruned from the
	// event or the event is completed and retired.
	nCorrelationTotal++;

	// get maximum number of correlations
	// use max correlations from pGlass if we have it
	if (pGlass) {
		nCorrelationMax = pGlass->getCorrelationMax();
	}

	// create pair for insertion
	std::pair<double, int> p(corr->getTCorrelation(), nCorrelationTotal);

	// check to see if we're at the correlation limit
	if (vCorrelation.size() == nCorrelationMax) {
		// find first correlation in vector
		std::pair<double, int> pdx;
		pdx = vCorrelation[0];
		auto pos = mCorrelation.find(pdx.second);

		// erase from map
		mCorrelation.erase(pos);

		// erase from vector
		vCorrelation.erase(vCorrelation.begin());
	}

	// Insert new correlation in proper time sequence into correlation vector
	// get the index of the new correlation
	int iCorrelation = indexCorrelation(corr->getTCorrelation());
	switch (iCorrelation) {
		case -2:
			// Empty vector, just add it
			vCorrelation.push_back(p);
			break;
		case -1:
			// Pick is before any others, insert at beginning
			vCorrelation.insert(vCorrelation.begin(), p);
			break;
		default:
			// correlation is somewhere in vector
			if (iCorrelation == vCorrelation.size() - 1) {
				// correlation is after all correlations, add to end
				vCorrelation.push_back(p);
			} else {
				// find where the correlation should be inserted
				auto it = std::next(vCorrelation.begin(), iCorrelation + 1);

				// insert at that location
				vCorrelation.insert(it, p);
			}
			break;
	}

	// add to correlation map
	mCorrelation[nCorrelationTotal] = corr;

	// make sure we have a pGlass and pGlass->pHypoList
	if ((pGlass) && (pGlass->getHypoList())) {
		// Attempt association of the new correlation.  If that fails create a
		// new hypo from the correlation
		if (!pGlass->getHypoList()->associate(corr)) {
			// not associated, we need to create a new hypo
			// NOTE: maybe move below to CCorrelation function to match pick?

			// correlations don't have a second travel time
			std::shared_ptr<traveltime::CTravelTime> nullTrav;

			// create new hypo
			std::shared_ptr<CHypo> hypo = std::make_shared<CHypo>(
					corr, pGlass->getTrvDefault(), nullTrav,
					pGlass->getTTT());

			// set hypo glass pointer and such
			hypo->setGlass(pGlass);
			hypo->setCutFactor(pGlass->getCutFactor());
			hypo->setCutPercentage(pGlass->getCutPercentage());
			hypo->setCutMin(pGlass->getCutMin());
			hypo->setCut(pGlass->getNucleate());
			hypo->setThresh(pGlass->getThresh());

			// add correlation to hypo
			hypo->addCorrelation(corr);

			// link the correlation to the hypo
			corr->addHypo(hypo);

			// Add other data to this hypo
			// Search for any associable picks that match hypo in the pick list
			// choosing to not localize after because we trust the correlation
			// location for this step
			pGlass->getPickList()->scavenge(hypo);

			// search for any associable correlations that match hypo in the
			// correlation list choosing to not localize after because we trust
			// the correlation location for this step
			scavenge(hypo);

			// ensure all data scavanged belong to hypo choosing to not localize
			// after because we trust  the correlation location for this step
			pGlass->getHypoList()->resolve(hypo);

			// add hypo to hypo list
			pGlass->getHypoList()->addHypo(hypo);

			// schedule it for processing
			pGlass->getHypoList()->pushFifo(hypo);
		}
	}

	// we're done, message was processed
	return (true);
}

// ---------------------------------------------------------indexCorrelation
int CCorrelationList::indexCorrelation(double tCorrelation) {
	std::lock_guard<std::recursive_mutex> listGuard(m_vCorrelationMutex);

	// handle empty vector case
	if (vCorrelation.size() == 0) {
		// return -2 to indicate empty vector
		return (-2);
	}

	// get the time of the first correlation in the list
	double tFirstCorrelation = vCorrelation[0].first;

	// handle correlation earlier than first element case
	// time is earlier than first correlation
	if (tCorrelation < tFirstCorrelation) {
		// return -1 to indicate earlier than first element
		return (-1);
	}

	// handle case that the correlation is later than last element
	int i1 = 0;
	int i2 = vCorrelation.size() - 1;
	double tLastCorrelation = vCorrelation[i2].first;

	// time is after last correlation
	if (tCorrelation >= tLastCorrelation) {
		// return index of last element
		return (i2);
	}

	// search for insertion point within vector
	// using a binary search
	// while upper minus lower bounds is greater than one
	while ((i2 - i1) > 1) {
		// compute current correlation index
		int ix = (i1 + i2) / 2;

		// get current correlation time
		double tCurrentCorrelation = vCorrelation[ix].first;

		// if time is before current correlation
		if (tCurrentCorrelation > tCorrelation) {
			// new upper bound is this index
			i2 = ix;
		} else {  // if (tCurrentCorrelation <= tCorrelation)
			// if time is after or equal to current correlation
			// new lower bound is this index
			i1 = ix;
		}
	}

	// return the last lower bound as the insertion point
	return (i1);
}

// ---------------------------------------------------------getPick
std::shared_ptr<CCorrelation> CCorrelationList::getCorrelation(
		int idCorrelation) {
	std::lock_guard<std::recursive_mutex> listGuard(m_vCorrelationMutex);

	// try to find that id in map
	auto pos = mCorrelation.find(idCorrelation);

	// make sure that we found something
	if (pos != mCorrelation.end()) {
		// return the correlation
		return (pos->second);
	}

	// found nothing
	return (NULL);
}

// -----------------------------------------------------checkDuplicate
bool CCorrelationList::checkDuplicate(CCorrelation * newCorrelation,
										double tWindow, double xWindow) {
	std::lock_guard<std::recursive_mutex> listGuard(m_vCorrelationMutex);

	// null checks
	if (newCorrelation == NULL) {
		return (false);
	}
	if (tWindow <= 0.0) {
		return (false);
	}
	if (xWindow <= 0.0) {
		return (false);
	}

	// get the index of the earliest possible match
	int it1 = indexCorrelation(newCorrelation->getTCorrelation() - tWindow);

	// get index of the latest possible correlation
	int it2 = indexCorrelation(newCorrelation->getTCorrelation() + tWindow);

	// index can't be negative, it1/2 negative if correlation before first in
	// list
	if (it1 < 0) {
		it1 = 0;
	}
	if (it2 < 0) {
		it2 = 0;
	}

	// don't bother if there's no correlatinos
	if (it1 == it2) {
		return (false);
	}

	// loop through possible matching correlations
	for (int it = it1; it <= it2; it++) {
		auto q = vCorrelation[it];
		std::shared_ptr<CCorrelation> cor = mCorrelation[q.second];

		// check if time difference is within window
		if (std::abs(newCorrelation->getTCorrelation() - cor->getTCorrelation())
				< tWindow) {
			// check if sites match
			if (newCorrelation->getSite()->getScnl()
					== cor->getSite()->getScnl()) {
				glassutil::CGeo geo1;
				geo1.setGeographic(newCorrelation->getLat(),
									newCorrelation->getLon(),
									newCorrelation->getZ());
				glassutil::CGeo geo2;
				geo2.setGeographic(cor->getLat(), cor->getLon(), cor->getZ());
				double delta = RAD2DEG * geo1.delta(&geo2);

				// check if distance difference is within window
				if (delta < xWindow) {
					// if match is found, log, and return
					glassutil::CLogit::log(
							glassutil::log_level::warn,
							"CCorrelationList::checkDuplicate: Duplicate "
									"(tWindow = " + std::to_string(tWindow)
									+ ", xWindow = " + std::to_string(xWindow)
									+ ") : old:" + cor->getSite()->getScnl()
									+ " "
									+ std::to_string(cor->getTCorrelation())
									+ " new(del):"
									+ newCorrelation->getSite()->getScnl() + " "
									+ std::to_string(
											newCorrelation->getTCorrelation()));
					return (true);
				}
			}
		}
	}

	// no match
	return (false);
}

// ---------------------------------------------------------scavenge
bool CCorrelationList::scavenge(std::shared_ptr<CHypo> hyp, double tDuration) {
	std::lock_guard<std::recursive_mutex> listGuard(m_vCorrelationMutex);

	// Scan all correlations within specified time range, adding any
	// that meet association criteria to hypo object provided.
	// Returns true if any associated.
	// null check
	if (hyp == NULL) {
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"CCorrelationList::scavenge: NULL CHypo provided.");
		return (false);
	}

	// check pGlass
	if (pGlass == NULL) {
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"CCorrelationList::scavenge: NULL glass pointer.");
		return (false);
	}

	glassutil::CLogit::log(glassutil::log_level::debug,
							"CCorrelationList::scavenge. " + hyp->getPid());

	// get the index of the correlation to start with
	// based on the hypo origin time
	int it1 = indexCorrelation(hyp->getTOrg() - tDuration);

	// get the index of the correlation to end with by using the hypo
	// origin time plus the provided duration
	int it2 = indexCorrelation(hyp->getTOrg() + tDuration);

	// index can't be negative
	// Primarily occurs if origin time is before first correlation
	if (it1 < 0) {
		it1 = 0;
	}
	if (it2 < 0) {
		it2 = 0;
	}

	// don't bother if there's no correlations
	if (it1 == it2) {
		return (false);
	}

	// for each correlation index between it1 and it2
	bool bAss = false;
	for (int it = it1; it < it2; it++) {
		// get the correlation from the vector
		auto q = vCorrelation[it];
		std::shared_ptr<CCorrelation> corr = mCorrelation[q.second];
		std::shared_ptr<CHypo> corrHyp = corr->getHypo();

		// check to see if this correlation is already in this hypo
		if (hyp->hasCorrelation(corr)) {
			// it is, skip it
			continue;
		}

		// check to see if this correlation can be associated with this hypo
		if (!hyp->associate(corr, pGlass->getCorrelationMatchingTWindow(),
							pGlass->getCorrelationMatchingXWindow())) {
			// it can't, skip it
			continue;
		}

		// check to see if this correlation is part of ANY hypo
		if (corrHyp == NULL) {
			// unassociated with any existing hypo
			// link correlation to the hypo we're working on
			corr->addHypo(hyp, "W", true);

			// add correlation to this hypo
			hyp->addCorrelation(corr);

			// we've associated a correlation
			bAss = true;
		} else {
			// associated with an existing hypo
			// Add it to this hypo, but don't change the correlation's hypo link
			// Let resolve() sort out which hypo the correlation fits best with
			hyp->addCorrelation(corr);

			// we've associated a correlation
			bAss = true;
		}
	}

	// return whether we've associated at least one correlation
	return (bAss);
}

// ---------------------------------------------------------rogues
std::vector<std::shared_ptr<CCorrelation>> CCorrelationList::rogues(
		std::string pidHyp, double tOrg, double tDuration) {
	std::lock_guard<std::recursive_mutex> listGuard(m_vCorrelationMutex);

	std::vector<std::shared_ptr<CCorrelation>> vRogue;

	// Generate rogue list (all correlations that are not associated
	// with given event, but could be)
	// null checks
	if (pidHyp == "") {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CPickList::rogues: Empty pidHyp provided.");
		return (vRogue);
	}

	if (tOrg <= 0) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CPickList::rogues: Invalid tOrg provided.");
		return (vRogue);
	}

	if (tDuration <= 0) {
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"CPickList::rogues: Invalid tDuration provided.");
		return (vRogue);
	}

	// get the index of the pick to start with
	// based on the provided origin time
	int it1 = indexCorrelation(tOrg);

	// index can't be negative
	// Primarily occurs if origin time is before first pick
	if (it1 < 0) {
		it1 = 0;
	}

	// get the index of the pick to end with by using the provided
	// origin time plus the provided duration
	int it2 = indexCorrelation(tOrg + tDuration);

	// for each pick index between it1 and it2
	for (int it = it1; it < it2; it++) {
		// get the current pick from the vector
		auto q = vCorrelation[it];
		std::shared_ptr<CCorrelation> corr = mCorrelation[q.second];
		std::shared_ptr<CHypo> corrHyp = corr->getHypo();

		// if the current pick is associated to this event
		if ((corrHyp != NULL) && (corrHyp->getPid() == pidHyp)) {
			// skip to next pick
			continue;
		}

		// Add current pick to rogues list
		vRogue.push_back(corr);
	}

	return (vRogue);
}

const CSiteList* CCorrelationList::getSiteList() const {
	std::lock_guard<std::recursive_mutex> corrListGuard(m_CorrelationListMutex);
	return (pSiteList);
}

void CCorrelationList::setSiteList(CSiteList* siteList) {
	std::lock_guard<std::recursive_mutex> corrListGuard(m_CorrelationListMutex);
	pSiteList = siteList;
}

const CGlass* CCorrelationList::getGlass() const {
	std::lock_guard<std::recursive_mutex> corrListGuard(m_CorrelationListMutex);
	return (pGlass);
}

void CCorrelationList::setGlass(CGlass* glass) {
	std::lock_guard<std::recursive_mutex> corrListGuard(m_CorrelationListMutex);
	pGlass = glass;
}

int CCorrelationList::getNCorrelationMax() const {
	return (nCorrelationMax);
}

void CCorrelationList::setNCorrelationMax(int correlationMax) {
	nCorrelationMax = correlationMax;
}

int CCorrelationList::getNCorrelationTotal() const {
	return (nCorrelationTotal);
}

int CCorrelationList::getVCorrelationSize() const {
	std::lock_guard<std::recursive_mutex> vCorrelationGuard(
			m_vCorrelationMutex);
	return (vCorrelation.size());
}

}  // namespace glasscore

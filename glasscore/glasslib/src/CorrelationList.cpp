#include <json.h>
#include <string>
#include <utility>
#include <memory>
#include <cmath>
#include <set>
#include <algorithm>
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

	m_pGlass = NULL;
	m_pSiteList = NULL;

	// clear correlations
	clearCorrelations();
}

// ---------------------------------------------------------clearCorrelations
void CCorrelationList::clearCorrelations() {
	std::lock_guard<std::recursive_mutex> listGuard(m_CorrelationListMutex);

	// clear the multiset
	m_msCorrelationList.clear();

	// reset nCorrelation
	m_iCorrelationTotal = 0;
	m_iCorrelationMax = 10000;
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
			return (addCorrelationFromJSON(com));
		}
	}

	// this communication was not handled
	return (false);
}

// -------------------------------------------------------addCorrelationFromJSON
bool CCorrelationList::addCorrelationFromJSON(
		std::shared_ptr<json::Object> correlation) {
	std::lock_guard<std::recursive_mutex> listGuard(m_CorrelationListMutex);

	// null check json
	if (correlation == NULL) {
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"CCorrelationList::addCorrelation: NULL json correlation.");
		return (false);
	}

	// null check pSiteList
	if (m_pSiteList == NULL) {
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
														m_iCorrelationTotal + 1,
														m_pSiteList);

	// check to see if we got a valid correlation
	if ((newCorrelation->getSite() == NULL)
			|| (newCorrelation->getTCorrelation() == 0)
			|| (newCorrelation->getID() == "")) {
		// cleanup
		delete (newCorrelation);
		// message was processed
		return (true);
	}

	// check if correlation is duplicate, if pGlass exists
	if (m_pGlass) {
		bool duplicate = checkDuplicate(
				newCorrelation, m_pGlass->getCorrelationMatchingTimeWindow(),
				m_pGlass->getCorrelationMatchingDistanceWindow());

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

	m_iCorrelationTotal++;

	// get maximum number of correlations
	// use max correlations from pGlass if we have it
	if (m_pGlass) {
		m_iCorrelationMax = m_pGlass->getMaxNumCorrelations();
	}

	// create pair for insertion
	std::pair<double, int> p(corr->getTCorrelation(), m_iCorrelationTotal);

	// check to see if we're at the correlation limit
	if (m_msCorrelationList.size() == m_iCorrelationMax) {
		std::multiset<std::shared_ptr<CCorrelation>, CorrelationCompare>::iterator oldest = // NOLINT
				m_msCorrelationList.begin();

		// find first pick in multiset
		std::shared_ptr<CCorrelation> oldestCorrelation = *oldest;

		// remove from from multiset
		m_msCorrelationList.erase(oldest);
	}

	// add to multiset
	m_msCorrelationList.insert(corr);

	// make sure we have a pGlass and pGlass->pHypoList
	if ((m_pGlass) && (m_pGlass->getHypoList())) {
		// Attempt association of the new correlation.  If that fails create a
		// new hypo from the correlation
		if (!m_pGlass->getHypoList()->associate(corr)) {
			// not associated, we need to create a new hypo
			// NOTE: maybe move below to CCorrelation function to match pick?

			// correlations don't have a second travel time
			std::shared_ptr<traveltime::CTravelTime> nullTrav;

			// create new hypo
			std::shared_ptr<CHypo> hypo = std::make_shared<CHypo>(
					corr, m_pGlass->getDefaultNucleationTravelTime(), nullTrav,
					m_pGlass->getAssociationTravelTimes());

			// set hypo glass pointer and such
			hypo->setGlass(m_pGlass);
			hypo->setDistanceCutoffFactor(m_pGlass->getDistanceCutoffFactor());
			hypo->setDistanceCutoffPercentage(
					m_pGlass->getDistanceCutoffPercentage());
			hypo->setMinDistanceCutoff(m_pGlass->getMinDistanceCutoff());
			hypo->setNucleationDataThreshold(
					m_pGlass->getNucleationDataThreshold());
			hypo->setNucleationStackThreshold(
					m_pGlass->getNucleationStackThreshold());

			// add correlation to hypo
			hypo->addCorrelation(corr);

			// link the correlation to the hypo
			corr->addHypo(hypo);

			// Add other data to this hypo
			// Search for any associable picks that match hypo in the pick list
			// choosing to not localize after because we trust the correlation
			// location for this step
			m_pGlass->getPickList()->scavenge(hypo);

			// search for any associable correlations that match hypo in the
			// correlation list choosing to not localize after because we trust
			// the correlation location for this step
			scavenge(hypo);

			// ensure all data scavanged belong to hypo choosing to not localize
			// after because we trust  the correlation location for this step
			m_pGlass->getHypoList()->resolve(hypo);

			// add hypo to hypo list
			m_pGlass->getHypoList()->addHypo(hypo);

			// schedule it for processing
			m_pGlass->getHypoList()->addHypoToProcess(hypo);
		}
	}

	// we're done, message was processed
	return (true);
}

// -----------------------------------------------------getCorrelation
bool CCorrelationList::checkDuplicate(CCorrelation * newCorrelation,
										double tWindow, double xWindow) {
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

	std::shared_ptr<CSite> nullSite;

	// construct the lower bound value. std::multiset requires
	// that this be in the form of a std::shared_ptr<CCorrelation>
	std::shared_ptr<CCorrelation> lowerValue = std::make_shared<CCorrelation>(
			nullSite, (newCorrelation->getTCorrelation() - tWindow), 0, "", "", 0,
			0, 0, 0, 0);

	// construct the upper bound value. std::multiset requires
	// that this be in the form of a std::shared_ptr<CCorrelation>
	std::shared_ptr<CCorrelation> upperValue = std::make_shared<CCorrelation>(
			nullSite, (newCorrelation->getTCorrelation() + tWindow), 0, "", "", 0,
			0, 0, 0, 0);

	// lock while we're searching the list
	std::lock_guard<std::recursive_mutex> listGuard(m_CorrelationListMutex);

	if (m_msCorrelationList.size() == 0) {
		return (false);
	}

	// get the bounds for this window
	std::multiset<std::shared_ptr<CCorrelation>, CorrelationCompare>::iterator lower = // NOLINT
			std::lower_bound(m_msCorrelationList.begin(),
								m_msCorrelationList.end(), lowerValue);
	std::multiset<std::shared_ptr<CCorrelation>, PickCompare>::iterator upper =
			std::upper_bound(m_msCorrelationList.begin(),
								m_msCorrelationList.end(), upperValue);

	// don't bother if there's no picks in the window
	if (lower == upper) {
		return (false);
	}

	// loop through possible matching picks
	for (std::multiset<std::shared_ptr<CCorrelation>, CorrelationCompare>::iterator it = // NOLINT
			lower; ((it != upper) && (it != m_msCorrelationList.end())); ++it) {
		std::shared_ptr<CCorrelation> cor = *it;
		// check if time difference is within window
		if (std::abs(newCorrelation->getTCorrelation() - cor->getTCorrelation())
				< tWindow) {
			// check if sites match
			if (newCorrelation->getSite()->getSCNL()
					== cor->getSite()->getSCNL()) {
				glassutil::CGeo geo1;
				geo1.setGeographic(newCorrelation->getLatitude(),
									newCorrelation->getLongitude(),
									newCorrelation->getDepth());
				glassutil::CGeo geo2;
				geo2.setGeographic(cor->getLatitude(), cor->getLongitude(),
									cor->getDepth());
				double delta = RAD2DEG * geo1.delta(&geo2);

				// check if distance difference is within window
				if (delta < xWindow) {
					// if match is found, log, and return
					glassutil::CLogit::log(
							glassutil::log_level::warn,
							"CCorrelationList::checkDuplicate: Duplicate "
									"(tWindow = " + std::to_string(tWindow)
									+ ", xWindow = " + std::to_string(xWindow)
									+ ") : old:" + cor->getSite()->getSCNL()
									+ " "
									+ std::to_string(cor->getTCorrelation())
									+ " new(del):"
									+ newCorrelation->getSite()->getSCNL() + " "
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
bool CCorrelationList::scavenge(std::shared_ptr<CHypo> hyp, double tWindow) {
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
	if (m_pGlass == NULL) {
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"CCorrelationList::scavenge: NULL glass pointer.");
		return (false);
	}
	bool associated = false;

	glassutil::CLogit::log(glassutil::log_level::debug,
							"CCorrelationList::scavenge. " + hyp->getID());

	std::shared_ptr<CSite> nullSite;

	// construct the lower bound value. std::multiset requires
	// that this be in the form of a std::shared_ptr<CCorrelation>
	std::shared_ptr<CCorrelation> lowerValue = std::make_shared<CCorrelation>(
			nullSite, (hyp->getTOrigin() - tWindow), 0, "", "", 0, 0, 0, 0, 0);

	// construct the upper bound value. std::multiset requires
	// that this be in the form of a std::shared_ptr<CCorrelation>
	std::shared_ptr<CCorrelation> upperValue = std::make_shared<CCorrelation>(
			nullSite, (hyp->getTOrigin() + tWindow), 0, "", "", 0, 0, 0, 0, 0);

	// lock while we're searching the list
	std::lock_guard<std::recursive_mutex> listGuard(m_CorrelationListMutex);

	if (m_msCorrelationList.size() == 0) {
		return (false);
	}

	// get the bounds for this window
	std::multiset<std::shared_ptr<CCorrelation>, CorrelationCompare>::iterator lower = // NOLINT
			std::lower_bound(m_msCorrelationList.begin(),
								m_msCorrelationList.end(), lowerValue);
	std::multiset<std::shared_ptr<CCorrelation>, PickCompare>::iterator upper =
			std::upper_bound(m_msCorrelationList.begin(),
								m_msCorrelationList.end(), upperValue);

	// don't bother if there's no picks in the window
	if (lower == upper) {
		return (false);
	}

	// loop through possible matching picks
	for (std::multiset<std::shared_ptr<CCorrelation>, CorrelationCompare>::iterator it = // NOLINT
			lower; ((it != upper) && (it != m_msCorrelationList.end())); ++it) {
		std::shared_ptr<CCorrelation> corr = *it;
		std::shared_ptr<CHypo> corrHyp = corr->getHypo();

		// check to see if this correlation is already in this hypo
		if (hyp->hasCorrelation(corr)) {
			// it is, skip it
			continue;
		}

		// check to see if this correlation can be associated with this hypo
		if (!hyp->associate(corr, m_pGlass->getCorrelationMatchingTimeWindow(),
							m_pGlass->getCorrelationMatchingDistanceWindow())) {
			// it can't, skip it
			continue;
		}

		// check to see if this correlation is part of ANY hypo
		if (corrHyp == NULL) {
			// unassociated with any existing hypo
			// link correlation to the hypo we're working on
			corr->addHypo(hyp, true);

			// add correlation to this hypo
			hyp->addCorrelation(corr);

			// we've associated a correlation
			associated = true;
		} else {
			// associated with an existing hypo
			// Add it to this hypo, but don't change the correlation's hypo link
			// Let resolve() sort out which hypo the correlation fits best with
			hyp->addCorrelation(corr);

			// we've associated a correlation
			associated = true;
		}
	}

	// return whether we've associated at least one correlation
	return (associated);
}

// ---------------------------------------------------------getSiteList
const CSiteList* CCorrelationList::getSiteList() const {
	std::lock_guard<std::recursive_mutex> corrListGuard(m_CorrelationListMutex);
	return (m_pSiteList);
}

// ---------------------------------------------------------setSiteList
void CCorrelationList::setSiteList(CSiteList* siteList) {
	std::lock_guard<std::recursive_mutex> corrListGuard(m_CorrelationListMutex);
	m_pSiteList = siteList;
}

// ---------------------------------------------------------getGlass
const CGlass* CCorrelationList::getGlass() const {
	std::lock_guard<std::recursive_mutex> corrListGuard(m_CorrelationListMutex);
	return (m_pGlass);
}

// ---------------------------------------------------------setGlass
void CCorrelationList::setGlass(CGlass* glass) {
	std::lock_guard<std::recursive_mutex> corrListGuard(m_CorrelationListMutex);
	m_pGlass = glass;
}

// ---------------------------------------------------------getCorrelationMax
int CCorrelationList::getCorrelationMax() const {
	return (m_iCorrelationMax);
}

// ---------------------------------------------------------setCorrelationMax
void CCorrelationList::setCorrelationMax(int correlationMax) {
	m_iCorrelationMax = correlationMax;
}

// ---------------------------------------------------------getCorrelationTotal
int CCorrelationList::getCorrelationTotal() const {
	return (m_iCorrelationTotal);
}

// ---------------------------------------------------------size
int CCorrelationList::size() const {
	std::lock_guard<std::recursive_mutex> vCorrelationGuard(
			m_CorrelationListMutex);
	return (m_msCorrelationList.size());
}

}  // namespace glasscore

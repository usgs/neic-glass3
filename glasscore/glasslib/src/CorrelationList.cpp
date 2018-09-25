#include "CorrelationList.h"
#include <json.h>
#include <date.h>
#include <logger.h>
#include <geo.h>
#include <string>
#include <utility>
#include <memory>
#include <cmath>
#include <set>
#include <vector>
#include <algorithm>
#include "Web.h"
#include "Node.h"
#include "PickList.h"
#include "HypoList.h"
#include "Hypo.h"
#include "Correlation.h"
#include "Site.h"
#include "Glass.h"

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

	m_pSiteList = NULL;

	// clear the multiset
	m_msCorrelationList.clear();

	// reset nCorrelation
	m_iCountOfTotalCorrelationsProcessed = 0;
	m_iMaxAllowableCorrelationCount = 10000;
}

// -------------------------------------------------------receiveExternalMessage
bool CCorrelationList::receiveExternalMessage(
		std::shared_ptr<json::Object> com) {
	// null check json
	if (com == NULL) {
		glass3::util::Logger::log(
				"error",
				"CCorrelationList::receiveExternalMessage: NULL json communication.");
		return (false);
	}

	// Input data can have Type keys
	if (com->HasKey("Type")
			&& ((*com)["Type"].GetType() == json::ValueType::StringVal)) {
		// dispatch to appropriate function based on Cmd value
		json::Value v = (*com)["Type"].ToString();

		// add a correlation
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
		glass3::util::Logger::log(
				"error",
				"CCorrelationList::addCorrelationFromJSON: NULL json correlation.");
		return (false);
	}

	// null check pSiteList
	if (m_pSiteList == NULL) {
		glass3::util::Logger::log(
				"error",
				"CCorrelationList::addCorrelationFromJSON: NULL m_pSiteList.");
		return (false);
	}

	// check cmd or type
	if (correlation->HasKey("Type")
			&& ((*correlation)["Type"].GetType() == json::ValueType::StringVal)) {
		std::string type = (*correlation)["Type"].ToString();

		if (type != "Correlation") {
			glass3::util::Logger::log(
					"warning",
					"CCorrelationList::addCorrelationFromJSON: Non-Correlation "
					"message passed in.");
			return (false);
		}
	} else {
		// no command or type
		glass3::util::Logger::log(
				"error",
				"CCorrelationList::addCorrelationFromJSON: Missing required Type "
				"Key.");
		return (false);
	}

	// create new correlation from json message
	CCorrelation * newCorrelation = new CCorrelation(correlation, m_pSiteList);

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
	bool duplicate = checkDuplicate(
			newCorrelation, CGlass::getCorrelationMatchingTimeWindow(),
			CGlass::getCorrelationMatchingDistanceWindow());

	// it is a duplicate, log and don't add correlation
	if (duplicate) {
		glass3::util::Logger::log(
				"warning",
				"CCorrelationList::addCorrelationFromJSON: Duplicate correlation "
				"not passed in.");
		delete (newCorrelation);
		// message was processed
		return (true);
	}

	// create new shared pointer to this correlation
	std::shared_ptr<CCorrelation> corr(newCorrelation);

	m_iCountOfTotalCorrelationsProcessed++;

	// get maximum number of correlations
	// use max correlations from CGlass if we have it
	if (CGlass::getMaxNumCorrelations() > 0) {
		m_iMaxAllowableCorrelationCount = CGlass::getMaxNumCorrelations();
	}

	// check to see if we're at the correlation limit
	while (m_msCorrelationList.size() >= m_iMaxAllowableCorrelationCount) {
		std::multiset<std::shared_ptr<CCorrelation>, CorrelationCompare>::iterator oldest =  // NOLINT
				m_msCorrelationList.begin();

		// find first pick in multiset
		std::shared_ptr<CCorrelation> oldestCorrelation = *oldest;

		// remove from from multiset
		m_msCorrelationList.erase(oldest);
	}

	// add to multiset
	m_msCorrelationList.insert(corr);

	// make sure we have a pGlass and pGlass->pHypoList
	if (CGlass::getHypoList()) {
		// Attempt association of the new correlation.  If that fails create a
		// new hypo from the correlation
		if (!CGlass::getHypoList()->associateData(corr)) {
			// not associated, we need to create a new hypo
			// NOTE: maybe move below to CCorrelation function to match pick?

			// correlations don't have a second travel time
			std::shared_ptr<traveltime::CTravelTime> nullTrav;

			// create new hypo
			std::shared_ptr<CHypo> hypo = std::make_shared<CHypo>(
					corr, CGlass::getDefaultNucleationTravelTime(), nullTrav,
					CGlass::getAssociationTravelTimes());

			// set thresholds
			hypo->setNucleationDataThreshold(
					CGlass::getNucleationDataThreshold());
			hypo->setNucleationStackThreshold(
					CGlass::getNucleationStackThreshold());

			// add correlation to hypo
			hypo->addCorrelationReference(corr);

			// link the correlation to the hypo
			corr->addHypoReference(hypo);

			// Add other data to this hypo
			// Search for any associable picks that match hypo in the pick list
			// choosing to not localize after because we trust the correlation
			// location for this step
			CGlass::getPickList()->scavenge(hypo);

			// search for any associable correlations that match hypo in the
			// correlation list choosing to not localize after because we trust
			// the correlation location for this step
			scavenge(hypo);

			// ensure all data scavanged belong to hypo choosing to not localize
			// after because we trust  the correlation location for this step
			CGlass::getHypoList()->resolveData(hypo);

			// add hypo to hypo list
			CGlass::getHypoList()->addHypo(hypo);

			// schedule it for processing
			CGlass::getHypoList()->appendToHypoProcessingQueue(hypo);
		}
	}

	// we're done, message was processed
	return (true);
}

// -----------------------------------------------------getCorrelations
std::vector<std::weak_ptr<CCorrelation>> CCorrelationList::getCorrelations(
		double t1, double t2) {
	std::vector<std::weak_ptr<CCorrelation>> correlations;

	if (t1 == t2) {
		return (correlations);
	}
	if (t1 > t2) {
		double temp = t2;
		t2 = t1;
		t1 = temp;
	}

	std::shared_ptr<CSite> nullSite;

	// construct the lower bound value. std::multiset requires
	// that this be in the form of a std::shared_ptr<CPick>
	std::shared_ptr<CCorrelation> lowerValue = std::make_shared<CCorrelation>(
			nullSite, t1, "", "", 0, 0, 0, 0, 0);

	// construct the upper bound value. std::multiset requires
	// that this be in the form of a std::shared_ptr<CPick>
	std::shared_ptr<CCorrelation> upperValue = std::make_shared<CCorrelation>(
			nullSite, t2, "", "", 0, 0, 0, 0, 0);

	std::lock_guard<std::recursive_mutex> listGuard(m_CorrelationListMutex);

	// don't bother if the list is empty
	if (m_msCorrelationList.size() == 0) {
		return (correlations);
	}

	// get the bounds for this window
	std::multiset<std::shared_ptr<CCorrelation>, CorrelationCompare>::iterator lower =  // NOLINT
			m_msCorrelationList.lower_bound(lowerValue);
	std::multiset<std::shared_ptr<CCorrelation>, CorrelationCompare>::iterator upper =  // NOLINT
			m_msCorrelationList.upper_bound(upperValue);

	// found nothing
	if (lower == m_msCorrelationList.end()) {
		return (correlations);
	}

	// found one
	if ((lower == upper) && (lower != m_msCorrelationList.end())) {
		std::shared_ptr<CCorrelation> aCorrelation = *lower;

		if (aCorrelation != NULL) {
			std::weak_ptr<CCorrelation> awCorrelation = aCorrelation;

			// add to the list of corrleations
			correlations.push_back(awCorrelation);
		}
		return (correlations);
	}

	// loop through found picks
	for (std::multiset<std::shared_ptr<CCorrelation>, CorrelationCompare>::iterator it =  // NOLINT
			lower; ((it != upper) && (it != m_msCorrelationList.end())); ++it) {
		std::shared_ptr<CCorrelation> aCorrelation = *it;

		if (aCorrelation != NULL) {
			std::weak_ptr<CCorrelation> awCorrelation = *it;

			// add to the list of hypos
			correlations.push_back(awCorrelation);
		}
	}

	// return the list of picks we found
	return (correlations);
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

	std::vector<std::weak_ptr<CCorrelation>> correlations = getCorrelations(
			newCorrelation->getTCorrelation() - tWindow,
			newCorrelation->getTCorrelation() + tWindow);

	if (correlations.size() == 0) {
		return (false);
	}

	// loop through possible matching correlations
	for (int i = 0; i < correlations.size(); i++) {
		// make sure pick is still valid before checking
		if (std::shared_ptr<CCorrelation> currentCorrelation = correlations[i]
				.lock()) {
			// check if time difference is within window
			if (std::abs(
					newCorrelation->getTCorrelation()
							- currentCorrelation->getTCorrelation())
					< tWindow) {
				// check if sites match
				if (newCorrelation->getSite()->getSCNL()
						== currentCorrelation->getSite()->getSCNL()) {
					glass3::util::Geo geo1;
					geo1.setGeographic(newCorrelation->getLatitude(),
										newCorrelation->getLongitude(),
										newCorrelation->getDepth());
					glass3::util::Geo geo2;
					geo2.setGeographic(currentCorrelation->getLatitude(),
										currentCorrelation->getLongitude(),
										currentCorrelation->getDepth());
					double delta = RAD2DEG * geo1.delta(&geo2);

					// check if distance difference is within window
					if (delta < xWindow) {
						// if match is found, log, and return
						glass3::util::Logger::log(
								"warning",
								"CCorrelationList::checkDuplicate: Duplicate "
										"(tWindow = " + std::to_string(tWindow)
										+ ", xWindow = "
										+ std::to_string(xWindow) + ") : old:"
										+ currentCorrelation->getSite()->getSCNL()
										+ " "
										+ std::to_string(
												currentCorrelation
														->getTCorrelation())
										+ " new(del):"
										+ newCorrelation->getSite()->getSCNL()
										+ " "
										+ std::to_string(
												newCorrelation->getTCorrelation()));
						return (true);
					}
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
		glass3::util::Logger::log(
				"error", "CCorrelationList::scavenge: NULL CHypo provided.");
		return (false);
	}

	glass3::util::Logger::log("debug",
								"CCorrelationList::scavenge. " + hyp->getID());

	bool associated = false;

	std::vector<std::weak_ptr<CCorrelation>> correlations = getCorrelations(
			hyp->getTOrigin() - tWindow, hyp->getTOrigin() + tWindow);

	if (correlations.size() == 0) {
		return (false);
	}

	// loop through possible matching correlations
	for (int i = 0; i < correlations.size(); i++) {
		// make sure pick is still valid before checking
		if (std::shared_ptr<CCorrelation> currentCorrelation = correlations[i]
				.lock()) {
			std::shared_ptr<CHypo> corrHyp = currentCorrelation
					->getHypoReference();

			// check to see if this correlation is already in this hypo
			if (hyp->hasCorrelationReference(currentCorrelation)) {
				// it is, skip it
				continue;
			}

			// check to see if this correlation can be associated with this hypo
			if (!hyp->canAssociate(
					currentCorrelation,
					CGlass::getCorrelationMatchingTimeWindow(),
					CGlass::getCorrelationMatchingDistanceWindow())) {
				// it can't, skip it
				continue;
			}

			// check to see if this correlation is part of ANY hypo
			if (corrHyp == NULL) {
				// unassociated with any existing hypo
				// link correlation to the hypo we're working on
				currentCorrelation->addHypoReference(hyp, true);

				// add correlation to this hypo
				hyp->addCorrelationReference(currentCorrelation);

				// we've associated a correlation
				associated = true;
			} else {
				// associated with an existing hypo
				// Add it to this hypo, but don't change the correlation's hypo link
				// Let resolve() sort out which hypo the correlation fits best with
				hyp->addCorrelationReference(currentCorrelation);

				// we've associated a correlation
				associated = true;
			}
		}
	}

	// return whether we've associated at least one correlation
	return (associated);
}

// ---------------------------------------------------------getCorrelationMax
int CCorrelationList::getMaxAllowableCorrelationCount() const {
	return (m_iMaxAllowableCorrelationCount);
}

// ---------------------------------------------------------setCorrelationMax
void CCorrelationList::setMaxAllowableCorrelationCount(int correlationMax) {
	m_iMaxAllowableCorrelationCount = correlationMax;
}

// ---------------------------------------------------------getCorrelationTotal
int CCorrelationList::getCountOfTotalCorrelationsProcessed() const {
	return (m_iCountOfTotalCorrelationsProcessed);
}

// ---------------------------------------------------------size
int CCorrelationList::length() const {
	std::lock_guard<std::recursive_mutex> vCorrelationGuard(
			m_CorrelationListMutex);
	return (m_msCorrelationList.size());
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

}  // namespace glasscore

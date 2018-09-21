#include <json.h>
#include <string>
#include <utility>
#include <memory>
#include <algorithm>
#include <cmath>
#include <set>
#include <vector>
#include <ctime>
#include <random>
#include "Date.h"
#include "Pid.h"
#include "Site.h"
#include "Pick.h"
#include "Glass.h"
#include "Web.h"
#include "Hypo.h"
#include "SiteList.h"
#include "PickList.h"
#include "HypoList.h"
#include "Logit.h"

#define MAX_QUEUE_FACTOR 10

namespace glasscore {

// ---------------------------------------------------------CPickList
CPickList::CPickList(int numThreads, int sleepTime, int checkInterval)
		: glass3::util::ThreadBaseClass("PickList", sleepTime, numThreads,
										checkInterval) {
	clear();

	// start up the threads
	start();
}

// ---------------------------------------------------------~CPickList
CPickList::~CPickList() {
	// clean up everything else
	clear();
}

// ---------------------------------------------------------~clear
void CPickList::clear() {
	std::lock_guard<std::recursive_mutex> pickListGuard(m_PickListMutex);

	m_pSiteList = NULL;

	// clear the multiset
	m_msPickList.clear();

	m_PicksToProcessMutex.lock();
	while (m_qPicksToProcess.empty() == false) {
		m_qPicksToProcess.pop();
	}
	m_PicksToProcessMutex.unlock();

	// reset nPick
	m_iCountOfTotalPicksProcessed = 0;
	m_iMaxAllowablePickCount = 10000;
}

// -------------------------------------------------------receiveExternalMessage
bool CPickList::receiveExternalMessage(std::shared_ptr<json::Object> com) {
	// null check json
	if (com == NULL) {
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"CPickList::receiveExternalMessage: NULL json communication.");
		return (false);
	}

	// Input pick data can have Type keys
	if (com->HasKey("Type")
			&& ((*com)["Type"].GetType() == json::ValueType::StringVal)) {
		// dispatch to appropriate function based on Cmd value
		json::Value v = (*com)["Type"].ToString();

		// add a pick
		if (v == "Pick") {
			return (addPick(com));
		}
	}

	// this communication was not handled
	return (false);
}

// ---------------------------------------------------------addPick
bool CPickList::addPick(std::shared_ptr<json::Object> pick) {
	// null check json
	if (pick == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CPickList::addPick: NULL json pick.");
		return (false);
	}

	// null check pSiteList
	if (m_pSiteList == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CPickList::addPick: NULL pSiteList.");
		return (false);
	}

	// check cmd or type
	if (pick->HasKey("Cmd")
			&& ((*pick)["Cmd"].GetType() == json::ValueType::StringVal)) {
		std::string cmd = (*pick)["Cmd"].ToString();

		if (cmd != "Pick") {
			glassutil::CLogit::log(
					glassutil::log_level::warn,
					"CPickList::addPick: Non-Pick message passed in.");
			return (false);
		}
	} else if (pick->HasKey("Type")
			&& ((*pick)["Type"].GetType() == json::ValueType::StringVal)) {
		std::string type = (*pick)["Type"].ToString();

		if (type != "Pick") {
			glassutil::CLogit::log(
					glassutil::log_level::warn,
					"CPickList::addPick: Non-Pick message passed in.");
			return (false);
		}
	} else {
		// no command or type
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"CPickList::addPick: Missing required Cmd/Type Key.");
		return (false);
	}

	// get the current size of the queue
	m_PicksToProcessMutex.lock();
	int queueSize = m_qPicksToProcess.size();
	m_PicksToProcessMutex.unlock();

	while (queueSize >= (getNumThreads() * MAX_QUEUE_FACTOR)) {
		/* glassutil::CLogit::log(glassutil::log_level::debug,
		 "CPickList::addPick. Delaying work due to "
		 "PickList process queue size."); */

		setThreadHealth();
		std::this_thread::sleep_for(std::chrono::milliseconds(getSleepTime()));

		// check to see if the queue has changed size
		m_PicksToProcessMutex.lock();
		queueSize = m_qPicksToProcess.size();
		m_PicksToProcessMutex.unlock();
	}

	// create new pick from json message
	CPick * newPick = new CPick(pick, m_pSiteList);

	// check to see if we got a valid pick
	if ((newPick->getSite() == NULL) || (newPick->getTPick() == 0)
			|| (newPick->getID() == "")) {
		// cleanup
		delete (newPick);
		// message was processed
		return (true);
	}

	// check if pick is duplicate, if pGlass exists
	bool duplicate = checkDuplicate(newPick->getTPick(),
									newPick->getSite()->getSCNL(),
									CGlass::getPickDuplicateTimeWindow());

	// it is a duplicate, log and don't add pick
	if (duplicate) {
		glassutil::CLogit::log(
				glassutil::log_level::warn,
				"CPickList::addPick: Duplicate pick not passed in.");
		delete (newPick);
		// message was processed
		return (true);
	}

	// create new shared pointer to this pick
	std::shared_ptr<CPick> pck(newPick);

	m_iCountOfTotalPicksProcessed++;

	// get maximum number of picks
	// use max picks from pGlass if we have it
	if (CGlass::getMaxNumPicks() > 0) {
		m_iMaxAllowablePickCount = CGlass::getMaxNumPicks();
	}

	// lock while we're modifying the multiset
	m_PickListMutex.lock();

	// check to see if we're at the pick limit
	if (m_msPickList.size() >= m_iMaxAllowablePickCount) {
		std::multiset<std::shared_ptr<CPick>, PickCompare>::iterator oldest =
				m_msPickList.begin();

		// find first pick in multiset
		std::shared_ptr<CPick> oldestPick = *oldest;

		// remove from site specific pick list
		oldestPick->getSite()->removePick(oldestPick);

		// remove from from multiset
		m_msPickList.erase(oldest);
	}

	// add to site specific pick list
	pck->getSite()->addPick(pck);

	// add to multiset
	m_msPickList.insert(pck);

	// done modifying the multiset
	m_PickListMutex.unlock();

	// wait until there's space in the queue
	// we don't want to build up a huge queue of unprocessed
	// picks
	if (CGlass::getHypoList()) {
		// add pick to processing list
		m_PicksToProcessMutex.lock();
		m_qPicksToProcess.push(pck);
		m_PicksToProcessMutex.unlock();
	}

	// we're done, message was processed
	return (true);
}

// -----------------------------------------------------getPicks
std::vector<std::weak_ptr<CPick>> CPickList::getPicks(double t1, double t2) {
	std::vector<std::weak_ptr<CPick>> picks;

	if (t1 == t2) {
		return (picks);
	}
	if (t1 > t2) {
		double temp = t2;
		t2 = t1;
		t1 = temp;
	}

	std::shared_ptr<CSite> nullSite;

	// construct the lower bound value. std::multiset requires
	// that this be in the form of a std::shared_ptr<CPick>
	std::shared_ptr<CPick> lowerValue = std::make_shared<CPick>(nullSite, t1,
																"", 0, 0);

	// construct the upper bound value. std::multiset requires
	// that this be in the form of a std::shared_ptr<CPick>
	std::shared_ptr<CPick> upperValue = std::make_shared<CPick>(nullSite, t2,
																"", 0, 0);

	std::lock_guard<std::recursive_mutex> listGuard(m_PickListMutex);

	// don't bother if the list is empty
	if (m_msPickList.size() == 0) {
		return (picks);
	}

	// get the bounds for this window
	std::multiset<std::shared_ptr<CPick>, PickCompare>::iterator lower =
			m_msPickList.lower_bound(lowerValue);
	std::multiset<std::shared_ptr<CPick>, PickCompare>::iterator upper =
			m_msPickList.upper_bound(upperValue);

	// found nothing
	if (lower == m_msPickList.end()) {
		return (picks);
	}

	// found one
	if ((lower == upper) && (lower != m_msPickList.end())) {
		std::shared_ptr<CPick> aPick = *lower;

		if (aPick != NULL) {
			std::weak_ptr<CPick> awPick = aPick;

			// add to the list of picks
			picks.push_back(awPick);
		}
		return (picks);
	}  // end found one

	// loop through found picks
	for (std::multiset<std::shared_ptr<CPick>, PickCompare>::iterator it = lower;
			((it != upper) && (it != m_msPickList.end())); ++it) {
		std::shared_ptr<CPick> aPick = *it;

		if (aPick != NULL) {
			std::weak_ptr<CPick> awPick = aPick;

			// add to the list of hypos
			picks.push_back(awPick);
		}
	}

	// return the list of picks we found
	return (picks);
}

// -----------------------------------------------------checkDuplicate
bool CPickList::checkDuplicate(double newTPick, std::string newSCNL,
								double tWindow) {
	// null checks
	if (newTPick < 0) {
		return (false);
	}
	if (newSCNL == "") {
		return (false);
	}
	if (tWindow == 0.0) {
		return (false);
	}

	std::vector<std::weak_ptr<CPick>> picks = getPicks(newTPick - tWindow,
														newTPick + tWindow);

	if (picks.size() == 0) {
		return (false);
	}

	// loop through possible matching picks
	for (int i = 0; i < picks.size(); i++) {
		// make sure pick is still valid before checking
		if (std::shared_ptr<CPick> currentPick = picks[i].lock()) {
			// check site
			if (currentPick->getSite() == NULL) {
				continue;
			}

			// get the values we care about
			double currentTPick = currentPick->getTPick();
			std::string currentSCNL = currentPick->getSite()->getSCNL();

			// check if time difference is within window
			if (std::abs(newTPick - currentTPick) < tWindow) {
				// check if sites match
				if (newSCNL == currentSCNL) {
					// if match is found, log and return true
					glassutil::CLogit::log(
							glassutil::log_level::warn,
							"CPickList::checkDuplicate: Duplicate (window = "
									+ std::to_string(tWindow) + ") : old:"
									+ currentSCNL + " "
									+ std::to_string(currentTPick)
									+ " new(del):" + newSCNL + " "
									+ std::to_string(newTPick));
					return (true);
				}
			}
		}
	}

	return (false);
}

// ---------------------------------------------------------scavenge
bool CPickList::scavenge(std::shared_ptr<CHypo> hyp, double tDuration) {
	// Scan all picks within specified time range, adding any
	// that meet association criteria to hypo object provided.
	// Returns true if any associated.

	// null check
	if (hyp == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CPickList::scavenge: NULL CHypo provided.");
		return (false);
	}

	char sLog[1024];

	glassutil::CLogit::log(glassutil::log_level::debug,
							"CPickList::scavenge. " + hyp->getID());

	// Calculate range for possible associations
	double sdassoc = CGlass::getAssociationSDCutoff();
	int addCount = 0;
	bool associated = false;

	std::vector<std::weak_ptr<CPick>> picks = getPicks(
			hyp->getTOrigin() - tDuration, hyp->getTOrigin() + tDuration);

	if (picks.size() == 0) {
		return (false);
	}

	// loop through possible matching picks
	for (int i = 0; i < picks.size(); i++) {
		// make sure pick is still valid before checking
		if (std::shared_ptr<CPick> currentPick = picks[i].lock()) {
			// nullcheck
			if (currentPick == NULL) {
				continue;
			}

			std::shared_ptr<CHypo> pickHyp = currentPick->getHypoReference();

			// check to see if this pick is already in this hypo
			if (hyp->hasPickReference(currentPick)) {
				// it is, skip it
				continue;
			}

			// check to see if this pick can be associated with this hypo
			if (!hyp->canAssociate(currentPick, ASSOC_SIGMA_VALUE_SECONDS,
									sdassoc)) {
				// it can't, skip it
				continue;
			}

			// check to see if this pick is part of ANY hypo
			if (pickHyp == NULL) {
				// unassociated with any existing hypo
				// link pick to the hypo we're working on
				currentPick->addHypoReference(hyp, true);

				// add pick to this hypo
				hyp->addPickReference(currentPick);

				// we've associated a pick
				associated = true;
				addCount++;
			} else {
				// associated with an existing hypo
				// Add it to this hypo, but don't change the pick's hypo link
				// Let resolve() sort out which hypo the pick fits best with
				hyp->addPickReference(currentPick);

				// we've associated a pick
				associated = true;
				addCount++;
			}
		}
	}

	glassutil::CLogit::log(
			glassutil::log_level::debug,
			"CPickList::scavenge " + hyp->getID() + " added:"
					+ std::to_string(addCount));

	// return whether we've associated at least one pick
	return (associated);
}

// ---------------------------------------------------------work
glass3::util::WorkState CPickList::work() {
	bool bNucleateThisPick = true;

	// make sure we have a HypoList
	if (CGlass::getHypoList() == NULL) {
		// on to the next loop
		return (glass3::util::WorkState::Idle);
	}

	// check to see that we've not run too far ahead of the hypo processing
	// if we have, pause for a bit to allow for it to catch up.
	if (CGlass::getHypoList()->getHypoProcessingQueueLength()
			> (CGlass::getHypoList()->getNumThreads() * MAX_QUEUE_FACTOR)) {
		glassutil::CLogit::log(glassutil::log_level::debug,
								"CPickList::work. Delaying work due to "
								"HypoList process queue size.");
		// on to the next loop
		return (glass3::util::WorkState::Idle);
	}

	// lock for queue access
	m_PicksToProcessMutex.lock();

	// are there any jobs
	if (m_qPicksToProcess.empty() == true) {
		// unlock and skip until next time
		m_PicksToProcessMutex.unlock();

		// on to the next loop
		return (glass3::util::WorkState::Idle);
	}

	// get the next pick
	std::shared_ptr<CPick> pck = m_qPicksToProcess.front();
	m_qPicksToProcess.pop();

	// done with queue
	m_PicksToProcessMutex.unlock();

	// check the pick
	if (pck == NULL) {
		// on to the next loop
		return (glass3::util::WorkState::Idle);
	}

	// Attempt to associate the pick
	CGlass::getHypoList()->associateData(pck);

	// check to see if the pick is currently associated to a hypo
	std::shared_ptr<CHypo> pHypo = pck->getHypoReference();
	if (pHypo != NULL) {
		// compute ratio
		double adBayesRatio = (pHypo->getBayesValue())
				/ (pHypo->getNucleationStackThreshold());

		std::string pt = glassutil::CDate::encodeDateTime(pck->getTPick());

		// check to see if the ratio is high enough to not bother
		// NOTE: Hardcoded ratio threshold
		if (adBayesRatio > 2.0) {
			glassutil::CLogit::log(
					glassutil::log_level::debug,
					"CPickList::work(): SKIPNUC tPick:" + pt
							+ "; idPick:" + pck->getID() + " due to "
							"association with a hypo with stack twice "
									"threshold ("
							+ std::to_string(pHypo->getBayesValue()) + ")");
			bNucleateThisPick = false;
		}
	}

	// Attempt nucleation unless we were told not to.
	if (bNucleateThisPick) {
		pck->nucleate();
	}

	// give up some time at the end of the loop
	return (glass3::util::WorkState::OK);
}

// ---------------------------------------------------------getSiteList
const CSiteList* CPickList::getSiteList() const {
	std::lock_guard<std::recursive_mutex> pickListGuard(m_PickListMutex);
	return (m_pSiteList);
}

// ---------------------------------------------------------setSiteList
void CPickList::setSiteList(CSiteList* siteList) {
	std::lock_guard<std::recursive_mutex> pickListGuard(m_PickListMutex);
	m_pSiteList = siteList;
}

// ---------------------------------------------------------getPickMax
int CPickList::getMaxAllowablePickCount() const {
	return (m_iMaxAllowablePickCount);
}

// ---------------------------------------------------------setPickMax
void CPickList::setMaxAllowablePickCount(int picknMax) {
	m_iMaxAllowablePickCount = picknMax;
}

// ---------------------------------------------------------getPickTotal
int CPickList::getCountOfTotalPicksProcessed() const {
	return (m_iCountOfTotalPicksProcessed);
}

// ---------------------------------------------------------size
int CPickList::length() const {
	std::lock_guard<std::recursive_mutex> vPickGuard(m_PickListMutex);
	return (m_msPickList.size());
}

}  // namespace glasscore

#include "PickList.h"
#include <json.h>
#include <date.h>
#include <logger.h>
#include <string>
#include <utility>
#include <memory>
#include <algorithm>
#include <cmath>
#include <set>
#include <vector>
#include <ctime>
#include "Site.h"
#include "Pick.h"
#include "Glass.h"
#include "Web.h"
#include "Hypo.h"
#include "SiteList.h"
#include "HypoList.h"

namespace glasscore {

// constants
const int CPickList::k_nMaxAllowablePickCountDefault;
const int CPickList::k_ProcessQueueMaximumSize;

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
	m_iMaxAllowablePickCount = k_nMaxAllowablePickCountDefault;

	// init the upper and lower values
	std::shared_ptr<CSite> nullSite;
	if (m_LowerValue == NULL) {
		m_LowerValue = std::make_shared<CPick>(nullSite, 0, "lower", 0, 0);
	}
	if (m_UpperValue == NULL) {
		m_UpperValue = std::make_shared<CPick>(nullSite, 0, "upper", 0, 0);
	}
}

// -------------------------------------------------------receiveExternalMessage
bool CPickList::receiveExternalMessage(std::shared_ptr<json::Object> com) {
	// null check json
	if (com == NULL) {
		glass3::util::Logger::log(
				"error",
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
		glass3::util::Logger::log("error",
									"CPickList::addPick: NULL json pick.");
		return (false);
	}

	// null check pSiteList
	if (m_pSiteList == NULL) {
		glass3::util::Logger::log("error",
									"CPickList::addPick: NULL pSiteList.");
		return (false);
	}

	// check cmd or type
	if (pick->HasKey("Cmd")
			&& ((*pick)["Cmd"].GetType() == json::ValueType::StringVal)) {
		std::string cmd = (*pick)["Cmd"].ToString();

		if (cmd != "Pick") {
			glass3::util::Logger::log(
					"warning",
					"CPickList::addPick: Non-Pick message passed in.");
			return (false);
		}
	} else if (pick->HasKey("Type")
			&& ((*pick)["Type"].GetType() == json::ValueType::StringVal)) {
		std::string type = (*pick)["Type"].ToString();

		if (type != "Pick") {
			glass3::util::Logger::log(
					"warning",
					"CPickList::addPick: Non-Pick message passed in.");
			return (false);
		}
	} else {
		// no command or type
		glass3::util::Logger::log(
				"error", "CPickList::addPick: Missing required Cmd/Type Key.");
		return (false);
	}

	// get the current size of the queue
	while (m_PicksToProcessMutex.try_lock() == false) {
		std::this_thread::sleep_for(
								std::chrono::milliseconds(100));
		setThreadHealth();
	}
	int queueSize = m_qPicksToProcess.size();
	m_PicksToProcessMutex.unlock();
	setThreadHealth();

	// don't let the queue get too large
	while (queueSize >= k_ProcessQueueMaximumSize) {
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

	// add pick to processing list, work() will do the rest
	m_PicksToProcessMutex.lock();
	m_qPicksToProcess.push(pick);
	m_PicksToProcessMutex.unlock();
	setThreadHealth();

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

	// nullcheck
	if ((m_LowerValue == NULL) || (m_UpperValue == NULL)) {
		return (picks);
	}

	// set the lower bound value. std::multiset requires
	// that this be in the form of a std::shared_ptr<CPick>
	m_LowerValue->setTSort(t1);

	// set the upper bound value. std::multiset requires
	// that this be in the form of a std::shared_ptr<CPick>
	m_UpperValue->setTSort(t2);

	std::lock_guard<std::recursive_mutex> listGuard(m_PickListMutex);

	// don't bother if the list is empty
	if (m_msPickList.size() == 0) {
		return (picks);
	}

	// get the bounds for this window
	std::multiset<std::shared_ptr<CPick>, PickCompare>::iterator lower =
			m_msPickList.lower_bound(m_LowerValue);
	std::multiset<std::shared_ptr<CPick>, PickCompare>::iterator upper =
			m_msPickList.upper_bound(m_UpperValue);

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

// -----------------------------------------------------getDuplicate
std::shared_ptr<CPick> CPickList::getDuplicate(double newTPick,
												std::string newSCNL,
												double tWindow) {
	// null checks
	if (newTPick < 0) {
		return (NULL);
	}
	if (newSCNL == "") {
		return (NULL);
	}
	if (tWindow == 0.0) {
		return (NULL);
	}

	std::vector<std::weak_ptr<CPick>> picks = getPicks(newTPick - tWindow,
														newTPick + tWindow);

	if (picks.size() == 0) {
		return (NULL);
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
					glass3::util::Logger::log(
							"debug",
							"CPickList::getDuplicate: Duplicate pick found (window = "
									+ std::to_string(tWindow) + "): existing pick: "
									+ currentSCNL + " "
									+ glass3::util::Date::encodeISO8601Time(currentTPick)
									+ " duplicate pick: " + newSCNL + " "
									+ glass3::util::Date::encodeISO8601Time(newTPick));
					return (currentPick);
				}
			}
		}
	}

	return (NULL);
}

// ---------------------------------------------------------scavenge
bool CPickList::scavenge(std::shared_ptr<CHypo> hyp, double tDuration) {
	// Scan all picks within specified time range, adding any
	// that meet association criteria to hypo object provided.
	// Returns true if any associated.

	// null check
	if (hyp == NULL) {
		glass3::util::Logger::log(
				"error", "CPickList::scavenge: NULL CHypo provided.");
		return (false);
	}

	char sLog[glass3::util::Logger::k_nMaxLogEntrySize];

	// glass3::util::Logger::log("debug", "CPickList::scavenge. " + hyp->getID());

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
			if (!hyp->canAssociate(currentPick,
									CGlass::k_dAssociationSecondsPerSigma,
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

	glass3::util::Logger::log(
			"debug",
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
			> (CGlass::getHypoList()->getNumThreads()
					* CGlass::iMaxQueueLenPerThreadFactor)) {
		glass3::util::Logger::log("debug",
									"CPickList::work. Delaying work due to "
									"HypoList process queue size.");
		// on to the next loop
		return (glass3::util::WorkState::Idle);
	}

	// lock for queue access
	while (m_PicksToProcessMutex.try_lock() == false) {
		std::this_thread::sleep_for(
								std::chrono::milliseconds(100));
		setThreadHealth();
	}

	// are there any picks to process
	if (m_qPicksToProcess.empty() == true) {
		// unlock and skip until next time
		m_PicksToProcessMutex.unlock();

		// on to the next loop
		return (glass3::util::WorkState::Idle);
	}

	// get the next pick
	std::shared_ptr<json::Object> jsonPick = m_qPicksToProcess.front();
	m_qPicksToProcess.pop();

	// done with queue
	m_PicksToProcessMutex.unlock();
	setThreadHealth();

	// check the pick
	if (jsonPick == NULL) {
		// on to the next loop
		return (glass3::util::WorkState::OK);
	}

	// create new pick from json message
	CPick * newPick = new CPick(jsonPick, m_pSiteList);

	// check to see if we got a valid pick
	if ((newPick->getSite() == NULL) || (newPick->getTPick() == 0)
			|| (newPick->getID() == "")) {
		// pick was not properly constructed, ignore new pick

		// cleanup
		delete (newPick);

		// message was processed
		return (glass3::util::WorkState::OK);
	}

	// signal that the thread is still alive after pick parsing
	setThreadHealth();

	// check if pick is duplicate
	std::shared_ptr<CPick> existingPick = getDuplicate(
			newPick->getTPick(), newPick->getSite()->getSCNL(),
			CGlass::getPickDuplicateTimeWindow());

	setThreadHealth();

	// it is a duplicate
	if (existingPick != NULL) {
		// do we allow updates (latest pick wins rather than
		// first pick wins)
		if (CGlass::getAllowPickUpdates()) {
			// update exiting pick, we update rather than replace
			// because the pick might be linked to a hypo
			existingPick->initialize(existingPick->getSite(),
										newPick->getTPick(), newPick->getID(),
										newPick->getSource(),
										newPick->getBackAzimuth(),
										newPick->getSlowness(),
										newPick->getClassifiedPhase(),
										newPick->getClassifiedPhaseProbability(),
										newPick->getClassifiedDistance(),
										newPick->getClassifiedDistanceProbability(),
										newPick->getClassifiedAzimuth(),
										newPick->getClassifiedAzimuthProbability(),
										newPick->getClassifiedDepth(),
										newPick->getClassifiedDepthProbability(),
										newPick->getClassifiedMagnitude(),
										newPick->getClassifiedMagnitudeProbability());

			// update the position of the pick in the sort
			updatePosition(existingPick);

			// if the pick was associated to a hypo,
			// reprocess that hypo
			std::shared_ptr<CHypo> hypo = existingPick->getHypoReference();
			if (hypo != NULL) {
				CGlass::getHypoList()->appendToHypoProcessingQueue(hypo);
			}
		} else {
			// ignore new pick (first pick wins)
			// glass3::util::Logger::log(
			// "debug",
			// "CPickList::work: Duplicate pick not passed in.");
		}

		// cleanup
		delete (newPick);

		// message was processed
		return (glass3::util::WorkState::OK);
	}

	// are we configured to check pick noise classification
	if (CGlass::getPickNoiseClassificationThreshold() > 0) {
		// check to see if the phase classification is valid and above
		// our threshold

		if ((std::isnan(newPick->getClassifiedPhaseProbability()) != true) &&
			(newPick->getClassifiedPhaseProbability() >
			CGlass::getPickNoiseClassificationThreshold())) {
			// check to see if the phase is classified as noise
			if (newPick->getClassifiedPhase() == "Noise") {
				// this pick is noise, ignore new pick
				// cleanup
				delete (newPick);
				// message was processed (rejected)
				return (glass3::util::WorkState::OK);
			}
		}
	}

	// create new shared pointer to this pick
	std::shared_ptr<CPick> pick(newPick);
	setThreadHealth();

	m_iCountOfTotalPicksProcessed++;

	// get maximum number of picks
	// use max picks from pGlass if we have it
	if (CGlass::getMaxNumPicks() > 0) {
		m_iMaxAllowablePickCount = CGlass::getMaxNumPicks();
	}

	// lock while we're modifying the multiset
	while (m_PickListMutex.try_lock() == false) {
		std::this_thread::sleep_for(
								std::chrono::milliseconds(100));
		setThreadHealth();
	}

	// check to see if we're at the pick limit
	if (m_msPickList.size() >= m_iMaxAllowablePickCount) {
		std::multiset<std::shared_ptr<CPick>, PickCompare>::iterator oldest =
				m_msPickList.begin();

		// find first pick in multiset
		std::shared_ptr<CPick> oldestPick = *oldest;

		// check to see if the new pick is older than the
		// first pick in multiset
		if (pick->getTPick() <= oldestPick->getTPick()) {
			m_PickListMutex.unlock();
			// it is, don't insert
			// message was processed
			return (glass3::util::WorkState::OK);
		}

		// remove from site specific pick list
		oldestPick->getSite()->removePick(oldestPick);

		// remove from from multiset
		m_msPickList.erase(oldest);
	}

	// add to site specific pick list
	pick->getSite()->addPick(pick);

	// add to multiset
	m_msPickList.insert(pick);

	// done modifying the multiset
	m_PickListMutex.unlock();

	// signal that the thread is still alive after pick insertion
	setThreadHealth();

	// Attempt to associate the pick with existing hypos
	CGlass::getHypoList()->associateData(pick);

	// check to see if the pick is now associated to a hypo
	std::shared_ptr<CHypo> pHypo = pick->getHypoReference();
	if (pHypo != NULL) {
		// compute ratio to threshold
		double adBayesRatio = (pHypo->getBayesValue())
				/ (pHypo->getNucleationStackThreshold());

		std::string pt = glass3::util::Date::encodeDateTime(pick->getTPick());

		// check to see if the ratio is high enough to not bother
		// nucleating
		// NOTE: Hardcoded ratio threshold
		if (adBayesRatio > 2.0) {
			glass3::util::Logger::log(
					"debug",
					"CPickList::work(): SKIPNUC tPick:" + pt + "; idPick:"
							+ pick->getID() + " ("
							+ pick->getSite()->getSCNL() +
							") due to association with hypo "
							+ pHypo->getID() + " with stack twice threshold ("
							+ std::to_string(pHypo->getBayesValue()) + ", "
							+ std::to_string(adBayesRatio) + ")");
			bNucleateThisPick = false;
		}
	}

	// signal that the thread is still alive, since association might have
	// taken awhile
	setThreadHealth();

	// check to see if this pick *FITS* with an existing large event
	// if so do not nucleate
	if (bNucleateThisPick == true) {
		if (CGlass::getHypoList()->fitData(pick) == true) {
			std::string pt = glass3::util::Date::encodeDateTime(pick->getTPick());
			glass3::util::Logger::log(
					"debug",
					"CPickList::work(): SKIPNUC tPick:" + pt + "; idPick:"
							+ pick->getID() + " ("
							+ pick->getSite()->getSCNL() +
							") because it fits with another hypo");
			bNucleateThisPick = false;
		}
	}

	setThreadHealth();

	// Attempt nucleation unless we were told not to.
	if (bNucleateThisPick == true) {
		pick->nucleate(this);
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

// ---------------------------------------------------------updatePosition
void CPickList::updatePosition(std::shared_ptr<CPick> pick) {
	// nullchecks
	if (pick == NULL) {
		return;
	}

	std::lock_guard<std::recursive_mutex> listGuard(m_PickListMutex);

	// from my research, the best way to "update" the position of an item
	// in a multiset when the key value has changed (in this case, the pick
	// time) is to remove and re-add the item. This will give us O(log n)
	// complexity for updating one item, which is better than a full sort
	// (which I'm not really sure how to do on a multiset)
	// erase
	eraseFromMultiset(pick);

	// update tSort
	pick->setTSort(pick->getTPick());

	// insert
	m_msPickList.insert(pick);
}

// ---------------------------------------------------------eraseFromMultiset
void CPickList::eraseFromMultiset(std::shared_ptr<CPick> pick) {
	// nullchecks
	if (pick == NULL) {
		return;
	}

	std::lock_guard<std::recursive_mutex> listGuard(m_PickListMutex);

	if (m_msPickList.size() == 0) {
		return;
	}

	// first, try to delete the pick the efficient way
	// we need to be careful, because multiple picks in the mulitset
	// can have the same tSort, and a simple erase would delete
	// them all, which would be BAD, so we need to confirm the id
	auto lower = m_msPickList.lower_bound(pick);
	auto upper = m_msPickList.upper_bound(pick);

	// for all matching (tSort range) picks
	for (auto it = lower; ((it != upper) && (it != m_msPickList.end())); ++it) {
		std::shared_ptr<CPick> aPick = *it;

		// only erase the correct one
		if (aPick->getID() == pick->getID()) {
			m_msPickList.erase(it);
			return;
		}
	}

	glass3::util::Logger::log(
			"warning",
			"CPickList::eraseFromMultiset: efficient delete for pick "
					+ pick->getID() + " didn't work.");

	// if we didn't delete it efficiently, loop through all picks, I know this is
	// brute force, but the efficient delete didn't work and we want to be sure
	// note: this may just be me being paranoid
	for (auto it = m_msPickList.begin(); (it != m_msPickList.end()); ++it) {
		std::shared_ptr<CPick> aPick = *it;

		// only erase the correct one
		if (aPick->getID() == pick->getID()) {
			m_msPickList.erase(it);
			return;
		}
	}

	glass3::util::Logger::log(
			"error",
			"CPickList::eraseFromMultiset: did not delete pick " + pick->getID()
					+ " in multiset, id not found.");
}

}  // namespace glasscore

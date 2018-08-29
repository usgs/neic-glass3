#include <json.h>
#include <string>
#include <utility>
#include <memory>
#include <algorithm>
#include <cmath>
#include <set>
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

#define MAX_QUEUE_FACTOR 3

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

	m_pGlass = NULL;
	m_pSiteList = NULL;

	// clear picks
	clearPicks();
}

// ---------------------------------------------------------~clear
void CPickList::clearPicks() {
	std::lock_guard<std::recursive_mutex> listGuard(m_PickListMutex);

	// clear the multiset
	m_msPickList.clear();

	m_PicksToProcessMutex.lock();
	while (m_qPicksToProcess.empty() == false) {
		m_qPicksToProcess.pop();
	}
	m_PicksToProcessMutex.unlock();

	// reset nPick
	m_iPickTotal = 0;
	m_iPickMax = 10000;
}

// ---------------------------------------------------------Dispatch
bool CPickList::dispatch(std::shared_ptr<json::Object> com) {
	// null check json
	if (com == NULL) {
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"CPickList::dispatch: NULL json communication.");
		return (false);
	}

	// check for a command
	if (com->HasKey("Cmd")
			&& ((*com)["Cmd"].GetType() == json::ValueType::StringVal)) {
		// dispatch to appropriate function based on Cmd value
		json::Value v = (*com)["Cmd"].ToString();

		// add a pick
		if (v == "Pick") {
			return (addPick(com));
		}

		// clear all picks
		if (v == "ClearGlass") {
			clearPicks();

			// ClearGlass is also relevant to other glass
			// components, return false so they also get a
			// chance to process it
			return (false);
		}
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

	// create new pick from json message
	CPick * newPick = new CPick(pick, m_iPickTotal + 1, m_pSiteList);

	// check to see if we got a valid pick
	if ((newPick->getSite() == NULL) || (newPick->getTPick() == 0)
			|| (newPick->getID() == "")) {
		// cleanup
		delete (newPick);
		// message was processed
		return (true);
	}

	// check if pick is duplicate, if pGlass exists
	if (m_pGlass) {
		bool duplicate = checkDuplicate(newPick->getTPick(),
										newPick->getSite()->getSCNL(),
										m_pGlass->getPickDuplicateTimeWindow());

		// it is a duplicate, log and don't add pick
		if (duplicate) {
			glassutil::CLogit::log(
					glassutil::log_level::warn,
					"CPickList::addPick: Duplicate pick not passed in.");
			delete (newPick);
			// message was processed
			return (true);
		}
	}

	// create new shared pointer to this pick
	std::shared_ptr<CPick> pck(newPick);

	m_iPickTotal++;

	// get maximum number of picks
	// use max picks from pGlass if we have it
	if (m_pGlass) {
		m_iPickMax = m_pGlass->getMaxNumPicks();
	}

	// lock while we're modifying the multiset
	m_PickListMutex.lock();

	// check to see if we're at the pick limit
	if (m_msPickList.size() == m_iPickMax) {
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

	// get the current size of the queue
	m_PicksToProcessMutex.lock();
	int queueSize = m_qPicksToProcess.size();
	m_PicksToProcessMutex.unlock();

	// wait until there's space in the queue
	// we don't want to build up a huge queue of unprocessed
	// picks
	if ((m_pGlass) && (m_pGlass->getHypoList())) {
		while (queueSize >= (getNumThreads() * MAX_QUEUE_FACTOR)) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));

			// check to see if the queue has changed size
			m_PicksToProcessMutex.lock();
			queueSize = m_qPicksToProcess.size();
			m_PicksToProcessMutex.unlock();
		}

		// add pick to processing list
		m_PicksToProcessMutex.lock();
		m_qPicksToProcess.push(pck);
		m_PicksToProcessMutex.unlock();
	}

	// we're done, message was processed
	return (true);
}

// -----------------------------------------------------checkDuplicate
bool CPickList::checkDuplicate(double newTPick, std::string newSCNL,
								double tWindow) {
	// lock while we're searching the list
	std::lock_guard<std::recursive_mutex> listGuard(m_PickListMutex);

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

	// construct the lower bound value. std::multiset requires
	// that this be in the form of a std::shared_ptr<CPick>
	std::shared_ptr<CPick> lowerValue = std::make_shared<CPick>();
	lowerValue->initialize(NULL, (newTPick - tWindow), 0, "", 0, 0);

	// construct the upper bound value. std::multiset requires
	// that this be in the form of a std::shared_ptr<CPick>
	std::shared_ptr<CPick> upperValue = std::make_shared<CPick>();
	upperValue->initialize(NULL, (newTPick + tWindow), 0, "", 0, 0);

	if (m_msPickList.size() == 0) {
		return (false);
	}

	// get the bounds for this window
	std::multiset<std::shared_ptr<CPick>, PickCompare>::iterator lower =
			std::lower_bound(m_msPickList.begin(), m_msPickList.end(),
								lowerValue);
	std::multiset<std::shared_ptr<CPick>, PickCompare>::iterator upper =
			std::upper_bound(m_msPickList.begin(), m_msPickList.end(),
								upperValue);

	// don't bother if there's no picks in the window
	if (lower == upper) {
		return (false);
	}

	// loop through possible matching picks
	for (std::multiset<std::shared_ptr<CPick>, PickCompare>::iterator it = lower;
			((it != upper) && (it != m_msPickList.end())); ++it) {
		std::shared_ptr<CPick> currentPick = *it;

		// nullcheck
		if (currentPick == NULL) {
			continue;
		}
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
								+ std::to_string(currentTPick) + " new(del):"
								+ newSCNL + " " + std::to_string(newTPick));
				return (true);
			}
		}
	}

	return (false);
}

// ---------------------------------------------------------scavenge
bool CPickList::scavenge(std::shared_ptr<CHypo> hyp, double tWindow) {
	// Scan all picks within specified time range, adding any
	// that meet association criteria to hypo object provided.
	// Returns true if any associated.
	// lock while we're searching the list
	std::lock_guard<std::recursive_mutex> listGuard(m_PickListMutex);

	// null check
	if (hyp == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CPickList::scavenge: NULL CHypo provided.");
		return (false);
	}

	// check pGlass
	if (m_pGlass == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CPickList::scavenge: NULL glass pointer.");
		return (false);
	}

	char sLog[1024];

	glassutil::CLogit::log(glassutil::log_level::debug,
							"CPickList::scavenge. " + hyp->getID());

	// Calculate range for possible associations
	double sdassoc = m_pGlass->getAssociationSDCutoff();
	int addCount = 0;
	bool associated = false;

	// construct the lower bound value. std::multiset requires
	// that this be in the form of a std::shared_ptr<CPick>
	std::shared_ptr<CPick> lowerValue = std::make_shared<CPick>();
	lowerValue->initialize(NULL, (hyp->getTOrigin() - tWindow), 0, "", 0, 0);

	// construct the upper bound value. std::multiset requires
	// that this be in the form of a std::shared_ptr<CPick>
	std::shared_ptr<CPick> upperValue = std::make_shared<CPick>();
	upperValue->initialize(NULL, (hyp->getTOrigin() + tWindow), 0, "", 0, 0);

	if (m_msPickList.size() == 0) {
		return (false);
	}

	// get the bounds for this window
	std::multiset<std::shared_ptr<CPick>, PickCompare>::iterator lower =
			std::lower_bound(m_msPickList.begin(), m_msPickList.end(),
								lowerValue);
	std::multiset<std::shared_ptr<CPick>, PickCompare>::iterator upper =
			std::upper_bound(m_msPickList.begin(), m_msPickList.end(),
								upperValue);

	// don't bother if there's no picks in the window
	if (lower == upper) {
		return (false);
	}

	// loop through possible matching picks
	for (std::multiset<std::shared_ptr<CPick>, PickCompare>::iterator it = lower;
			((it != upper) && (it != m_msPickList.end())); ++it) {
		std::shared_ptr<CPick> pck = *it;

		// nullcheck
		if (pck == NULL) {
			continue;
		}

		std::shared_ptr<CHypo> pickHyp = pck->getHypo();

		// check to see if this pick is already in this hypo
		if (hyp->hasPick(pck)) {
			// it is, skip it
			continue;
		}

		// check to see if this pick can be associated with this hypo
		if (!hyp->associate(pck, 1.0, sdassoc)) {
			// it can't, skip it
			continue;
		}

		// check to see if this pick is part of ANY hypo
		if (pickHyp == NULL) {
			// unassociated with any existing hypo
			// link pick to the hypo we're working on
			pck->addHypo(hyp, true);

			// add pick to this hypo
			hyp->addPick(pck);

			// we've associated a pick
			associated = true;
			addCount++;
		} else {
			// associated with an existing hypo
			// Add it to this hypo, but don't change the pick's hypo link
			// Let resolve() sort out which hypo the pick fits best with
			hyp->addPick(pck);

			// we've associated a pick
			associated = true;
			addCount++;
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
	// make sure we have a pGlass and pGlass->pHypoList
	if (m_pGlass == NULL) {
		// on to the next loop
		return (glass3::util::WorkState::Idle);
	}
	if (m_pGlass->getHypoList() == NULL) {
		// on to the next loop
		return (glass3::util::WorkState::Idle);
	}

	// check to see that we've not run to far ahead of the hypo processing
	if (m_pGlass->getHypoList()->getHyposToProcessSize()
			> (m_pGlass->getHypoList()->getNumThreads() * MAX_QUEUE_FACTOR)) {
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

	// Attempt both association and nucleation of the new pick.
	// If both succeed, the mess is sorted out in darwin/evolve
	// associate
	m_pGlass->getHypoList()->associate(pck);

	// nucleate
	pck->nucleate();

	// give up some time at the end of the loop
	return (glass3::util::WorkState::OK);
}

const CSiteList* CPickList::getSiteList() const {
	std::lock_guard<std::recursive_mutex> pickListGuard(m_PickListMutex);
	return (m_pSiteList);
}

void CPickList::setSiteList(CSiteList* siteList) {
	std::lock_guard<std::recursive_mutex> pickListGuard(m_PickListMutex);
	m_pSiteList = siteList;
}

const CGlass* CPickList::getGlass() const {
	std::lock_guard<std::recursive_mutex> pickListGuard(m_PickListMutex);
	return (m_pGlass);
}

void CPickList::setGlass(CGlass* glass) {
	std::lock_guard<std::recursive_mutex> pickListGuard(m_PickListMutex);
	m_pGlass = glass;
}

int CPickList::getPickMax() const {
	return (m_iPickMax);
}

void CPickList::setPickMax(int picknMax) {
	m_iPickMax = picknMax;
}

int CPickList::getPickTotal() const {
	return (m_iPickTotal);
}

int CPickList::size() const {
	std::lock_guard<std::recursive_mutex> vPickGuard(m_PickListMutex);
	return (m_msPickList.size());
}

}  // namespace glasscore

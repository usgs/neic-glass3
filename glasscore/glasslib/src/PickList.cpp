#include <json.h>
#include <string>
#include <utility>
#include <memory>
#include <algorithm>
#include <cmath>
#include <map>
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

#define MAX_QUEUE_FACTOR 3

namespace glasscore {

// pick sort function
bool sortFunc(const std::pair<double, int> &lhs,
				const std::pair<double, int> &rhs) {
	if (lhs.first < rhs.first) {
		return (true);
	}
	return (false);
}

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

	// clear the vector and map
	m_vPick.clear();
	m_mPick.clear();

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
		bool duplicate = checkDuplicate(newPick,
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

	m_PickListMutex.lock();

	// create new shared pointer to this pick
	std::shared_ptr<CPick> pck(newPick);

	// Add pick to cache (mPick) and time sorted
	// index (vPick). If vPick has reached its
	// maximum capacity (nPickMax), then the
	// first pick in the vector is removed and the
	// corresponding entry in the cache is erased.
	// It should be noted, that since what is cached
	// is a smart pointer, if it is currently part
	// of an active event, the actual pick will not
	// be removed until either it is pruned from the
	// event or the event is completed and retired.
	m_iPickTotal++;

	// get maximum number of picks
	// use max picks from pGlass if we have it
	if (m_pGlass) {
		m_iPickMax = m_pGlass->getMaxNumPicks();
	}

	// create pair for insertion
	std::pair<double, int> p(pck->getTPick(), m_iPickTotal);

	// check to see if we're at the pick limit
	if (m_vPick.size() == m_iPickMax) {
		// find first pick in vector
		std::pair<double, int> pdx;
		pdx = m_vPick[0];
		auto pos = m_mPick.find(pdx.second);

		// remove pick from per site pick list
		pos->second->getSite()->removePick(pos->second);

		// erase from map
		m_mPick.erase(pos);

		// erase from vector
		m_vPick.erase(m_vPick.begin());
	}

	// Insert new pick in proper time sequence into pick vector
	// get the index of the new pick
	int iPick = indexPick(pck->getTPick());
	switch (iPick) {
		case -2:
			// Empty vector, just add it
			m_vPick.push_back(p);
			break;
		case -1:
			// Pick is before any others, insert at beginning
			m_vPick.insert(m_vPick.begin(), p);
			break;
		default:
			// pick is somewhere in vector
			if (iPick == m_vPick.size() - 1) {
				// pick is after all picks, add to end
				m_vPick.push_back(p);
			} else {
				// find where the pick should be inserted
				auto it = std::next(m_vPick.begin(), iPick + 1);

				// insert at that location
				m_vPick.insert(it, p);
			}
			break;
	}

	// add to pick map
	m_mPick[m_iPickTotal] = pck;

	// add to site specific pick list
	pck->getSite()->addPick(pck);

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

// ---------------------------------------------------------indexPixk
int CPickList::indexPick(double tPick) {
	std::lock_guard<std::recursive_mutex> listGuard(m_PickListMutex);

	// handle empty vector case
	if (m_vPick.size() == 0) {
		// return -2 to indicate empty vector
		return (-2);
	}

	double tFirstPick = m_vPick[0].first;

	// handle pick earlier than first element case
	// time is earlier than first pick
	if (tPick < tFirstPick) {
		// return -1 to indicate earlier than first element
		return (-1);
	}

	// handle case that the pick is later than last element
	int i1 = 0;
	int i2 = m_vPick.size() - 1;
	double tLastPick = m_vPick[i2].first;

	// time is after last pick
	if (tPick >= tLastPick) {
		// return index of last element
		return (i2);
	}

	// search for insertion point within vector
	// using a binary search
	// while upper minus lower bounds is greater than one
	while ((i2 - i1) > 1) {
		// compute current pick index
		int ix = (i1 + i2) / 2;

		double tCurrentPick = m_vPick[ix].first;

		// if time is before current pick
		if (tCurrentPick > tPick) {
			// new upper bound is this index
			i2 = ix;
		} else {  // if (tCurrentPick <= tPick)
			// if time is after or equal to current pick
			// new lower bound is this index
			i1 = ix;
		}
	}

	// return the last lower bound as the insertion point
	return (i1);
}

// ---------------------------------------------------------getPick
std::shared_ptr<CPick> CPickList::getPick(int idPick) {
	std::lock_guard<std::recursive_mutex> listGuard(m_PickListMutex);

	// try to find that id in map
	auto pos = m_mPick.find(idPick);

	// make sure that we found something
	if (pos != m_mPick.end()) {
		// return the pick
		return (pos->second);
	}

	// found nothing
	return (NULL);
}

// ---------------------------------------------------------listPicks
void CPickList::listPicks() {
	std::lock_guard<std::recursive_mutex> listGuard(m_PickListMutex);

	int n = 0;
	char sLog[1024];

	// for each pick
	for (auto p : m_vPick) {
		// list it
		snprintf(sLog, sizeof(sLog), "%d: %.2f %d", n++, p.first, p.second);
		glassutil::CLogit::Out(sLog);
	}
}

// -----------------------------------------------------checkDuplicate
bool CPickList::checkDuplicate(CPick *newPick, double window) {
	// null checks
	if (newPick == NULL) {
		return (false);
	}
	if (window == 0.0) {
		return (false);
	}

	// set default return to no match
	bool matched = false;

	std::lock_guard<std::recursive_mutex> listGuard(m_PickListMutex);

	// get the index of the earliest possible match
	int it1 = indexPick(newPick->getTPick() - window);

	// get index of the latest possible pick
	int it2 = indexPick(newPick->getTPick() + window);

	// index can't be negative, it1/2 negative if pick before first in list
	if (it1 < 0) {
		it1 = 0;
	}
	if (it2 < 0) {
		it2 = 0;
	}

	// don't bother if there's no picks
	if (it1 == it2) {
		return (false);
	}

	// loop through possible matching picks
	for (int it = it1; it <= it2; it++) {
		auto q = m_vPick[it];
		std::shared_ptr<CPick> pck = m_mPick[q.second];

		// check if time difference is within window
		if (std::abs(newPick->getTPick() - pck->getTPick()) < window) {
			// check if sites match
			if (newPick->getSite()->getSCNL() == pck->getSite()->getSCNL()) {
				// if match is found, set to true, log, and break out of loop
				matched = true;
				glassutil::CLogit::log(
						glassutil::log_level::warn,
						"CPickList::checkDuplicat: Duplicate (window = "
								+ std::to_string(window) + ") : old:"
								+ pck->getSite()->getSCNL() + " "
								+ std::to_string(pck->getTPick()) + " new(del):"
								+ newPick->getSite()->getSCNL() + " "
								+ std::to_string(newPick->getTPick()));
				break;
			}
		}
	}

	return (matched);
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

	std::lock_guard<std::recursive_mutex> listGuard(m_PickListMutex);

	// get the index of the pick to start with
	// based on the hypo origin time
	int it1 = indexPick(hyp->getTOrigin());

	// index can't be negative
	// Primarily occurs if origin time is before first pick
	if (it1 < 0) {
		it1 = 0;
	}

	// get the index of the pick to end with by using the hypo
	// origin time plus the provided duration
	int it2 = indexPick(hyp->getTOrigin() + tDuration);

	// don't bother if there's no picks
	if (it1 == it2) {
		return (false);
	}

	int addCount = 0;

	// for each pick index between it1 and it2
	bool bAss = false;
	for (int it = it1; it < it2; it++) {
		// get the pick from the vector
		auto q = m_vPick[it];
		std::shared_ptr<CPick> pck = m_mPick[q.second];
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
			bAss = true;
			addCount++;
		} else {
			// associated with an existing hypo
			// Add it to this hypo, but don't change the pick's hypo link
			// Let resolve() sort out which hypo the pick fits best with
			hyp->addPick(pck);

			// we've associated a pick
			bAss = true;
			addCount++;
		}
	}

	glassutil::CLogit::log(
			glassutil::log_level::debug,
			"CPickList::scavenge " + hyp->getID() + " added:"
					+ std::to_string(addCount));

	// return whether we've associated at least one pick
	return (bAss);
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
	return (m_vPick.size());
}

}  // namespace glasscore

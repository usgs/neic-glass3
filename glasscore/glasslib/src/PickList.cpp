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
CPickList::CPickList(int numThreads, int sleepTime, int checkInterval) {
	// seed the random number generator
	std::random_device randomDevice;
	m_RandomGenerator.seed(randomDevice());

	// setup threads
	m_bRunProcessLoop = true;
	m_iNumThreads = numThreads;
	m_iSleepTimeMS = sleepTime;
	m_iStatusCheckInterval = checkInterval;
	std::time(&tLastStatusCheck);

	clear();

	// create threads
	for (int i = 0; i < m_iNumThreads; i++) {
		// create thread
		vProcessThreads.push_back(std::thread(&CPickList::processPick, this));

		// add to status map if we're tracking status
		if (m_iStatusCheckInterval > 0) {
			m_StatusMutex.lock();
			m_ThreadStatusMap[vProcessThreads[i].get_id()] = true;
			m_StatusMutex.unlock();
		}
	}
}

// ---------------------------------------------------------~CPickList
CPickList::~CPickList() {
	// disable status checking
	m_StatusMutex.lock();
	m_ThreadStatusMap.clear();
	m_StatusMutex.unlock();

	// signal threads to finish
	m_bRunProcessLoop = false;

	// wait for threads to finish
	for (int i = 0; i < vProcessThreads.size(); i++) {
		vProcessThreads[i].join();
	}

	// clean up everything else
	clear();
}

// ---------------------------------------------------------~clear
void CPickList::clear() {
	std::lock_guard<std::recursive_mutex> pickListGuard(m_PickListMutex);

	pGlass = NULL;
	pSiteList = NULL;

	// clear picks
	clearPicks();
}

// ---------------------------------------------------------~clear
void CPickList::clearPicks() {
	std::lock_guard<std::recursive_mutex> listGuard(m_vPickMutex);

	// clear the vector and map
	vPick.clear();
	mPick.clear();

	m_qProcessMutex.lock();
	while (qProcessList.empty() == false) {
		qProcessList.pop();
	}
	m_qProcessMutex.unlock();

	// reset nPick
	nPick = 0;
	nPickTotal = -1;
	nPickMax = 10000;
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
	if (pSiteList == NULL) {
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
	CPick * newPick = new CPick(pick, nPick + 1, pSiteList);

	// check to see if we got a valid pick
	if ((newPick->getSite() == NULL) || (newPick->getTPick() == 0)
			|| (newPick->getPid() == "")) {
		// cleanup
		delete (newPick);
		// message was processed
		return (true);
	}

	// check if pick is duplicate, if pGlass exists
	if (pGlass) {
		bool duplicate = checkDuplicate(newPick,
										pGlass->getPickDuplicateWindow());

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

	m_vPickMutex.lock();

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
	nPickTotal++;
	nPick++;

	// get maximum number of picks
	// use max picks from pGlass if we have it
	if (pGlass) {
		nPickMax = pGlass->getPickMax();
	}

	// create pair for insertion
	std::pair<double, int> p(pck->getTPick(), nPick);

	// check to see if we're at the pick limit
	if (vPick.size() == nPickMax) {
		// find first pick in vector
		std::pair<double, int> pdx;
		pdx = vPick[0];
		auto pos = mPick.find(pdx.second);

		// remove pick from per site pick list
		pos->second->getSite()->remPick(pos->second);

		// erase from map
		mPick.erase(pos);

		// erase from vector
		vPick.erase(vPick.begin());
	}

	// Insert new pick in proper time sequence into pick vector
	// get the index of the new pick
	int iPick = indexPick(pck->getTPick());
	switch (iPick) {
		case -2:
			// Empty vector, just add it
			vPick.push_back(p);
			break;
		case -1:
			// Pick is before any others, insert at beginning
			vPick.insert(vPick.begin(), p);
			break;
		default:
			// pick is somewhere in vector
			if (iPick == vPick.size() - 1) {
				// pick is after all picks, add to end
				vPick.push_back(p);
			} else {
				// find where the pick should be inserted
				auto it = std::next(vPick.begin(), iPick + 1);

				// insert at that location
				vPick.insert(it, p);
			}
			break;
	}

	// add to pick map
	mPick[nPick] = pck;

	// add to site specific pick list
	pck->getSite()->addPick(pck);

	m_vPickMutex.unlock();

	// get the current size of the queue
	m_qProcessMutex.lock();
	int queueSize = qProcessList.size();
	m_qProcessMutex.unlock();

	// wait until there's space in the queue
	// we don't want to build up a huge queue of unprocessed
	// picks
	if ((pGlass) && (pGlass->getHypoList())) {
		while (queueSize >= (m_iNumThreads * MAX_QUEUE_FACTOR)) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));

			// check to see if the queue has changed size
			m_qProcessMutex.lock();
			queueSize = qProcessList.size();
			m_qProcessMutex.unlock();
		}

		// add pick to processing list
		m_qProcessMutex.lock();
		qProcessList.push(pck);
		m_qProcessMutex.unlock();
	}

	// we're done, message was processed
	return (true);
}

// ---------------------------------------------------------indexPixk
int CPickList::indexPick(double tPick) {
	std::lock_guard<std::recursive_mutex> listGuard(m_vPickMutex);

	// handle empty vector case
	if (vPick.size() == 0) {
		// return -2 to indicate empty vector
		return (-2);
	}

	double tFirstPick = vPick[0].first;

	// handle pick earlier than first element case
	// time is earlier than first pick
	if (tPick < tFirstPick) {
		// return -1 to indicate earlier than first element
		return (-1);
	}

	// handle case that the pick is later than last element
	int i1 = 0;
	int i2 = vPick.size() - 1;
	double tLastPick = vPick[i2].first;

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

		double tCurrentPick = vPick[ix].first;

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
	std::lock_guard<std::recursive_mutex> listGuard(m_vPickMutex);

	// try to find that id in map
	auto pos = mPick.find(idPick);

	// make sure that we found something
	if (pos != mPick.end()) {
		// return the pick
		return (pos->second);
	}

	// found nothing
	return (NULL);
}

// ---------------------------------------------------------listPicks
void CPickList::listPicks() {
	std::lock_guard<std::recursive_mutex> listGuard(m_vPickMutex);

	int n = 0;
	char sLog[1024];

	// for each pick
	for (auto p : vPick) {
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

	std::lock_guard<std::recursive_mutex> listGuard(m_vPickMutex);

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
		auto q = vPick[it];
		std::shared_ptr<CPick> pck = mPick[q.second];

		// check if time difference is within window
		if (std::abs(newPick->getTPick() - pck->getTPick()) < window) {
			// check if sites match
			if (newPick->getSite()->getScnl() == pck->getSite()->getScnl()) {
				// if match is found, set to true, log, and break out of loop
				matched = true;
				glassutil::CLogit::log(
						glassutil::log_level::warn,
						"CPickList::checkDuplicat: Duplicate (window = "
								+ std::to_string(window) + ") : old:"
								+ pck->getSite()->getScnl() + " "
								+ std::to_string(pck->getTPick()) + " new(del):"
								+ newPick->getSite()->getScnl() + " "
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
	if (pGlass == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CPickList::scavenge: NULL glass pointer.");
		return (false);
	}

	char sLog[1024];

	glassutil::CLogit::log(glassutil::log_level::debug,
							"CPickList::scavenge. " + hyp->getPid());

	// Calculate range for possible associations
	double sdassoc = pGlass->getSdAssociate();

	std::lock_guard<std::recursive_mutex> listGuard(m_vPickMutex);

	// get the index of the pick to start with
	// based on the hypo origin time
	int it1 = indexPick(hyp->getTOrg());

	// index can't be negative
	// Primarily occurs if origin time is before first pick
	if (it1 < 0) {
		it1 = 0;
	}

	// get the index of the pick to end with by using the hypo
	// origin time plus the provided duration
	int it2 = indexPick(hyp->getTOrg() + tDuration);

	// don't bother if there's no picks
	if (it1 == it2) {
		return (false);
	}

	int addCount = 0;

	// for each pick index between it1 and it2
	bool bAss = false;
	for (int it = it1; it < it2; it++) {
		// get the pick from the vector
		auto q = vPick[it];
		std::shared_ptr<CPick> pck = mPick[q.second];
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
			pck->addHypo(hyp, "W", true);

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
			"CPickList::scavenge " + hyp->getPid() + " added:"
					+ std::to_string(addCount));

	// return whether we've associated at least one pick
	return (bAss);
}

// ---------------------------------------------------------rogues
std::vector<std::shared_ptr<CPick>> CPickList::rogues(std::string pidHyp,
														double tOrg,
														double tDuration) {
	// Generate rogue list (all picks that are not associated
	// with given event, but could be)
	std::vector<std::shared_ptr<CPick>> vRogue;

	// null checks
	if (pidHyp == "") {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CPickList::rogues: Empty pidHyp provided.");
		return (vRogue);
	}
	if (tOrg <= 0) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CPickList::rogues: Invalid tOrg provided.");
		return (vRogue);;
	}
	if (tDuration <= 0) {
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"CPickList::rogues: Invalid tDuration provided.");
		return (vRogue);
	}

	std::lock_guard<std::recursive_mutex> listGuard(m_vPickMutex);

	// get the index of the pick to start with
	// based on the provided origin time
	int it1 = indexPick(tOrg);

	// index can't be negative
	// Primarily occurs if origin time is before first pick
	if (it1 < 0) {
		it1 = 0;
	}

	// get the index of the pick to end with by using the provided
	// origin time plus the provided duration
	int it2 = indexPick(tOrg + tDuration);

	// for each pick index between it1 and it2
	for (int it = it1; it < it2; it++) {
		// get the current pick from the vector
		auto q = vPick[it];
		std::shared_ptr<CPick> pck = mPick[q.second];
		std::shared_ptr<CHypo> pickHyp = pck->getHypo();

		// if the current pick is associated to this event
		if ((pickHyp != NULL) && (pickHyp->getPid() == pidHyp)) {
			// skip to next pick
			continue;
		}

		// Add current pick to rogues list
		vRogue.push_back(pck);
	}

	return (vRogue);
}

void CPickList::processPick() {
	while (m_bRunProcessLoop == true) {
		// update thread status
		setStatus(true);

		// make sure we have a pGlass and pGlass->pHypoList
		if (pGlass == NULL) {
			// give up some time at the end of the loop
			if (m_bRunProcessLoop == true) {
				jobSleep();
			}

			// on to the next loop
			continue;
		}
		if (pGlass->getHypoList() == NULL) {
			// give up some time at the end of the loop
			if (m_bRunProcessLoop == true) {
				jobSleep();
			}

			// on to the next loop
			continue;
		}

		// check to see that we've not run to far ahead of the hypo processing
		if (pGlass->getHypoList()->getFifoSize()
				> (pGlass->getHypoList()->getNThreads() * MAX_QUEUE_FACTOR)) {
			// give up some time
			if (m_bRunProcessLoop == true) {
				jobSleep();
			}

			// on to the next loop
			continue;
		}

		// lock for queue access
		m_qProcessMutex.lock();

		// are there any jobs
		if (qProcessList.empty() == true) {
			// unlock and skip until next time
			m_qProcessMutex.unlock();

			// give up some time at the end of the loop
			if (m_bRunProcessLoop == true) {
				jobSleep();
			}

			// on to the next loop
			continue;
		}

		// get the next pick
		std::shared_ptr<CPick> pck = qProcessList.front();
		qProcessList.pop();

		// done with queue
		m_qProcessMutex.unlock();

		if (pck == NULL) {
			// give up some time at the end of the loop
			if (m_bRunProcessLoop == true) {
				jobSleep();
			}

			// on to the next loop
			continue;
		}

		// Attempt both association and nucleation of the new pick.
		// If both succeed, the mess is sorted out in darwin/evolve
		// associate
		pGlass->getHypoList()->associate(pck);

		// nucleate
		pck->nucleate();

		// give up some time at the end of the loop
		if (m_bRunProcessLoop == true) {
			jobSleep();
		}
	}

	setStatus(false);
	glassutil::CLogit::log(glassutil::log_level::info,
							"CPickList::processPick(): Thread Exit.)");
}

// ---------------------------------------------------------setStatus
void CPickList::jobSleep() {
	// if we're processing jobs
	if (m_bRunProcessLoop == true) {
		// sleep for a random amount of time, to better distribute
		// the load across all job threads.
		std::uniform_int_distribution<> distribution(m_iSleepTimeMS / 4,
														m_iSleepTimeMS);
		int sleeptime = distribution(m_RandomGenerator);
		std::this_thread::sleep_for(std::chrono::milliseconds(sleeptime));
	}
}

// ---------------------------------------------------------setStatus
void CPickList::setStatus(bool status) {
	std::lock_guard<std::mutex> statusGuard(m_StatusMutex);

	// update thread status
	if (m_ThreadStatusMap.find(std::this_thread::get_id())
			!= m_ThreadStatusMap.end()) {
		m_ThreadStatusMap[std::this_thread::get_id()] = status;
	}
}

// ---------------------------------------------------------statusCheck
bool CPickList::statusCheck() {
	// if we have a negative check interval,
	// we shouldn't worry about thread status checks.
	if (m_iStatusCheckInterval < 0)
		return (true);

	// thread is dead if we're not running
	if (m_bRunProcessLoop == false) {
		glassutil::CLogit::log(
				glassutil::log_level::warn,
				"CPickList::statusCheck(): m_bRunProcessLoop is false.");
		return (false);
	}

	// see if it's time to check
	time_t tNow;
	std::time(&tNow);
	if ((tNow - tLastStatusCheck) >= m_iStatusCheckInterval) {
		// get the thread status
		std::lock_guard<std::mutex> statusGuard(m_StatusMutex);

		// check all the threads
		std::map<std::thread::id, bool>::iterator StatusItr;
		for (StatusItr = m_ThreadStatusMap.begin();
				StatusItr != m_ThreadStatusMap.end(); ++StatusItr) {
			// get the thread status
			bool status = static_cast<bool>(StatusItr->second);

			// at least one thread did not respond
			if (status != true) {
				glassutil::CLogit::log(
						glassutil::log_level::error,
						"CPickList::statusCheck(): At least one thread"
								" did not respond in the last"
								+ std::to_string(m_iStatusCheckInterval)
								+ "seconds.");

				return (false);
			}

			// mark check as false until next time
			// if the thread is alive, it'll mark it
			// as true again.
			StatusItr->second = false;
		}

		// remember the last time we checked
		tLastStatusCheck = tNow;
	}

	// everything is awesome
	return (true);
}

const CSiteList* CPickList::getSiteList() const {
	std::lock_guard<std::recursive_mutex> pickListGuard(m_PickListMutex);
	return (pSiteList);
}

void CPickList::setSiteList(CSiteList* siteList) {
	std::lock_guard<std::recursive_mutex> pickListGuard(m_PickListMutex);
	pSiteList = siteList;
}

const CGlass* CPickList::getGlass() const {
	std::lock_guard<std::recursive_mutex> pickListGuard(m_PickListMutex);
	return (pGlass);
}

void CPickList::setGlass(CGlass* glass) {
	std::lock_guard<std::recursive_mutex> pickListGuard(m_PickListMutex);
	pGlass = glass;
}

int CPickList::getNPick() const {
	std::lock_guard<std::recursive_mutex> vPickGuard(m_vPickMutex);
	return (nPick);
}

int CPickList::getNPickMax() const {
	std::lock_guard<std::recursive_mutex> pickListGuard(m_PickListMutex);
	return (nPickMax);
}

void CPickList::setNPickMax(int picknMax) {
	std::lock_guard<std::recursive_mutex> pickListGuard(m_PickListMutex);
	nPickMax = picknMax;
}

int CPickList::getNPickTotal() const {
	std::lock_guard<std::recursive_mutex> vPickGuard(m_vPickMutex);
	return (nPickTotal);
}

int CPickList::getVPickSize() const {
	std::lock_guard<std::recursive_mutex> vPickGuard(m_vPickMutex);
	return (vPick.size());
}

}  // namespace glasscore

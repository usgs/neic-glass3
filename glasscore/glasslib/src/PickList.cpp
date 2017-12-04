#include <json.h>
#include <string>
#include <utility>
#include <memory>
#include <algorithm>
#include <cmath>
#include <map>
#include <vector>
#include <ctime>
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
	std::lock_guard<std::recursive_mutex> listGuard(m_vPickMutex);

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
bool CPickList::dispatch(json::Object *com) {
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
bool CPickList::addPick(json::Object *pick) {
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
	if ((newPick->pSite == NULL) || (newPick->tPick == 0)
			|| (newPick->sPid == "")) {
		// cleanup
		delete (newPick);
		return (false);
	}

	// check if pick is duplicate, if pGlass exists
	if (pGlass) {
		bool duplicate = checkDuplicate(newPick, pGlass->pickDuplicateWindow);

		// it is a duplicate, log and don't add pick
		if (duplicate) {
			glassutil::CLogit::log(
					glassutil::log_level::warn,
					"CPickList::addPick: Duplicate pick not passed in.");
			delete (newPick);
			return (false);
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
		nPickMax = pGlass->nPickMax;
	}

	// create pair for insertion
	std::pair<double, int> p(pck->tPick, nPick);

	// check to see if we're at the pick limit
	if (vPick.size() == nPickMax) {
		// find first pick in vector
		std::pair<double, int> pdx;
		pdx = vPick[0];
		auto pos = mPick.find(pdx.second);

		// remove pick from per site pick list
		pos->second->pSite->remPick(pos->second);

		// erase from map
		mPick.erase(pos);

		// erase from vector
		vPick.erase(vPick.begin());
	}

	// Insert new pick in proper time sequence into pick vector
	// get the index of the new pick
	int iPick = indexPick(pck->tPick);
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
	pck->pSite->addPick(pck);

	m_vPickMutex.unlock();

	// get the current size of the queue
	m_qProcessMutex.lock();
	int queueSize = qProcessList.size();
	m_qProcessMutex.unlock();

	// wait until there's space in the queue
	// we don't want to build up a huge queue of unprocessed
	// picks
	if ((pGlass) && (pGlass->pHypoList)) {
		while (queueSize >= (m_iNumThreads)) {
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

	// we're done
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

	// handle pick earlier than first element case
	// time is earlier than first pick
	if (tPick < vPick[0].first) {
		// return -1 to indicate earlier than first element
		return (-1);
	}

	// handle case that the pick is later than last element
	int i1 = 0;
	int i2 = vPick.size() - 1;

	// time is after last pick
	if (tPick >= vPick[i2].first) {
		// return index of last element
		return (i2);
	}

	// search for insertion point within vector
	// using a binary search
	int ix;

	// while upper minus lower bounds is greater than one
	while ((i2 - i1) > 1) {
		// compute current pick index
		ix = (i1 + i2) / 2;

		// if time is before current pick
		if (vPick[ix].first > tPick) {
			// new upper bound is this index
			i2 = ix;
		} else if (vPick[ix].first <= tPick) {
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
	std::lock_guard<std::recursive_mutex> listGuard(m_vPickMutex);

	// null checks
	if (newPick == NULL) {
		return (false);
	}
	if (window == 0.0) {
		return (false);
	}

	// set default return to no match
	bool matched = false;

	// get the index of the earliest possible match
	int it1 = indexPick(newPick->tPick - window);

	// get index of the latest possible pick
	int it2 = indexPick(newPick->tPick + window);

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
		if (std::abs(newPick->tPick - pck->tPick) < window) {
			// check if sites match
			if (newPick->pSite->sScnl == pck->pSite->sScnl) {
				// if match is found, set to true, log, and break out of loop
				matched = true;
				glassutil::CLogit::log(
						glassutil::log_level::warn,
						"CPickList::checkDuplicat: Duplicate (window = "
								+ std::to_string(window) + ") : old:"
								+ pck->pSite->sScnl + " "
								+ std::to_string(pck->tPick) + " new(del):"
								+ newPick->pSite->sScnl + " "
								+ std::to_string(newPick->tPick));
				break;
			}
		}
	}

	return (matched);
}

// ---------------------------------------------------------scavenge
bool CPickList::scavenge(std::shared_ptr<CHypo> hyp, double tDuration) {
	std::lock_guard<std::recursive_mutex> listGuard(m_vPickMutex);

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
							"CPickList::scavenge. " + hyp->sPid);

	// Calculate range for possible associations
	double sdassoc = pGlass->sdAssociate;

	// get the index of the pick to start with
	// based on the hypo origin time
	int it1 = indexPick(hyp->tOrg);

	// index can't be negative
	// Primarily occurs if origin time is before first pick
	if (it1 < 0) {
		it1 = 0;
	}

	// get the index of the pick to end with by using the hypo
	// origin time plus the provided duration
	int it2 = indexPick(hyp->tOrg + tDuration);

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
		std::shared_ptr<CHypo> pickHyp = pck->pHypo;

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
			if (pGlass->bTrack) {
				snprintf(
						sLog, sizeof(sLog), "WAF %s %s %s (%d)\n",
						hyp->sPid.substr(0, 4).c_str(),
						glassutil::CDate::encodeDateTime(pck->tPick).c_str(),
						pck->pSite->sScnl.c_str(),
						static_cast<int>(hyp->vPick.size()));
				glassutil::CLogit::Out(sLog);
			}

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
			"CPickList::scavenge " + hyp->sPid + " added:"
					+ std::to_string(addCount));

	// return whether we've associated at least one pick
	return (bAss);
}

// ---------------------------------------------------------rogues
std::vector<std::shared_ptr<CPick>> CPickList::rogues(std::string pidHyp,
														double tOrg,
														double tDuration) {
	std::lock_guard<std::recursive_mutex> listGuard(m_vPickMutex);

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
		std::shared_ptr<CHypo> pickHyp = pck->pHypo;

		// if the current pick is associated to this event
		if ((pickHyp != NULL) && (pickHyp->sPid == pidHyp)) {
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
		// make sure we're still running
		if (m_bRunProcessLoop == false)
			break;

		// update thread status
		setStatus(true);

		// make sure we have a pGlass and pGlass->pHypoList
		if (pGlass == NULL) {
			// give up some time at the end of the loop
			jobSleep();

			// on to the next loop
			continue;
		}
		if (pGlass->pHypoList == NULL) {
			// give up some time at the end of the loop
			jobSleep();

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
			jobSleep();

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
			jobSleep();

			// on to the next loop
			continue;
		}

		// Attempt both association and nucleation of the new pick.
		// If both succeed, the mess is sorted out in darwin/evolve
		// associate
		pGlass->pHypoList->associate(pck);

		// nucleate
		pck->nucleate();

		// give up some time at the end of the loop
		jobSleep();
	}

	setStatus(false);
	glassutil::CLogit::log(glassutil::log_level::info,
							"CPickList::processPick(): Thread Exit.)");
}

// ---------------------------------------------------------setStatus
void CPickList::jobSleep() {
	if (m_bRunProcessLoop == true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(m_iSleepTimeMS));
	}
}

// ---------------------------------------------------------setStatus
void CPickList::setStatus(bool status) {
	// update thread status
	m_StatusMutex.lock();
	if (m_ThreadStatusMap.find(std::this_thread::get_id())
			!= m_ThreadStatusMap.end()) {
		m_ThreadStatusMap[std::this_thread::get_id()] = status;
	}
	m_StatusMutex.unlock();
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
		m_StatusMutex.lock();
		std::map<std::thread::id, bool>::iterator StatusItr;
		for (StatusItr = m_ThreadStatusMap.begin();
				StatusItr != m_ThreadStatusMap.end(); ++StatusItr) {
			// get the thread status
			std::thread::id threadid = StatusItr->first;
			bool status = static_cast<bool>(StatusItr->second);

			// at least one thread did not respond
			if (status != true) {
				m_StatusMutex.unlock();

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
		m_StatusMutex.unlock();

		// remember the last time we checked
		tLastStatusCheck = tNow;
	}

	// everything is awesome
	return (true);
}
}  // namespace glasscore

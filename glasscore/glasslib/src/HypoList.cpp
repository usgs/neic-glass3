#include <json.h>
#include <string>
#include <memory>
#include <utility>
#include <algorithm>
#include <vector>
#include <map>
#include <ctime>
#include "Date.h"
#include "Site.h"
#include "Pick.h"
#include "Correlation.h"
#include "Hypo.h"
#include "Glass.h"
#include "Web.h"
#include "SiteList.h"
#include "PickList.h"
#include "HypoList.h"
#include "CorrelationList.h"
#include "Logit.h"
#include "Pid.h"

namespace glasscore {

// sort functions
bool sortHypo(const std::pair<double, std::string> &lhs,
				const std::pair<double, std::string> &rhs) {
	if (lhs.first < rhs.first)
		return (true);

	return (false);
}

// ---------------------------------------------------------CHypoList
CHypoList::CHypoList(int numThreads, int sleepTime, int checkInterval) {
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
		vProcessThreads.push_back(std::thread(&CHypoList::processHypos, this));

		// add to status map if we're tracking status
		if (m_iStatusCheckInterval > 0) {
			m_StatusMutex.lock();
			m_ThreadStatusMap[vProcessThreads[i].get_id()] = true;
			m_StatusMutex.unlock();
		}
	}
}

// ---------------------------------------------------------~CHypoList
CHypoList::~CHypoList() {
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

// ---------------------------------------------------------clear
void CHypoList::clear() {
	clearHypos();
	bSendEvent = false;

	pGlass = NULL;
}

// ---------------------------------------------------------clearHypos
void CHypoList::clearHypos() {
	std::lock_guard<std::mutex> queueGuard(m_QueueMutex);
	qFifo.clear();

	std::lock_guard<std::recursive_mutex> listGuard(m_vHypoMutex);
	vHypo.clear();
	mHypo.clear();

	// reset nHypo
	nHypo = 0;
	nHypoTotal = -1;
	nHypoMax = 100;
}

// ---------------------------------------------------------Dispatch
bool CHypoList::dispatch(json::Object *com) {
	// null check json
	if (com == NULL) {
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"CHypoList::dispatch: NULL json communication.");
		return (false);
	}

	// check for a command
	if (com->HasKey("Cmd")
			&& ((*com)["Cmd"].GetType() == json::ValueType::StringVal)) {
		// dispatch to appropriate function based on Cmd value
		json::Value v = (*com)["Cmd"].ToString();

		// clear all hypos
		if (v == "ClearGlass") {
			clearHypos();

			// ClearGlass is also relevant to other glass
			// components, return false so they also get a
			// chance to process it
			return (false);
		}

		// a hypo message has been requested
		if (v == "ReqHypo") {
			return (reqHypo(com));
		}
	}

	// this communication was not handled
	return (false);
}

// ---------------------------------------------------------indexHypo
int CHypoList::indexHypo(double tOrg) {
	std::lock_guard<std::recursive_mutex> listGuard(m_vHypoMutex);

	// handle empty vector case
	if (vHypo.size() == 0) {
		// return -2 to indicate empty vector
		return (-2);
	}

	double tFirstOrg = vHypo[0].first;

	// handle origin earlier than first element case
	// time is earlier than first origin
	if (tOrg < tFirstOrg) {
		// return -1 to indicate earlier than first element
		return (-1);
	}

	// handle case that the origin is later than last element
	int i1 = 0;
	int i2 = vHypo.size() - 1;
	double tLastOrg = vHypo[i2].first;
	// time is after last origin
	if (tOrg >= tLastOrg) {
		return (i2);
	}

	// search for insertion point within vector
	// using a binary search
	// while upper minus lower bounds is greater than one
	while ((i2 - i1) > 1) {
		// compute current origin index
		int ix = (i1 + i2) / 2;
		double tCurrentOrg = vHypo[ix].first;

		// if time is before current origin
		if (tCurrentOrg > tOrg) {
			// new upper bound is this index
			i2 = ix;
		} else {  // if (tCurrentOrg <= tOrg)
			// if time is after or equal to current origin
			// new lower bound is this index
			i1 = ix;
		}
	}

	// return the last lower bound as the insertion point
	return (i1);
}

// ---------------------------------------------------------listPicks
void CHypoList::listHypos() {
	std::lock_guard<std::recursive_mutex> listGuard(m_vHypoMutex);

	int n = 0;
	char sLog[1024];

	// for each hypo
	for (auto p : vHypo) {
		// list it
		snprintf(sLog, sizeof(sLog), "%d: %.2f %s", n++, p.first,
					p.second.c_str());
		glassutil::CLogit::Out(sLog);
	}
}

// ---------------------------------------------------------pushFifo
int CHypoList::pushFifo(std::shared_ptr<CHypo> hyp) {
	std::lock_guard<std::mutex> queueGuard(m_QueueMutex);
	// nullcheck
	if (hyp == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypoList::pushFifo: NULL hypo provided.");

		// return the current size of the queue
		int size = qFifo.size();

		return (size);
	}

	// get this hypo's id
	std::string pid = hyp->getPid();

	// is this id already on the queue?
	if (std::find(qFifo.begin(), qFifo.end(), pid) == qFifo.end()) {
		// it is not, add it
		qFifo.push_back(pid);
	}

	// return the current size of the queue
	int size = qFifo.size();

	glassutil::CLogit::log(
			glassutil::log_level::debug,
			"CHypoList::pushFifo: sPid:" + pid + " " + std::to_string(size)
					+ " hypos in queue.");

	return (size);
}

// ---------------------------------------------------------popFifo
std::shared_ptr<CHypo> CHypoList::popFifo() {
	// std::lock_guard<std::mutex> queueGuard(m_QueueMutex);
	m_QueueMutex.lock();

	// Pop first hypocenter off processing fifo
	// is there anything on the queue?
	if (qFifo.size() < 1) {
		// nope
		m_QueueMutex.unlock();
		return (NULL);
	}

	// get the first id on the queue
	std::string pid = qFifo.front();

	// remove the first id from the queue now that we have it
	qFifo.erase(qFifo.begin());

	m_QueueMutex.unlock();

	m_vHypoMutex.lock();

	// use the map to get the hypo based on the id
	std::shared_ptr<CHypo> hyp = mHypo[pid];

	m_vHypoMutex.unlock();

	// return the hypo
	return (hyp);
}

int CHypoList::getFifoSize() {
	std::lock_guard<std::mutex> queueGuard(m_QueueMutex);

	// return the current size of the queue
	int size = qFifo.size();
	return (size);
}

// ---------------------------------------------------------addHypo
bool CHypoList::addHypo(std::shared_ptr<CHypo> hypo, bool scheduleProcessing) {
	std::lock_guard<std::recursive_mutex> listGuard(m_vHypoMutex);

	// nullcheck
	if (hypo == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypoList::addHypo: NULL hypo provided.");

		return (false);
	}

	// set some basic hypo values from pGlass if we have it
	if (pGlass) {
		hypo->setGlass(pGlass);
		hypo->setCutFactor(pGlass->dCutFactor);
		hypo->setCutPercentage(pGlass->dCutPercentage);
		hypo->setCutMin(pGlass->dCutMin);
	}

	// Add hypo to cache (mHypo) and time sorted
	// index (vHypo). If vHypo has reached its
	// maximum capacity (nHypoMax), then the
	// first hypo in the vector is removed and the
	// corresponding entry in the cache is erased.
	nHypoTotal++;
	nHypo++;

	// get maximum number of hypos
	// use max picks from pGlass if we have it
	if (pGlass) {
		nHypoMax = pGlass->nHypoMax;
	}

	// create pair for insertion
	std::pair<double, std::string> p(hypo->getTOrg(), hypo->getPid());

	// remove oldest hypo if this new one
	// pushes us over the limit
	if (vHypo.size() == nHypoMax) {
		// get first hypo in vector
		std::pair<double, std::string> pdx;
		pdx = vHypo[0];
		std::shared_ptr<CHypo> firstHypo = mHypo[pdx.second];

		// remove it
		remHypo(firstHypo, false);

		glassutil::CLogit::log(
				glassutil::log_level::debug,
				"CHypoList::addHypo: Current: "
						+ std::to_string(static_cast<int>(vHypo.size()))
						+ " Max: " + std::to_string(nHypoMax)
						+ " Removing Hypo: " + pdx.second);

		// create expiration message
		json::Object expire;
		expire["Cmd"] = "Expire";
		expire["Pid"] = pdx.second;

		// send message
		if (pGlass) {
			pGlass->send(&expire);
		}
	}

	// Insert new hypo in proper time sequence into hypo vector
	// get the index of the new hypo
	int ihypo = indexHypo(hypo->getTOrg());
	switch (ihypo) {
		case -2:
			// Empty vector, just add it
			vHypo.push_back(p);
			break;
		case -1:
			// hypo is before any others, insert at beginning
			vHypo.insert(vHypo.begin(), p);
			break;
		default:
			// hypo is somewhere in vector
			if (ihypo == vHypo.size() - 1) {
				// hypo is after all other hypos, add to end
				vHypo.push_back(p);
			} else {
				// find where the hypo should be inserted
				auto it = std::next(vHypo.begin(), ihypo + 1);

				// insert at that location
				vHypo.insert(it, p);
			}
			break;
	}
	// add to hypo map
	mHypo[hypo->getPid()] = hypo;

	// Schedule this hypo for refinement. Note that this
	// hypo will be the first one in the queue, and will be the
	// first one processed.
	if (scheduleProcessing == true) {
		pushFifo(hypo);
	}

	// done
	return (true);
}

// ---------------------------------------------------------remHypo
// Remove and unmap Hypocenter from vector, map, and pick
void CHypoList::remHypo(std::shared_ptr<CHypo> hypo, bool reportCancel) {
	m_vHypoMutex.lock();

	// nullcheck
	if (hypo == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypoList::remHypo: NULL hypo provided.");

		return;
	}

	// get the id
	std::string pid = hypo->getPid();

	// unlink all the hypo's data
	hypo->clearPicks();
	hypo->clearCorrelations();

	// erase this hypo from the vector
	// search through all hypos
	for (int iq = 0; iq < vHypo.size(); iq++) {
		// get current hypo
		auto q = vHypo[iq];

		// if the current hypo matches the id of the
		// hypo to delete
		if (q.second == pid) {
			// erase this hypo
			vHypo.erase(vHypo.begin() + iq);

			// done
			break;
		}
	}

	// erase this hypo from the map
	mHypo.erase(pid);

	m_vHypoMutex.unlock();

	// Send cancellation message for this hypo if we've sent an event
	// message
	if (reportCancel == true) {
		if (hypo->getEvent()) {
			// create cancellation message
			json::Object can;
			can["Cmd"] = "Cancel";
			can["Pid"] = pid;

			// send message
			if (pGlass) {
				pGlass->send(&can);
			}
		}
	}
}

// ---------------------------------------------------------findHypo
std::shared_ptr<CHypo> CHypoList::findHypo(double t1, double t2) {
	std::lock_guard<std::recursive_mutex> listGuard(m_vHypoMutex);

	// don't bother if the list is empty
	if (vHypo.size() == 0) {
		return (NULL);
	}

	// get the index of the starting time of the selection range
	int ix1 = indexHypo(t1);

	// check starting index
	if (ix1 < 0) {
		// start time is before start of list, set to first index
		ix1 = 0;
	}

	if (ix1 >= vHypo.size()) {
		// starting index is greater than or equal to the size of the list,
		// no hypos to find
		return (NULL);
	}

	// get the index of the ending time of the selection range
	int ix2 = indexHypo(t2);

	// check ending index
	if (ix2 < 0) {
		return (NULL);
	}

	// based on the the next index after the starting index, check for a valid
	// hypo that doesn't exceed the end of the time range
	if (vHypo[ix1 + 1].first < t2) {
		// the hypo is not past the ending time of the selection range
		// get the id
		std::string pid = vHypo[ix1 + 1].second;

		// using the id, return the hypo
		return (mHypo[pid]);
	}

	// no valid hypo found
	return (NULL);
}

// ---------------------------------------------------------associate
bool CHypoList::associate(std::shared_ptr<CPick> pk) {
	std::lock_guard<std::recursive_mutex> listGuard(m_vHypoMutex);

	// nullcheck
	if (pk == NULL) {
		glassutil::CLogit::log(glassutil::log_level::warn,
								"CHypoList::associate: NULL pick provided.");

		return (false);
	}

	if (pGlass == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypoList::associate: NULL pGlass.");
		return (false);
	}

	// are there any hypos to associate with?
	if (vHypo.size() < 1) {
		// nope
		return (false);
	}

	glassutil::CLogit::log(
			glassutil::log_level::debug,
			"CHypoList::associate idPick:" + std::to_string(pk->getIdPick()));

	std::string pid;
	std::vector<std::shared_ptr<CHypo>> viper;

	// compute the index range to search for hypos to associate with
	// (a potential hypo must be before the pick we're associating)
	// use the pick time minus 2400 seconds to compute the starting index
	// NOTE: Hard coded time delta
	int it1 = indexHypo(pk->getTPick() - 2400.0);

	// check to see the index indicates that the time is before the
	// start of the hypo list
	if (it1 < 0) {
		// set the starting index to the beginning of the hypo list
		it1 = 0;
	}

	// get the ending index based on the pick time (a potential hypo can't
	// be after the pick we're associating)
	int it2 = indexHypo(pk->getTPick());

	std::string pidmax;
	double sdassoc = pGlass->sdAssociate;

	// for each hypo in the list within the
	// time range
	for (int it = it1; it <= it2; it++) {
		// get this hypo id
		pid = vHypo[it].second;

		// get this hypo based on the id
		std::shared_ptr<CHypo> hyp = mHypo[pid];

		// check to see if the pick will associate with
		// this hypo
		// NOTE: The sigma value passed into associate is hard coded
		if (hyp->associate(pk, 1.0, sdassoc)) {
			// add to the list of hypos this pick can associate with
			viper.push_back(hyp);

			// remember this id for later
			pidmax = hyp->getPid();
		}
	}

	// there were no hypos that the pick associated with
	if (viper.size() < 1) {
		return (false);
	}

	// there was only one hypo that the pick associated with
	if (viper.size() == 1) {
		// get the lucky hypo
		std::shared_ptr<CHypo> hyp = mHypo[pidmax];

		// log
		if (pGlass->bTrack) {
			char sLog[1024];
			snprintf(
					sLog, sizeof(sLog), "ASS %s %s %s (%d)\n",
					hyp->getPid().c_str(),
					glassutil::CDate::encodeDateTime(pk->getTPick()).c_str(),
					pk->getSite()->getScnl().c_str(),
					static_cast<int>(hyp->getVPickSize()));
			glassutil::CLogit::Out(sLog);
		}

		// link the pick to the hypo
		pk->addHypo(hyp, "", true);

		// link the hypo to the pick
		hyp->addPick(pk);

		glassutil::CLogit::log(
				glassutil::log_level::debug,
				"CHypoList::associate (pick) sPid:" + hyp->getPid()
						+ " resetting cycle count due to new association");

		// reset the cycle count
		hyp->setCycle(0);

		// add to the processing queue
		pushFifo(hyp);

		// the pick was associated
		return (true);
	}

	// For each hypo that the pick could associate with
	for (auto q : viper) {
		glassutil::CLogit::log(
				glassutil::log_level::debug,
				"CHypoList::associate (pick) sPid:" + q->getPid()
						+ " resetting cycle count due to new association");

		// reset the cycle count
		q->setCycle(0);

		// add the hypo to the processing queue
		// note that we didn't link the pick to any hypos
		pushFifo(q);
	}

	// the pick was associated
	return (true);
}

// ---------------------------------------------------------associate
bool CHypoList::associate(std::shared_ptr<CCorrelation> corr) {
	std::lock_guard<std::recursive_mutex> listGuard(m_vHypoMutex);

	// nullcheck
	if (corr == NULL) {
		glassutil::CLogit::log(
				glassutil::log_level::warn,
				"CHypoList::associate: NULL correlation provided.");

		return (false);
	}

	if (pGlass == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypoList::associate: NULL pGlass.");
		return (false);
	}

	// are there any hypos to associate with?
	if (vHypo.size() < 1) {
		// nope
		return (false);
	}

	std::shared_ptr<CHypo> hyp;
	std::string pid;
	bool bass = false;
	std::vector<std::shared_ptr<CHypo>> viper;

	// compute the index range to search for hypos to associate with
	// (a potential hypo must be before the pick we're associating)
	// use the correlation time minus correlationMatchingTWindow to compute the
	// starting index
	// NOTE: There is a potential issue with this. The time sequence order of
	// vHypo is based on the initial value of tOrg when the hypo is first added
	// to the list. Various processing steps (relocation, etc) could change
	// the tOrg, but vHypo is never resorted. Caryl agrees that there
	// *could* be issues here, but has no idea how significant they are.
	// Could affect association to splits with similar origin times.
	int it1 = indexHypo(
			corr->getTCorrelation() - pGlass->correlationMatchingTWindow);

	// check to see the index indicates that the time is before the
	// start of the hypo list
	if (it1 < 0) {
		// set the starting index to the beginning of the hypo list
		it1 = 0;
	}

	// get the ending index based on the correlation time plus
	// correlationMatchingTWindow
	int it2 = indexHypo(
			corr->getTCorrelation() + pGlass->correlationMatchingTWindow);

	std::string pidmax;

	// for each hypo in the list within the
	// time range
	for (int it = it1; it <= it2; it++) {
		// get this hypo id
		pid = vHypo[it].second;

		// get this hypo based on the id
		hyp = mHypo[pid];

		// check to see if the correlation will associate with
		// this hypo
		if (hyp->associate(corr, pGlass->correlationMatchingTWindow,
							pGlass->correlationMatchingXWindow)) {
			// add to the list of hypos this correlation can associate with
			viper.push_back(hyp);

			// remember this id for later
			pidmax = hyp->getPid();
		}
	}

	// there were no hypos that the correlation associated with
	if (viper.size() < 1) {
		return (false);
	}

	// there was only one hypo that the correlation associated with
	if (viper.size() == 1) {
		// get the lucky hypo
		hyp = mHypo[pidmax];

		// log
		if (pGlass->bTrack) {
			char sLog[1024];
			snprintf(
					sLog,
					sizeof(sLog),
					"C-ASS %s %s %s (%d)\n",
					hyp->getPid().substr(0, 4).c_str(),
					glassutil::CDate::encodeDateTime(corr->getTCorrelation()).c_str(),
					corr->getSite()->getScnl().c_str(),
					static_cast<int>(hyp->getVCorrSize()));
			glassutil::CLogit::Out(sLog);
		}

		// link the correlation to the hypo
		corr->addHypo(hyp, "", true);

		// link the hypo to the correlation
		hyp->addCorrelation(corr);

		glassutil::CLogit::log(
				glassutil::log_level::debug,
				"CHypoList::associate (correlation) sPid:" + hyp->getPid()
						+ " resetting cycle count due to new association");

		// reset the cycle count
		hyp->setCycle(0);

		// add to the processing queue
		pushFifo(hyp);

		// the pick was associated
		return (true);
	}

	// For each hypo that the correlation could associate with
	for (auto q : viper) {
		glassutil::CLogit::log(
				glassutil::log_level::debug,
				"CHypoList::associate (correlation) sPid:" + q->getPid()
						+ " resetting cycle count due to new association");

		// reset the cycle count
		q->setCycle(0);

		// add the hypo to the processing queue
		// note that we didn't link the correlation to any hypos
		pushFifo(q);
	}

	// the pick was associated
	return (true);
}

// ---------------------------------------------------------resolve
bool CHypoList::resolve(std::shared_ptr<CHypo> hyp) {
	std::lock_guard<std::recursive_mutex> listGuard(m_vHypoMutex);

	// null checks
	if (hyp == NULL) {
		glassutil::CLogit::log(glassutil::log_level::warn,
								"CHypoList::resolve: NULL hypo provided.");

		return (false);
	}

	if (pGlass == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypoList::resolve: NULL pGlass.");
		return (false);
	}

	// return whether we've changed the pick set
	return (hyp->resolve(hyp));
}

// ---------------------------------------------------------darwin
void CHypoList::darwin() {
	if (pGlass == NULL) {
		return;
	}

	// don't bother if there's nothing to do
	if (getFifoSize() < 1) {
		return;
	}

	// log the cycle count and queue size
	char sLog[1024];

	// update thread status
	setStatus(true);

	// get the next hypo to process
	std::shared_ptr<CHypo> hyp = popFifo();

	// check to see if we got a valid hypo
	if (!hyp) {
		// nothing to process, move on
		return;
	}

	// log the hypo we're working on
	glassutil::CLogit::log(
			glassutil::log_level::debug,
			"CHypoList::darwin Processing Hypo sPid:" + hyp->getPid()
					+ " Cycle:" + std::to_string(hyp->getCycle())
					+ " Fifo Size:" + std::to_string(getFifoSize()));

	// check to see if this hypo is viable.
	if (hyp->cancel()) {
		// this hypo is no longer viable
		// log
		glassutil::CLogit::log(
				glassutil::log_level::debug,
				"CHypoList::darwin canceling sPid:" + hyp->getPid()
						+ " processCount:"
						+ std::to_string(hyp->getProcessCount()));

		// remove hypo from the hypo list
		remHypo(hyp);

		sort();

		// done with processing
		return;
	}

	// check to see if we've hit the iCycle Limit for this
	// hypo
	if (hyp->getCycle() >= pGlass->iCycleLimit) {
		// log
		glassutil::CLogit::log(
				glassutil::log_level::debug,
				"CHypoList::darwin skipping sPid:" + hyp->getPid()
						+ " at cycle limit:" + std::to_string(hyp->getCycle())
						+ +" processCount:"
						+ std::to_string(hyp->getProcessCount()));
		return;
	}

	// process this hypocenter
	evolve(hyp);

	// resort the hypocenter list to maintain
	// time order
	sort();
}

// ---------------------------------------------------------evolve
bool CHypoList::evolve(std::shared_ptr<CHypo> hyp, int announce) {
	// nullcheck
	if (hyp == NULL) {
		glassutil::CLogit::log(glassutil::log_level::warn,
								"CHypoList::evolve: NULL hypo provided.");
		return (false);
	}
	if (pGlass == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypoList::evolve: NULL pGlass.");
		return (false);
	}

	std::string pid = hyp->getPid();

	std::chrono::high_resolution_clock::time_point tEvolveStartTime =
			std::chrono::high_resolution_clock::now();

	hyp->incrementProcessCount();
	hyp->setCycle(hyp->getCycle() + 1);

	// initialize breport, gets set to true later if event isn't cancelled.
	// otherwise, in some cases events will not be reported.
	bool breport = false;

	// locate the hypo
	hyp->localize();

	std::chrono::high_resolution_clock::time_point tLocalizeEndTime =
			std::chrono::high_resolution_clock::now();
	double localizeTime = std::chrono::duration_cast<
			std::chrono::duration<double>>(tLocalizeEndTime - tEvolveStartTime)
			.count();

	// Search for any associable picks that match hypo in the pick list
	// NOTE: This uses the hard coded 2400 second scavenge duration default
	if (pGlass->pPickList->scavenge(hyp)) {
		// we should report this hypo since it has changed
		breport = true;

		// relocate the hypo
		hyp->localize();
	}

	// search for any associable correlations that match hypo in the correlation
	// list
	if (pGlass->pCorrelationList->scavenge(hyp)) {
		// we should report this hypo since it has changed
		breport = true;

		// relocate the hypo
		hyp->localize();
	}

	std::chrono::high_resolution_clock::time_point tScavengeEndTime =
			std::chrono::high_resolution_clock::now();
	double scavengeTime = std::chrono::duration_cast<
			std::chrono::duration<double>>(tScavengeEndTime - tLocalizeEndTime)
			.count();

	// Ensure all data belong to hypo
	if (resolve(hyp)) {
		// we should report this hypo since it has changed
		breport = true;

		// relocate the hypo
		hyp->localize();
	}

	std::chrono::high_resolution_clock::time_point tResolveEndTime =
			std::chrono::high_resolution_clock::now();
	double resolveTime = std::chrono::duration_cast<
			std::chrono::duration<double>>(tResolveEndTime - tScavengeEndTime)
			.count();

	// Remove data that no longer fit hypo's association criteria
	if (hyp->prune()) {
		// we should report this hypo since it has changed
		breport = true;

		// relocate the hypo
		hyp->localize();
	}

	std::chrono::high_resolution_clock::time_point tPruneEndTime =
			std::chrono::high_resolution_clock::now();
	double pruneTime =
			std::chrono::duration_cast<std::chrono::duration<double>>(
					tPruneEndTime - tResolveEndTime).count();

	/*// Check to see if event can be merged into another
	 if (merge(hyp))
	 {
	 return (false);
	 }
	 */

	// check to see if this hypo is viable.
	if (hyp->cancel()) {
		std::chrono::high_resolution_clock::time_point tCancelEndTime =
				std::chrono::high_resolution_clock::now();
		double cancelTime = std::chrono::duration_cast<
				std::chrono::duration<double>>(tCancelEndTime - tPruneEndTime)
				.count();

		remHypo(hyp);

		std::chrono::high_resolution_clock::time_point tRemoveEndTime =
				std::chrono::high_resolution_clock::now();
		double removeTime = std::chrono::duration_cast<
				std::chrono::duration<double>>(tRemoveEndTime - tCancelEndTime)
				.count();

		double evolveTime = std::chrono::duration_cast<
				std::chrono::duration<double>>(
				tRemoveEndTime - tEvolveStartTime).count();

		glassutil::CLogit::log(
				glassutil::log_level::debug,
				"CHypoList::evolve: Canceled sPid:" + pid + " cycle:"
						+ std::to_string(hyp->getCycle()) + " processCount:"
						+ std::to_string(hyp->getProcessCount())
						+ " Evolve Timing: localizeTime:"
						+ std::to_string(localizeTime) + " scavengeTime:"
						+ std::to_string(scavengeTime) + " resolveTime:"
						+ std::to_string(resolveTime) + " pruneTime:"
						+ std::to_string(pruneTime) + " cancelTime:"
						+ std::to_string(cancelTime) + " removeTime:"
						+ std::to_string(removeTime) + " evolveTime:"
						+ std::to_string(evolveTime));

		// return false since the hypo was canceled.
		return (false);
	}

	std::chrono::high_resolution_clock::time_point tCancelEndTime =
			std::chrono::high_resolution_clock::now();
	double cancelTime =
			std::chrono::duration_cast<std::chrono::duration<double>>(
					tCancelEndTime - tPruneEndTime).count();

	// report if asked
	// NOTE: why?
	if (announce == 1) {
		breport = true;
	}

	// announce if a correlation has been added to an existing event
	// NOTE: Is there a better way to do this?
	if ((hyp->getCorrAdded() == true) && (hyp->getVPickSize() >= hyp->getCut())) {
		breport = true;
		hyp->setCorrAdded(false);
	} else {
		hyp->setCorrAdded(false);
	}

	// if we're supposed to report
	if (breport) {
		// if we CAN report
		if (hyp->reportCheck() == true) {
			// report
			hyp->event();
		}
	}

	std::chrono::high_resolution_clock::time_point tReportEndTime =
			std::chrono::high_resolution_clock::now();
	double reportTime =
			std::chrono::duration_cast<std::chrono::duration<double>>(
					tReportEndTime - tCancelEndTime).count();

	// check for and log any miss-linked picks
	hyp->trap();

	std::chrono::high_resolution_clock::time_point tTrapEndTime =
			std::chrono::high_resolution_clock::now();
	double trapTime = std::chrono::duration_cast<std::chrono::duration<double>>(
			tTrapEndTime - tReportEndTime).count();

	double evolveTime =
			std::chrono::duration_cast<std::chrono::duration<double>>(
					tTrapEndTime - tEvolveStartTime).count();

	glassutil::CLogit::log(
			glassutil::log_level::debug,
			"CHypoList::evolve: Finished sPid:" + pid + " cycle:"
					+ std::to_string(hyp->getCycle()) + " processCount:"
					+ std::to_string(hyp->getProcessCount())
					+ " Evolve Timing: localizeTime:"
					+ std::to_string(localizeTime) + " scavengeTime:"
					+ std::to_string(scavengeTime) + " resolveTime:"
					+ std::to_string(resolveTime) + " pruneTime:"
					+ std::to_string(pruneTime) + " cancelTime:"
					+ std::to_string(cancelTime) + " reportTime:"
					+ std::to_string(reportTime) + " trapTime:"
					+ std::to_string(trapTime) + " evolveTime:"
					+ std::to_string(evolveTime));

	// the hypo survived, so return true
	return (true);
}
/*
// ---------------------------------------------------------merge
bool CHypoList::merge(std::shared_ptr<CHypo> hypo) {
	std::lock_guard<std::recursive_mutex> listGuard(m_vHypoMutex);

	if (pGlass == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypoList::merge: NULL pGlass.");
		return (false);
	}

	// are there any hypos to associate with?
	if (vHypo.size() < 1) {
		// nope
		return (false);
	}

	char sLog[1024];
	double distanceCut = 2.;
	double timeCut = 60.;
	double delta;
	glassutil::CGeo geo;
	geo.setGeographic(hypo->getLat(), hypo->getLon(), 6371.0);
	std::shared_ptr<CHypo> hypo2;
	int itHypo = indexHypo(hypo->getTOrg());

	int it1 = indexHypo(hypo->getTOrg() - timeCut);

	// check to see the index indicates that the time is before the
	// start of the hypo list
	if (it1 < 0) {
		// set the starting index to the beginning of the hypo list
		it1 = 0;
	}

	// get the ending index based on the pick time (a potential hypo can't
	// be after the pick we're associating)
	int it2 = indexHypo(hypo->getTOrg() + timeCut);

	std::string pidmax;

	// for each hypo in the list within the
	// time range
	for (int it = it1; it <= it2; it++) {
		if (it != itHypo) {
			// get this hypo id
			std::string pid = vHypo[it].second;
			// get this hypo based on the id
			hypo2 = mHypo[pid];
			glassutil::CGeo geo2;
			geo2.setGeographic(hypo2->getLat(), hypo2->getLon(), 6371.0);

			// check distance between events
			delta = geo.delta(&geo2);
			if (delta < distanceCut
					&& (hypo->vPick.size() <= hypo2->vPick.size())
					&& hypo->getPid() != hypo2->getPid()) {
				snprintf(
						sLog, sizeof(sLog),
						"CHypoList::merge: Potentially Merging %s into %s\n",
						hypo->getPid().c_str(), hypo2->getPid().c_str());
				glassutil::CLogit::log(sLog);

				std::shared_ptr<CHypo> hypo3 = std::make_shared<CHypo>(
						(hypo2->getLat() + hypo->getLat()) / 2.,
						(hypo2->getLon() + hypo->getLon()) / 2.,
						(hypo2->getZ() + hypo->getZ()) / 2.,
						(hypo2->getTOrg() + hypo->getTOrg()) / 2.,
						glassutil::CPid::pid(), hypo2->sWeb, hypo2->getBayes(),
						hypo2->dThresh, hypo2->nCut, hypo2->pTrv1, hypo2->pTrv2,
						pGlass->pTTT);

				// set hypo glass pointer and such
				hypo3->pGlass = pGlass;
				hypo3->dCutFactor = pGlass->dCutFactor;
				hypo3->dCutPercentage = pGlass->dCutPercentage;
				hypo3->dCutMin = pGlass->dCutMin;

				// add all picks for other two events
				for (auto pick : hypo->vPick) {
					// they're not associated yet, just potentially
					pick->setAss("N");
					hypo3->addPick(pick);
				}

				for (auto pick : hypo2->vPick) {
					// they're not associated yet, just potentially
					pick->setAss("N");
					hypo3->addPick(pick);
				}

				// First localization attempt after nucleation
				// make 3 passes

				// hypo3->anneal(); need to flush out
				hypo3->localize();

				if (pGlass->pPickList->scavenge(hypo3)) {
					// relocate the hypo
					hypo3->localize();
				}

				// Remove picks that no longer fit hypo's association criteria
				if (hypo3->prune()) {
					// relocate the hypo
					hypo3->localize();
				}

				int npick = hypo3->vPick.size();


				 for (int ipass = 0; ipass < 3; ipass++) {

				 // get an initial location via synthetic annealing,
				 // which also prunes out any poorly fitting picks
				 hypo3->localize(500, 100.0, 1.0);

				 // get the number of picks we have now
				 npick = hypo3->vPick.size();

				 snprintf(sLog, sizeof(sLog), "CHypoList::merge: -- New Potential Event Passed %d %d/%d", ipass, npick,
				 ncut);
				 glassutil::CLogit::log(sLog);
				 }

				// check that it held onto picks delete other two events
				if (npick > 0.6 * (hypo->vPick.size() + hypo2->vPick.size())) {
					snprintf(
							sLog,
							sizeof(sLog),
							"CHypoList::merge: -- keeping new event %s which"
							" associated %d picks of %d potential picks",
							hypo3->getPid().c_str(),
							npick,
							(static_cast<int>(hypo->vPick.size())
									+ static_cast<int>(hypo2->vPick.size())));
					glassutil::CLogit::log(sLog);

					snprintf(sLog, sizeof(sLog),
								" ** Canceling merged event %s\n",
								hypo->getPid().c_str());
					glassutil::CLogit::Out(sLog);
					remHypo(hypo);

					// check for and log any miss-linked picks
					hypo->trap();

					snprintf(sLog, sizeof(sLog),
								" ** Canceling merged event %s\n",
								hypo2->getPid().c_str());
					glassutil::CLogit::Out(sLog);
					remHypo(hypo2);

					// check for and log any miss-linked picks
					hypo2->trap();
					return true;
				} else {
					// else delete potential new events
					snprintf(
							sLog,
							sizeof(sLog),
							"CHypoList::merge: -- cancelling potential new"
							" event %s which associated %d picks of %d"
							" potential picks",
							hypo3->getPid().c_str(),
							npick,
							(static_cast<int>(hypo->vPick.size())
									+ static_cast<int>(hypo2->vPick.size())));
					glassutil::CLogit::log(sLog);
				}
			}
		}
	}

	return (false);
}
*/
// ---------------------------------------------------------ReqHypo
bool CHypoList::reqHypo(json::Object *com) {
	std::lock_guard<std::recursive_mutex> listGuard(m_vHypoMutex);

	// null check json
	if (com == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypoList::reqHypo: NULL json communication.");
		return (false);
	}

	// check cmd
	if (com->HasKey("Cmd")
			&& ((*com)["Cmd"].GetType() == json::ValueType::StringVal)) {
		std::string cmd = (*com)["Cmd"].ToString();

		if (cmd != "ReqHypo") {
			glassutil::CLogit::log(
					glassutil::log_level::warn,
					"HypoList::reqHypo: Non-ReqHypo message passed in.");
			return (false);
		}
	} else if (com->HasKey("Type")
			&& ((*com)["Type"].GetType() == json::ValueType::StringVal)) {
		std::string type = (*com)["Type"].ToString();

		if (type != "ReqHypo") {
			glassutil::CLogit::log(
					glassutil::log_level::warn,
					"HypoList::reqHypo: Non-ReqHypo message passed in.");
			return (false);
		}
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"HypoList::reqHypo: Missing required Cmd or Type Key.");
		return (false);
	}

	// pid
	std::string sPid;
	if (com->HasKey("Pid")
			&& ((*com)["Pid"].GetType() == json::ValueType::StringVal)) {
		sPid = (*com)["Pid"].ToString();
	} else {
		glassutil::CLogit::log(glassutil::log_level::error,
								"HypoList::reqHypo: Missing required Pid Key.");
		return (false);
	}

	// log
	char sLog[1024];
	snprintf(sLog, sizeof(sLog), "ReqHypo %s\n", sPid.c_str());
	glassutil::CLogit::Out(sLog);

	// get the hypo
	std::shared_ptr<CHypo> hyp = mHypo[sPid];

	// check the hypo
	if (!hyp) {
		return (false);
	}

	// generate the hypo message
	hyp->hypo();

	// done
	return (true);
}

// ---------------------------------------------------------sort
void CHypoList::sort() {
	std::lock_guard<std::recursive_mutex> listGuard(m_vHypoMutex);
	// sort hypos
	std::sort(vHypo.begin(), vHypo.end(), sortHypo);
}

// ---------------------------------------------------------processHypos
void CHypoList::processHypos() {
	glassutil::CLogit::log(glassutil::log_level::debug,
							"CHypoList::processHypos: startup");

	while (m_bRunProcessLoop == true) {
		// make sure we're still running
		if (m_bRunProcessLoop == false)
			break;

		// update thread status
		setStatus(true);

		// run the job
		try {
			darwin();
		} catch (const std::exception &e) {
			glassutil::CLogit::log(
					glassutil::log_level::error,
					"CHypoList::processHypos: Exception during darwin(): "
							+ std::string(e.what()));
			break;
		}

		// give up some time at the end of the loop
		jobSleep();
	}

	setStatus(false);
	glassutil::CLogit::log(glassutil::log_level::debug,
							"CHypoList::processHypos: Thread Exit.");
}

// ---------------------------------------------------------setStatus
void CHypoList::jobSleep() {
	if (m_bRunProcessLoop == true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(m_iSleepTimeMS));
	}
}

// ---------------------------------------------------------setStatus
void CHypoList::setStatus(bool status) {
	// update thread status
	m_StatusMutex.lock();
	if (m_ThreadStatusMap.find(std::this_thread::get_id())
			!= m_ThreadStatusMap.end()) {
		m_ThreadStatusMap[std::this_thread::get_id()] = status;
	}
	m_StatusMutex.unlock();
}

// ---------------------------------------------------------statusCheck
bool CHypoList::statusCheck() {
	// if we have a negative check interval,
	// we shouldn't worry about thread status checks.
	if (m_iStatusCheckInterval < 0)
		return (true);

	// thread is dead if we're not running
	if (m_bRunProcessLoop == false) {
		glassutil::CLogit::log(
				glassutil::log_level::warn,
				"CHypoList::statusCheck(): m_bRunProcessLoop is false.");
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
			bool status = static_cast<bool>(StatusItr->second);

			// at least one thread did not respond
			if (status != true) {
				m_StatusMutex.unlock();

				glassutil::CLogit::log(
						glassutil::log_level::error,
						"CHypoList::statusCheck(): At least one thread"
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

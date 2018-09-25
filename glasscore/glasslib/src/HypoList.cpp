#include "HypoList.h"
#include <logger.h>
#include <geo.h>
#include <json.h>
#include <string>
#include <memory>
#include <utility>
#include <algorithm>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <ctime>
#include "Site.h"
#include "Pick.h"
#include "Correlation.h"
#include "Hypo.h"
#include "Glass.h"
#include "Web.h"
#include "SiteList.h"
#include "PickList.h"
#include "CorrelationList.h"

namespace glasscore {

// ---------------------------------------------------------CHypoList
CHypoList::CHypoList(int numThreads, int sleepTime, int checkInterval)
		: glass3::util::ThreadBaseClass("HypoList", sleepTime, numThreads,
										checkInterval) {
	clear();

	// start up the threads
	start();
}

// ---------------------------------------------------------~CHypoList
CHypoList::~CHypoList() {
	// clean up everything else
	clear();
}

// ---------------------------------------------------------addHypo
bool CHypoList::addHypo(std::shared_ptr<CHypo> hypo, bool scheduleProcessing) {
	// nullcheck
	if (hypo == NULL) {
		glass3::util::Logger::log("error",
									"CHypoList::addHypo: NULL hypo provided.");

		return (false);
	}

	// lock for this scope
	std::lock_guard<std::recursive_mutex> listGuard(m_HypoListMutex);

	m_iCountOfTotalHyposProcessed++;

	// get maximum number of hypos
	// use max hypos from CGlass if we have it
	if (CGlass::getMaxNumHypos() > 0) {
		m_iMaxAllowableHypoCount = CGlass::getMaxNumHypos();
	}

	// remove oldest hypo if this new one
	// pushes us over the limit
	if (m_msHypoList.size() >= m_iMaxAllowableHypoCount) {
		std::multiset<std::shared_ptr<CHypo>, HypoCompare>::iterator oldest =
				m_msHypoList.begin();

		// find first hypo in multiset
		std::shared_ptr<CHypo> oldestHypo = *oldest;

		// send expiration message
		if (oldestHypo->getHypoGenerated()) {
			CGlass::sendExternalMessage(oldestHypo->generateExpireMessage());
		}
		// remove it
		removeHypo(oldestHypo, false);
	}

	// add to multiset
	m_msHypoList.insert(hypo);

	// add to hypo map
	m_mHypo[hypo->getID()] = hypo;

	// Schedule this hypo for refinement. Note that this
	// hypo will be the first one in the queue, and will be the
	// first one processed.
	if (scheduleProcessing == true) {
		appendToHypoProcessingQueue(hypo);
	}

	// done
	return (true);
}

// ---------------------------------------------------------associateData
bool CHypoList::associateData(std::shared_ptr<CPick> pk) {
	// nullcheck
	if (pk == NULL) {
		glass3::util::Logger::log(
				"warning", "CHypoList::associate: NULL pick provided.");

		return (false);
	}

	std::vector<std::shared_ptr<CHypo>> assocHypoList;

	// compute the list of hypos to associate with
	// (a potential hypo must be before the pick we're associating)
	// use the pick time minus 2400 seconds to compute the starting index
	// NOTE: Hard coded time delta
	std::vector<std::weak_ptr<CHypo>> hypoList = getHypos(pk->getTPick() - 2400,
															pk->getTPick());

	// make sure we got any hypos
	if (hypoList.size() == 0) {
		/*
		 glass3::util::Logger::log(
		 "debug",
		 "CHypoList::associate NOASSOC idPick:" + pk->getID()
		 + "; No Usable Hypos"); */
		// nope
		return (false);
	}

	std::shared_ptr<CHypo> bestHyp;
	double sdassoc = CGlass::getAssociationSDCutoff();

	// for each hypo in the list within the time range
	for (int i = 0; i < hypoList.size(); i++) {
		// make sure hypo is still valid before associating
		if (std::shared_ptr<CHypo> hyp = hypoList[i].lock()) {
			// check to see if the pick will associate with
			// this hypo
			// NOTE: The sigma value passed into associate is hard coded
			if (hyp->canAssociate(pk, ASSOC_SIGMA_VALUE_SECONDS, sdassoc)) {
				// add to the list of hypos this pick can associate with
				assocHypoList.push_back(hyp);

				// remember this hypo
				bestHyp = hyp;
			}
		}
	}

	// there were no hypos that the pick associated with
	if (assocHypoList.size() < 1) {
		/*
		 glass3::util::Logger::log(
		 "debug", "CHypoList::associate NOASSOC idPick:" + pk->getID());
		 */
		return (false);
	}

	// there was only one hypo that the pick associated with
	if (assocHypoList.size() == 1) {
		// link the pick to the hypo
		pk->addHypoReference(bestHyp, true);

		// link the hypo to the pick
		bestHyp->addPickReference(pk);

		glass3::util::Logger::log(
				"debug",
				"CHypoList::associate (pick) sPid:" + bestHyp->getID()
						+ " resetting cycle count due to new association");

		// reset the cycle count
		bestHyp->setProcessCount(0);

		// add to the processing queue
		appendToHypoProcessingQueue(bestHyp);

		glass3::util::Logger::log(
				"debug",
				"CHypoList::associate ASSOC idPick:" + pk->getID()
						+ "; numHypos: 1");

		// the pick was associated
		return (true);
	}

	// For each hypo that the pick could associate with
	for (auto q : assocHypoList) {
		glass3::util::Logger::log(
				"debug",
				"CHypoList::associate (pick) sPid:" + q->getID()
						+ " resetting cycle count due to new association");

		// reset the cycle count
		q->setProcessCount(0);

		// add the hypo to the processing queue
		// note that we didn't link the pick to any hypos
		appendToHypoProcessingQueue(q);
	}

	glass3::util::Logger::log(
			"debug",
			"CHypoList::associate ASSOC idPick:" + pk->getID() + "; numHypos: "
					+ std::to_string(assocHypoList.size()));

	// the pick was associated
	return (true);
}

// ---------------------------------------------------------associateData
bool CHypoList::associateData(std::shared_ptr<CCorrelation> corr) {
	// nullcheck
	if (corr == NULL) {
		glass3::util::Logger::log(
				"warning", "CHypoList::associate: NULL correlation provided.");

		return (false);
	}

	std::vector<std::shared_ptr<CHypo>> assocHypoList;

	// compute the index range to search for hypos to associate with
	// (a potential hypo must be before the pick we're associating)
	// use the correlation time minus correlationMatchingTWindow to compute the
	// starting index
	std::vector<std::weak_ptr<CHypo>> hypoList = getHypos(
			corr->getTCorrelation()
					- CGlass::getCorrelationMatchingTimeWindow(),
			corr->getTCorrelation()
					- CGlass::getCorrelationMatchingTimeWindow());

	// make sure we got any hypos
	if (hypoList.size() == 0) {
		// nope
		return (false);
	}

	std::shared_ptr<CHypo> bestHyp;

	// for each hypo in the list within the time range
	for (int i = 0; i < hypoList.size(); i++) {
		// make sure hypo is still valid before associating
		if (std::shared_ptr<CHypo> hyp = hypoList[i].lock()) {
			// check to see if the correlation will associate with
			// this hypo
			if (hyp->canAssociate(
					corr, CGlass::getCorrelationMatchingTimeWindow(),
					CGlass::getCorrelationMatchingDistanceWindow())) {
				// add to the list of hypos this correlation can associate with
				assocHypoList.push_back(hyp);

				// remember this hypo for later
				bestHyp = hyp;
			}
		}
	}

	// there were no hypos that the correlation associated with
	if (assocHypoList.size() < 1) {
		return (false);
	}

	// there was only one hypo that the correlation associated with
	if (assocHypoList.size() == 1) {
		// link the correlation to the hypo
		corr->addHypoReference(bestHyp, true);

		// link the hypo to the correlation
		bestHyp->addCorrelationReference(corr);

		glass3::util::Logger::log(
				"debug",
				"CHypoList::associate (correlation) sPid:" + bestHyp->getID()
						+ " resetting cycle count due to new association");

		// reset the cycle count
		bestHyp->setProcessCount(0);

		// add to the processing queue
		appendToHypoProcessingQueue(bestHyp);

		// the pick was associated
		return (true);
	}

	// For each hypo that the correlation could associate with
	for (auto q : assocHypoList) {
		glass3::util::Logger::log(
				"debug",
				"CHypoList::associate (correlation) sPid:" + q->getID()
						+ " resetting cycle count due to new association");

		// reset the cycle count
		q->setProcessCount(0);

		// add the hypo to the processing queue
		// note that we didn't link the correlation to any hypos
		appendToHypoProcessingQueue(q);
	}

	// the pick was associated
	return (true);
}

// ---------------------------------------------------------clear
void CHypoList::clear() {
	std::lock_guard<std::mutex> queueGuard(m_HypoProcessingQueueMutex);
	m_lHypoProcessingQueue.clear();

	std::lock_guard<std::recursive_mutex> listGuard(m_HypoListMutex);
	m_msHypoList.clear();
	m_mHypo.clear();

	// reset
	m_iCountOfTotalHyposProcessed = 0;
	m_iMaxAllowableHypoCount = 100;
}

// ---------------------------------------------------------work
glass3::util::WorkState CHypoList::work() {
	// don't bother if there's nothing to do
	if (getHypoProcessingQueueLength() < 1) {
		// on to the next loop
		return (glass3::util::WorkState::Idle);
	}

	// log the cycle count and queue size
	char sLog[1024];

	// get the next hypo to process
	std::shared_ptr<CHypo> hyp = getNextHypoFromProcessingQueue();

	// check to see if we got a valid hypo
	if (!hyp) {
		// nothing to process, move on
		// on to the next loop
		return (glass3::util::WorkState::Idle);
	}

	std::lock_guard<std::mutex> hypoGuard(hyp->getProcessingMutex());

	try {
		// log the hypo we're working on
		glass3::util::Logger::log(
				"debug",
				"CHypoList::work Processing Hypo sPid:" + hyp->getID()
						+ " Cycle:" + std::to_string(hyp->getProcessCount())
						+ " Fifo Size:"
						+ std::to_string(getHypoProcessingQueueLength()));

		// check to see if this hypo is viable.
		if (hyp->cancelCheck()) {
			// this hypo is no longer viable
			// log
			glass3::util::Logger::log(
					"debug",
					"CHypoList::work canceling sPid:" + hyp->getID()
							+ " processCount:"
							+ std::to_string(hyp->getTotalProcessCount()));

			// remove hypo from the hypo list
			removeHypo(hyp);

			// done with processing
			return (glass3::util::WorkState::OK);
		}

		// check to see if we've hit the iCycle Limit for this
		// hypo
		if (hyp->getProcessCount() >= CGlass::getProcessLimit()) {
			// log
			glass3::util::Logger::log(
					"debug",
					"CHypoList::work skipping sPid:" + hyp->getID()
							+ " at cycle limit:"
							+ std::to_string(hyp->getProcessCount())
							+ +" processCount:"
							+ std::to_string(hyp->getTotalProcessCount()));

			return (glass3::util::WorkState::OK);
		}

		// process this hypocenter
		if (processHypo(hyp) == true) {
			// reposition the hypo in the list to maintain
			// time order
			updatePosition(hyp);
		}
	} catch (const std::exception &e) {
		glass3::util::Logger::log(
				"error",
				"CHypoList::work: Exception during processing: "
						+ std::string(e.what()));
		return (glass3::util::WorkState::Error);
	}

	// done with processing
	return (glass3::util::WorkState::OK);
}

// ------------------------------------------------------receiveExternalMessage
bool CHypoList::receiveExternalMessage(std::shared_ptr<json::Object> com) {
	// null check json
	if (com == NULL) {
		glass3::util::Logger::log(
				"error",
				"CHypoList::receiveExternalMessage: NULL json communication.");
		return (false);
	}

	// check for a command
	if (com->HasKey("Cmd")
			&& ((*com)["Cmd"].GetType() == json::ValueType::StringVal)) {
		// dispatch to appropriate function based on Cmd value
		json::Value v = (*com)["Cmd"].ToString();

		// a hypo message has been requested
		if (v == "ReqHypo") {
			return (requestHypo(com));
		}
	}

	// this communication was not handled
	return (false);
}

// ---------------------------------------------------------processHypo
bool CHypoList::processHypo(std::shared_ptr<CHypo> hyp) {
	// nullcheck
	if (hyp == NULL) {
		glass3::util::Logger::log(
				"warning", "CHypoList::processHypo: NULL hypo provided.");
		return (false);
	}

	// check to see if this is a valid hypo, a hypo must always have an id
	if (hyp->getID() == "") {
		return (false);
	}

	std::string pid = hyp->getID();
	// int OriginalPicks = hyp->getPickDataSize();
	std::chrono::high_resolution_clock::time_point tEvolveStartTime =
			std::chrono::high_resolution_clock::now();

	hyp->incrementTotalProcessCount();
	hyp->setProcessCount(hyp->getProcessCount() + 1);

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
	if (CGlass::getPickList()->scavenge(hyp)) {
		// we should report this hypo since it has changed
		breport = true;
		// relocate the hypo
		hyp->localize();
	}

	// search for any associable correlations that match hypo in the correlation
	// list
	if (CGlass::getCorrelationList()->scavenge(hyp)) {
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
	if (resolveData(hyp)) {
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
	if (hyp->pruneData()) {
		// we should report this hypo since it has changed
		breport = true;
		// relocate the hypo
		hyp->localize();
	}

	// Iterate on pruning data
	if (hyp->pruneData()) {
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

	// check to see if this hypo is viable.
	if (hyp->cancelCheck()) {
		std::chrono::high_resolution_clock::time_point tCancelEndTime =
				std::chrono::high_resolution_clock::now();
		double cancelTime = std::chrono::duration_cast<
				std::chrono::duration<double>>(tCancelEndTime - tPruneEndTime)
				.count();

		removeHypo(hyp);

		std::chrono::high_resolution_clock::time_point tRemoveEndTime =
				std::chrono::high_resolution_clock::now();
		double removeTime = std::chrono::duration_cast<
				std::chrono::duration<double>>(tRemoveEndTime - tCancelEndTime)
				.count();

		double evolveTime = std::chrono::duration_cast<
				std::chrono::duration<double>>(
				tRemoveEndTime - tEvolveStartTime).count();

		glass3::util::Logger::log(
				"debug",
				"CHypoList::processHypo: Canceled sPid:" + pid + " cycle:"
						+ std::to_string(hyp->getProcessCount())
						+ " processCount:"
						+ std::to_string(hyp->getTotalProcessCount())
						+ " processHypo Timing: localizeTime:"
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

	// if event is all good check if proximal events can be merged.
	findAndMergeMatchingHypos(hyp);

	std::chrono::high_resolution_clock::time_point tMergeEndTime =
			std::chrono::high_resolution_clock::now();
	double mergeTime =
			std::chrono::duration_cast<std::chrono::duration<double>>(
					tMergeEndTime - tCancelEndTime).count();

	// announce if a correlation has been added to an existing event
	// NOTE: Is there a better way to do this?
	if ((hyp->getCorrelationAdded() == true)
			&& (hyp->getPickDataSize() >= hyp->getNucleationDataThreshold())) {
		breport = true;
		hyp->setCorrelationAdded(false);
	} else {
		hyp->setCorrelationAdded(false);
	}

	// check to see if this is a new event
	if (hyp->getTotalProcessCount() < 2) {
		glass3::util::Logger::log(
				"debug",
				"CHypoList::processHypo: Should report new hypo sPid:" + pid
						+ " cycle:" + std::to_string(hyp->getProcessCount())
						+ " processCount:"
						+ std::to_string(hyp->getTotalProcessCount()));

		// we should always report a new hypo (ensure it reports at least once)
		breport = true;
	}

	// if we're supposed to report
	if (breport == true) {
		// if we CAN report
		if (hyp->reportCheck() == true) {
			// report
			CGlass::sendExternalMessage(hyp->generateEventMessage());

			glass3::util::Logger::log(
					"debug",
					"CHypoList::processHypo: Reported hypo sPid:" + pid
							+ " cycle:" + std::to_string(hyp->getProcessCount())
							+ " processCount:"
							+ std::to_string(hyp->getTotalProcessCount()));
		} else {
			glass3::util::Logger::log(
					"debug",
					"CHypoList::processHypo: hypo sPid:" + pid
							+ " processCount:"
							+ std::to_string(hyp->getTotalProcessCount())
							+ " failed reportCheck()");
		}
	}

	std::chrono::high_resolution_clock::time_point tReportEndTime =
			std::chrono::high_resolution_clock::now();
	double reportTime =
			std::chrono::duration_cast<std::chrono::duration<double>>(
					tReportEndTime - tMergeEndTime).count();

	// check for and log any miss-linked picks
	hyp->trap();

	std::chrono::high_resolution_clock::time_point tTrapEndTime =
			std::chrono::high_resolution_clock::now();
	double trapTime = std::chrono::duration_cast<std::chrono::duration<double>>(
			tTrapEndTime - tReportEndTime).count();

	double evolveTime =
			std::chrono::duration_cast<std::chrono::duration<double>>(
					tTrapEndTime - tEvolveStartTime).count();

	glass3::util::Logger::log(
			"debug",
			"CHypoList::processHypo: Finished sPid:" + pid + " cycle:"
					+ std::to_string(hyp->getProcessCount()) + " processCount:"
					+ std::to_string(hyp->getTotalProcessCount())
					+ " processHypo Timing: localizeTime:"
					+ std::to_string(localizeTime) + " scavengeTime:"
					+ std::to_string(scavengeTime) + " resolveTime:"
					+ std::to_string(resolveTime) + " pruneTime:"
					+ std::to_string(pruneTime) + " cancelTime:"
					+ std::to_string(cancelTime) + " mergeTime:"
					+ std::to_string(mergeTime) + " reportTime:"
					+ std::to_string(reportTime) + " trapTime:"
					+ std::to_string(trapTime) + " evolveTime:"
					+ std::to_string(evolveTime));

	// if the number of picks associated with the event changed, reprocess
	/*
	 if (hyp->getVPickSize() != OriginalPicks) {
	 glass3::util::Logger::log(
	 "debug",
	 "CHypoList::processHypo: Picks changed for sPid:" + pid
	 + " old picks:" + std::to_string(OriginalPicks)
	 + " new picks:" + std::to_string(hyp->getVPickSize()));
	 addHypoToProcess(hyp);
	 }
	 */
	// the hypo survived, so return true
	return (true);
}

// -------------------------------------------------getHypoProcessingQueueLength
int CHypoList::getHypoProcessingQueueLength() {
	std::lock_guard<std::mutex> queueGuard(m_HypoProcessingQueueMutex);

	// return the current size of the queue
	int size = m_lHypoProcessingQueue.size();
	return (size);
}

// ---------------------------------------------------------getHypos
std::vector<std::weak_ptr<CHypo>> CHypoList::getHypos(double t1, double t2) {
	std::vector<std::weak_ptr<CHypo>> hypos;

	if (t1 == t2) {
		return (hypos);
	}
	// swap t1 and t2 if necessary so that t1 <= t2
	if (t1 > t2) {
		double temp = t2;
		t2 = t1;
		t1 = temp;
	}

	std::shared_ptr<traveltime::CTravelTime> nullTrav;
	std::shared_ptr<traveltime::CTTT> nullTTT;

	// construct the lower bound value. std::multiset requires
	// that this be in the form of a std::shared_ptr<CHypo>
	std::shared_ptr<CHypo> lowerValue = std::make_shared<CHypo>(0, 0, 0, t1, "",
																"", 0, 0, 0,
																nullTrav,
																nullTrav,
																nullTTT);

	// construct the upper bound value. std::multiset requires
	// that this be in the form of a std::shared_ptr<CHypo>
	std::shared_ptr<CHypo> upperValue = std::make_shared<CHypo>(0, 0, 0, t2, "",
																"", 0, 0, 0,
																nullTrav,
																nullTrav,
																nullTTT);

	std::lock_guard<std::recursive_mutex> listGuard(m_HypoListMutex);

	// don't bother if the list is empty
	if (m_msHypoList.size() == 0) {
		return (hypos);
	}

	// get the bounds for this window
	std::multiset<std::shared_ptr<CHypo>, HypoCompare>::iterator lower =
			m_msHypoList.lower_bound(lowerValue);
	std::multiset<std::shared_ptr<CHypo>, HypoCompare>::iterator upper =
			m_msHypoList.upper_bound(upperValue);

	// found nothing
	if (lower == m_msHypoList.end()) {
		return (hypos);
	}

	// found one
	if ((lower == upper) && (lower != m_msHypoList.end())) {
		std::shared_ptr<CHypo> aHypo = *lower;

		if (aHypo != NULL) {
			std::weak_ptr<CHypo> awHypo = aHypo;

			// add to the list of hypos
			hypos.push_back(awHypo);
		}
		return (hypos);
	}

	// loop through hypos
	for (std::multiset<std::shared_ptr<CHypo>, HypoCompare>::iterator it = lower;
			((it != upper) && (it != m_msHypoList.end())); ++it) {
		std::shared_ptr<CHypo> aHypo = *it;

		if (aHypo != NULL) {
			std::weak_ptr<CHypo> awHypo = aHypo;

			// add to the list of hypos
			hypos.push_back(awHypo);
		}
	}

	// return the list of hypos we found
	return (hypos);
}

// ---------------------------------------------------------getHypoMax
int CHypoList::getMaxAllowableHypoCount() const {
	return (m_iMaxAllowableHypoCount);
}

// ----------------------------------------------------------
int CHypoList::getCountOfTotalHyposProcessed() const {
	return (m_iCountOfTotalHyposProcessed);
}

// ---------------------------------------------------------size
int CHypoList::length() const {
	std::lock_guard<std::recursive_mutex> vHypoGuard(m_HypoListMutex);
	return (m_msHypoList.size());
}

// ---------------------------------------------------------mergeCloseEvents
bool CHypoList::findAndMergeMatchingHypos(std::shared_ptr<CHypo> hypo) {
	// nullcheck
	if (hypo == NULL) {
		return (false);
	}

	// check to see if this is a valid hypo, a hypo must always have an id
	if (hypo->getID() == "") {
		return (false);
	}

	char sLog[1024];  // logging string
	double distanceCut = CGlass::getHypoMergingDistanceWindow();
	double timeCut = CGlass::getHypoMergingTimeWindow();
	bool merged = false;

	// Get the list of hypos to try merging with with
	// (a potential hypo must be within time cut to consider)
	std::vector<std::weak_ptr<CHypo>> mergeList = getHypos(
			hypo->getTOrigin() - timeCut, hypo->getTOrigin() + timeCut);

	// make sure we got hypos returned
	if (mergeList.size() == 0) {
		// print not events to merge message
		snprintf(sLog, sizeof(sLog),
					"CHypoList::findAndMergeMatchingHypos: No hypos in merge "
					"window for %s",
					hypo->getID().c_str());
		glass3::util::Logger::log(sLog);
		return (merged);
	}

	// only this hypo was returned
	if (mergeList.size() == 1) {
		// print not events to merge message
		std::shared_ptr<CHypo> thypo = mergeList[0].lock();
		snprintf(
				sLog,
				sizeof(sLog),
				"CHypoList::findAndMergeMatchingHypos: No other hypos available "
				"to merge with %s",
				hypo->getID().c_str());
		glass3::util::Logger::log(sLog);
		return (merged);
	}

	// for each hypo in the mergeList
	for (int i = 0; i < mergeList.size(); i++) {
		// make sure hypo is still valid before merging
		if (std::shared_ptr<CHypo> aHypo = mergeList[i].lock()) {
			// make sure we're not looking at ourself
			if (hypo->getID() == aHypo->getID()) {
				continue;
			}

			// try to lock for this scope
			std::unique_lock<std::mutex> lock(aHypo->getProcessingMutex(),
												std::try_to_lock);
			if (!lock.owns_lock()) {
				// we don't have this hypo locked
				// move on
				continue;
			}

			snprintf(
					sLog, sizeof(sLog),
					"CHypoList::findAndMergeMatchingHypos: Testing merger of"
					"hypo %s and %s\n",
					aHypo->getID().c_str(), hypo->getID().c_str());
			glass3::util::Logger::log(sLog);

			// prefer to merge into the hypo that has already been published
			std::shared_ptr<CHypo> toHypo;
			std::shared_ptr<CHypo> fromHypo;
			if ((hypo->getHypoGenerated() == false)
					&& (aHypo->getHypoGenerated() == true)) {
				toHypo = aHypo;
				fromHypo = hypo;
			} else {
				toHypo = hypo;
				fromHypo = aHypo;
			}

			// get geo objects
			glass3::util::Geo fromGeo;
			fromGeo.setGeographic(fromHypo->getLatitude(),
									fromHypo->getLongitude(),
									EARTHRADIUSKM);

			glass3::util::Geo toGeo;
			toGeo.setGeographic(toHypo->getLatitude(), toHypo->getLongitude(),
			EARTHRADIUSKM);

			// calculate distance between hypos
			double distanceDiff = toGeo.delta(&fromGeo) / DEG2RAD;

			// check distance
			if (distanceDiff > distanceCut) {
				continue;
			}

			// get picks
			auto fromPicks = fromHypo->getPickData();
			auto toPicks = toHypo->getPickData();

			// get bayes values
			double fromBayes = fromHypo->getBayesValue();
			double toBayes = toHypo->getBayesValue();

			// Log info on two events
			snprintf(
					sLog,
					sizeof(sLog),
					"CHypoList::findAndMergeMatchingHypos: fromHypo:%s lat:%.3f, "
					"lon:%.3f, depth:%.3f, time:%.3f, bayes: %.3f, nPicks:%d\n",
					fromHypo->getID().c_str(), fromHypo->getLatitude(),
					fromHypo->getLongitude(), fromHypo->getDepth(),
					fromHypo->getTOrigin(), fromHypo->getBayesValue(),
					static_cast<int>(fromPicks.size()));
			glass3::util::Logger::log(sLog);

			snprintf(
					sLog, sizeof(sLog),
					"CHypoList::findAndMergeMatchingHypos: toHypo:%s lat:%.3f, "
					"lon:%.3f, depth:%.3f, time:%.3f, bayes: %.3f, nPicks:%d\n",
					toHypo->getID().c_str(), toHypo->getLatitude(),
					toHypo->getLongitude(), toHypo->getDepth(),
					toHypo->getTOrigin(), toHypo->getBayesValue(),
					static_cast<int>(toPicks.size()));
			glass3::util::Logger::log(sLog);

			// add all picks from fromHypo into toHypo
			for (auto pick : fromPicks) {
				toHypo->addPickReference(pick);
			}

			// initial localization attempt of toHypo after adding picks
			toHypo->anneal(2000, (distanceCut / 2.) * DEG2KM,
							(distanceCut / 100.) * DEG2KM, (timeCut / 2.), .01);

			// Remove picks from toHypo that do not fit initial location
			if (toHypo->pruneData()) {
				// relocate the toHypo if we pruned
				toHypo->localize();
			}

			// get the new bayes value
			double newBayes = toHypo->getBayesValue();

			// check that the new bayes is better than either of the original
			// bayes values
			if (newBayes
					> (std::max(toBayes, fromBayes))
							+ (.1 * std::min(toBayes, fromBayes))) {
				snprintf(
						sLog, sizeof(sLog),
						"CHypoList::findAndMergeMatchingHypos: merged fromHypo "
						"%s into toHypo %s because toHypo significantly better "
						"than fromHypo and original toHypo, newBayes:%.3f > "
						"toHypo Bayes:%.3f, fromHypo bayes:%.3f",
						fromHypo->getID().c_str(), toHypo->getID().c_str(),
						newBayes, toBayes, fromBayes);
				glass3::util::Logger::log(sLog);

				// toHypo has effectively been replaced, so now remove
				// fromHypo, since it was successfully merged
				removeHypo(fromHypo);

				// we've merged a hypo, move on to the next candidate
				merged = true;
			} else if (newBayes >= toBayes * 0.99) {  // protect against rounding
			// error or minor issue in location?
			// the new Hypo is at least as good as the old toHypo but it
			// hasn't improved significantly. This could be because either
			// from Hypo has a subset of the picks of the toHypo, or because
			// the two hypos are unrelated and share no picks. Resolve
			// duplicate picks and see if the weaker hypo can still stand.
				if (resolveData(toHypo)) {
					// relocate the toHypo if we resolved to get an updated
					// bayes value
					toHypo->localize();
				}

				// relocate and more importantly re-calc statistics for the
				// fromHypo, now that we've potentially ripped picks away from
				// it.
				fromHypo->localize();

				// Resolve duplicate picks, using our increased relative
				// strength as we've stripped dup's away from fromHypo.
				if (resolveData(toHypo)) {
					// relocate the toHypo if we resolved to get an updated
					// bayes value
					toHypo->localize();
				}

				// check to see if fromHypo is still healthy
				if (fromHypo->cancelCheck()) {
					snprintf(
							sLog,
							sizeof(sLog),
							"CHypoList::findAndMergeMatchingHypos: merged "
							"fromHypo %s into toHypo %s because fromHypo failed "
							"cancelCheck, toHypo:Bayes %.3f, fromHypo:bayes %.3f",
							fromHypo->getID().c_str(), toHypo->getID().c_str(),
							toHypo->getBayesValue(), fromHypo->getBayesValue());
					glass3::util::Logger::log(sLog);

					// fromHypo isn't strong enough to stand on it's own after
					// we've resolved picks with toHypo.  Merge essentially
					// successful, make fromHypo go away.
					removeHypo(fromHypo);

					// we've merged a hypo, move on to the next candidate
					merged = true;
				} else {
					// uh oh.  We were wrong.  fromHypo is still good.
					// What to do now? do we just leave it as is.  We really
					// haven't done anything evil to it, we just resolved
					// picks multiple times... We potentially stole a bunch of
					// picks from it, but not without merit, and the picks can't
					// belong to both it and toHypo forever, so...
					// And if there were NO overlapping picks between the two
					// events, then we haven't done anything to fromHypo at
					// all...
					// log something here?

					// the merged hypo (toHypo) was not better, revert toHypo.
					// and fromHypo
					snprintf(
							sLog, sizeof(sLog),
							"CHypoList::findAndMergeMatchingHypos: keeping "
							"modified hypos %s and %s because fromHypo passed "
							"cancelCheck, toHypo Bayes:%.3f, fromHypo "
							"bayes:%.3f",
							toHypo->getID().c_str(), fromHypo->getID().c_str(),
							toHypo->getBayesValue(), fromHypo->getBayesValue());
					glass3::util::Logger::log(sLog);
				}  // end else (fromHypo->cancelCheck())
			} else {
				// the merged hypo (toHypo) was not better, revert toHypo.
				snprintf(
						sLog,
						sizeof(sLog),
						"CHypoList::findAndMergeMatchingHypos: keeping original "
						"hypos %s and %s, newBayes:%.3f toHypo Bayes:%.3f, "
						"fromHypo bayes:%.3f",
						toHypo->getID().c_str(), fromHypo->getID().c_str(),
						newBayes, toBayes, fromBayes);
				glass3::util::Logger::log(sLog);

				// reset toHypo to where it was
				toHypo->clearPickReferences();

				// add the original toHypo picks
				for (auto pick : toPicks) {
					toHypo->addPickReference(pick);
				}

				// relocate toHypo
				toHypo->localize();
			}  // end else is toHypoBetter
		}
	}

	// return whether we've merged anything
	return (merged);
}

// --------------------------------------------------appendToHypoProcessingQueue
int CHypoList::appendToHypoProcessingQueue(std::shared_ptr<CHypo> hyp) {
	// don't use a lock guard for queue mutex and vhypolist mutex,
	// to avoid a deadlock when both mutexes are locked
	m_HypoProcessingQueueMutex.lock();
	int size = m_lHypoProcessingQueue.size();
	m_HypoProcessingQueueMutex.unlock();

	// nullcheck
	if (hyp == NULL) {
		glass3::util::Logger::log(
				"error",
				"CHypoList::appendToHypoProcessingQueue: NULL hypo provided.");
		return (size);
	}

	if (hyp->getID() == "") {
		return (size);
	}

	// get this hypo's id
	std::string pid = hyp->getID();

	// use the map to get see if this hypo is even on the hypo list
	m_HypoListMutex.lock();
	if (m_mHypo[pid] == NULL) {
		// it's not, we can't really process a hypo we don't have
		m_HypoListMutex.unlock();

		return (size);
	}
	m_HypoListMutex.unlock();

	// is this id already on the queue?
	m_HypoProcessingQueueMutex.lock();
	for (std::list<std::weak_ptr<CHypo>>::iterator it = m_lHypoProcessingQueue
			.begin(); it != m_lHypoProcessingQueue.end(); ++it) {
		std::shared_ptr<CHypo> aHyp = (*it).lock();

		if ((aHyp != NULL) && (aHyp->getID() == hyp->getID())) {
			// found it, don't bother adding it again
			m_HypoProcessingQueueMutex.unlock();
			return (size);
		}
	}

	// add to queue (FIFO)
	m_lHypoProcessingQueue.push_back(std::weak_ptr<CHypo>(hyp));

	// added one
	size++;
	m_HypoProcessingQueueMutex.unlock();

	glass3::util::Logger::log(
			"debug",
			"CHypoList::appendToHypoProcessingQueue: sPid:" + pid + " "
					+ std::to_string(size) + " hypos in queue.");

	return (size);
}

// -----------------------------------------------getNextHypoFromProcessingQueue
std::shared_ptr<CHypo> CHypoList::getNextHypoFromProcessingQueue() {
	std::lock_guard<std::mutex> queueGuard(m_HypoProcessingQueueMutex);

	// is there anything on the queue?
	if (m_lHypoProcessingQueue.size() == 0) {
		// nope
		return (NULL);
	}

	// Pop first hypocenter off processing queue (FIFO)
	std::list<std::weak_ptr<CHypo>>::iterator it =
			m_lHypoProcessingQueue.begin();
	while (it != m_lHypoProcessingQueue.end()) {
		// get the next hypo in the vector
		std::shared_ptr<CHypo> hyp = (*it).lock();

		// one way or another we're done with this hypo
		it = m_lHypoProcessingQueue.erase(it);

		// is it valid?
		if ((hyp != NULL) && (hyp->getID() != "")) {
			// return the hypo
			return (hyp);
		}
	}

	return (NULL);
}

// ---------------------------------------------------------removeHypo
void CHypoList::removeHypo(std::shared_ptr<CHypo> hypo, bool reportCancel) {
	// nullchecks
	if (hypo == NULL) {
		glass3::util::Logger::log(
				"error", "CHypoList::removeHypo: NULL hypo provided.");
		return;
	}
	if (hypo->getID() == "") {
		return;
	}

	// Send cancellation message for this hypo
	if (reportCancel == true) {
		// only if we've sent an event message
		if (hypo->getEventGenerated()) {
			// create cancellation message
			CGlass::sendExternalMessage(hypo->generateCancelMessage());
		}
	}

	// remove from from multiset
	eraseFromMultiset(hypo);

	// erase this hypo from the map
	m_HypoListMutex.lock();
	m_mHypo.erase(hypo->getID());
	m_HypoListMutex.unlock();

	// clear all other hypo data
	// we do this to signify that we've deleted this hypo (to prevent
	// double remove attempts) and to unlink all the hypo's data so
	// it can be used by others
	hypo->clear();
}

// ---------------------------------------------------------requestHypo
bool CHypoList::requestHypo(std::shared_ptr<json::Object> com) {
	// null check json
	if (com == NULL) {
		glass3::util::Logger::log(
				"error", "CHypoList::requestHypo: NULL json communication.");
		return (false);
	}

	// check cmd
	if (com->HasKey("Cmd")
			&& ((*com)["Cmd"].GetType() == json::ValueType::StringVal)) {
		std::string cmd = (*com)["Cmd"].ToString();

		if (cmd != "ReqHypo") {
			glass3::util::Logger::log(
					"warning",
					"HypoList::requestHypo: Non-requestHypo message passed in.");
			return (false);
		}
	} else if (com->HasKey("Type")
			&& ((*com)["Type"].GetType() == json::ValueType::StringVal)) {
		std::string type = (*com)["Type"].ToString();

		if (type != "requestHypo") {
			glass3::util::Logger::log(
					"warning",
					"HypoList::requestHypo: Non-requestHypo message passed in.");
			return (false);
		}
	} else {
		glass3::util::Logger::log(
				"error",
				"HypoList::requestHypo: Missing required Cmd or Type Key.");
		return (false);
	}

	// pid
	std::string sPid;
	if (com->HasKey("Pid")
			&& ((*com)["Pid"].GetType() == json::ValueType::StringVal)) {
		sPid = (*com)["Pid"].ToString();
	} else {
		glass3::util::Logger::log(
				"error", "HypoList::requestHypo: Missing required Pid Key.");
		return (false);
	}

	std::lock_guard<std::recursive_mutex> listGuard(m_HypoListMutex);

	// get the hypo
	std::shared_ptr<CHypo> hyp = m_mHypo[sPid];

	// check the hypo
	if (!hyp) {
		glass3::util::Logger::log(
				"warning",
				"HypoList::requestHypo: Could not find hypo for pid " + sPid);

		// return true even if we didn't find anything, since
		// we've handled the message
		return (true);
	}

	// generate the hypo message
	CGlass::sendExternalMessage(hyp->generateHypoMessage());

	// done
	return (true);
}

// ---------------------------------------------------------resolve
bool CHypoList::resolveData(std::shared_ptr<CHypo> hyp, bool allowStealing) {
	// null checks
	if (hyp == NULL) {
		glass3::util::Logger::log("warning",
									"CHypoList::resolve: NULL hypo provided.");

		return (false);
	}

	// this lock guard exists to avoid a deadlock that occurs
	// when it isn't present
	std::lock_guard<std::recursive_mutex> listGuard(m_HypoListMutex);

	// return whether we've changed the pick set
	return (hyp->resolveData(hyp, allowStealing));
}

// ---------------------------------------------------------setNHypoMax
void CHypoList::setMaxAllowableHypoCount(int hypoMax) {
	std::lock_guard<std::recursive_mutex> hypoListGuard(m_HypoListMutex);
	m_iMaxAllowableHypoCount = hypoMax;
}

// ---------------------------------------------------------updatePosition
void CHypoList::updatePosition(std::shared_ptr<CHypo> hyp) {
	// nullchecks
	if (hyp == NULL) {
		return;
	}
	if (hyp->getID() == "") {
		return;
	}

	std::lock_guard<std::recursive_mutex> listGuard(m_HypoListMutex);

	// from my research, the best way to "update" the position of an item
	// in a multiset when the key value has changed (in this case, the hypo
	// origin time) is to remove and re-add the item. This will give us O(log n)
	// complexity for updating one item, which is better than a full sort
	// (which I'm not really sure how to do on a multiset)
	// erase
	eraseFromMultiset(hyp);

	// update tSort
	hyp->setTSort(hyp->getTOrigin());

	// insert
	m_msHypoList.insert(hyp);
}

// ---------------------------------------------------------updatePosition
void CHypoList::eraseFromMultiset(std::shared_ptr<CHypo> hyp) {
	// nullchecks
	if (hyp == NULL) {
		return;
	}
	if (hyp->getID() == "") {
		return;
	}

	std::lock_guard<std::recursive_mutex> listGuard(m_HypoListMutex);

	if (m_msHypoList.size() == 0) {
		return;
	}

	// first, try to delete the hypo the efficient way
	// we need to be careful, because multiple hypos in the mulitset
	// can have the same tSort, and a simple erase would delete
	// them all, which would be BAD, so we need to confirm the id
	std::multiset<std::shared_ptr<CHypo>, HypoCompare>::iterator lower =
			m_msHypoList.lower_bound(hyp);
	std::multiset<std::shared_ptr<CHypo>, HypoCompare>::iterator upper =
			m_msHypoList.upper_bound(hyp);
	std::multiset<std::shared_ptr<CHypo>, HypoCompare>::iterator it;

	// for all matching (tSort range) hypos
	for (it = lower; ((it != upper) && (it != m_msHypoList.end())); ++it) {
		std::shared_ptr<CHypo> aHyp = *it;

		// only erase the correct one
		if (aHyp->getID() == hyp->getID()) {
			m_msHypoList.erase(it);
			return;
		}
	}

	glass3::util::Logger::log(
			"warning",
			"CHypoList::eraseFromMultiset: efficient delete for hypo "
					+ hyp->getID() + " didn't work.");

	// if we didn't delete it efficiently, loop through all hypos, I know this is
	// brute force, but the efficient delete didn't work, and the hypo list is
	// relatively small and we want to be sure
	// note: this may just be me being paranoid
	for (it = m_msHypoList.begin(); (it != m_msHypoList.end()); ++it) {
		std::shared_ptr<CHypo> aHyp = *it;

		// only erase the correct one
		if (aHyp->getID() == hyp->getID()) {
			m_msHypoList.erase(it);
			return;
		}
	}

	glass3::util::Logger::log(
			"error",
			"CHypoList::eraseFromMultiset: did not delete hypo " + hyp->getID()
					+ " in multiset, id not found.");
}
}  // namespace glasscore

#include "HypoList.h"
#include <logger.h>
#include <stringutil.h>
#include <geo.h>
#include <date.h>
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
#include <thread>
#include <chrono>
#include <mutex>
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

// constants
constexpr double CHypoList::k_nHypoSearchPastDurationForPick;
const int CHypoList::k_nMaxAllowableHypoCountDefault;
const unsigned int CHypoList::k_nNumberOfMergeAnnealIterations;
constexpr double CHypoList::k_dFinalMergeAnnealTimeStepSize;
constexpr double CHypoList::k_dMergeStackImprovementRatio;
constexpr double CHypoList::k_dMinimumRoundingProtectionRatio;
constexpr double CHypoList::k_dExistingTimeTolerance;
constexpr double CHypoList::k_dExistingDistanceTolerance;
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
}

// ---------------------------------------------------------addHypo
bool CHypoList::addHypo(std::shared_ptr<CHypo> hypo, bool scheduleProcessing,
		CPickList* parentThread) {
	// nullcheck
	if (hypo == NULL) {
		glass3::util::Logger::log("error",
									"CHypoList::addHypo: NULL hypo provided.");

		return (false);
	}

	if (parentThread != NULL) {
		parentThread->setThreadHealth();
	}

	// first check for similar hypos, we want the window a *little*
	// larger than the time tolerance, so we use 0.55
	std::vector<std::weak_ptr<CHypo>> hypoList = getHypos(
			hypo->getTOrigin() - (k_dExistingTimeTolerance),
			hypo->getTOrigin() + (k_dExistingTimeTolerance));

	// make sure that there were any existing hypos in the time window
	if (hypoList.size() >= 0) {
		// for each hypo in the list within the time range
		for (int i = 0; i < hypoList.size(); i++) {
			if (parentThread != NULL) {
				parentThread->setThreadHealth();
			}

			// make sure hypo is still valid before checking
			if (std::shared_ptr<CHypo> aHypo = hypoList[i].lock()) {
				if (aHypo->getID() == "") {
					continue;
				}

				// get the supporting pick data, we need it for
				// both checks
				auto hypoPicks = hypo->getPickData();

				// check to see if this hypo is close enough to match
				if ((std::abs(hypo->getTOrigin() - aHypo->getTOrigin()) <
						k_dExistingTimeTolerance) &&
					(std::abs(hypo->getLatitude() - aHypo->getLatitude()) <
						k_dExistingDistanceTolerance) &&
					(std::abs(hypo->getLongitude() - aHypo->getLongitude()) <
						k_dExistingDistanceTolerance)) {
					// we have a matching hypo
					// add all the picks in this hypo to
					// the existing hypo just in case
					int addPickCount = 0;

					for (auto pick : hypoPicks) {
						if (parentThread != NULL) {
							parentThread->setThreadHealth();
						}

						// try and add the pick to the hypo.
						// Keep track of how many picks were added.
						// if a pick was not added, it was probably
						// already in the event
						if (aHypo->addPickReference(pick) == true) {
							addPickCount++;
						}
					}

					glass3::util::Logger::log(
						"debug", "CHypoList::addHypo: Existing Proximal Hypo: "
						+ aHypo->getID()
						+ "; ot:"
						+ glass3::util::Date::encodeDateTime(aHypo->getTOrigin())
						+ "; lat:"
						+ glass3::util::to_string_with_precision(aHypo->getLatitude(), 3)
						+ "; lon:"
						+ glass3::util::to_string_with_precision(aHypo->getLongitude(), 3)
						+ "; z:"
						+ glass3::util::to_string_with_precision(aHypo->getDepth())
						+ " found in list. Not adding new Hypo: "
						+ hypo->getID()
						+ "; ot:"
						+ glass3::util::Date::encodeDateTime(hypo->getTOrigin())
						+ "; lat:"
						+ glass3::util::to_string_with_precision(hypo->getLatitude(), 3)
						+ "; lon:"
						+ glass3::util::to_string_with_precision(hypo->getLongitude(), 3)
						+ "; z:"
						+ glass3::util::to_string_with_precision(hypo->getDepth())
						+ " -> "
						+ glass3::util::to_string_with_precision(
							std::abs(hypo->getTOrigin() - aHypo->getTOrigin()), 3)
						+ " < "
						+ glass3::util::to_string_with_precision(k_dExistingTimeTolerance, 3)
						+ "; "
						+ glass3::util::to_string_with_precision(
							std::abs(hypo->getLatitude() - aHypo->getLatitude()), 3)
						+ " < "
						+ glass3::util::to_string_with_precision(k_dExistingDistanceTolerance, 3)
						+ "; "
						+ glass3::util::to_string_with_precision(
							std::abs(hypo->getLongitude() - aHypo->getLongitude()), 3)
						+ " < "
						+ glass3::util::to_string_with_precision(
							k_dExistingDistanceTolerance, 3)
						+ "; added "
						+ std::to_string(addPickCount)
						+ " picks from new hypo to existing hypo.");

					if ((addPickCount > 0) && (scheduleProcessing == true)) {
						appendToHypoProcessingQueue(aHypo);
					}

					// didn't add the hypo
					return(false);
				}  // end if hypo is close enough

				// now compare the pick sets to see if they overlap
				int hasPickCount = 0;
				int numPicks = hypoPicks.size();

				// for each pick in hypo, count how many picks are also
				// in aHypo
				for (auto pick : hypoPicks) {
					if (parentThread != NULL) {
						parentThread->setThreadHealth();
					}

					// check to see if aHypo has this pick
					if (aHypo->hasPickReference(pick) == true) {
						hasPickCount++;
					}
				}

				// compute the percentage of picks that hypo has that are
				// already in aHypo
				double threshold = 0.80;  // this needs to be configurable
				double percentCommon = static_cast<double>(hasPickCount)
					/ static_cast<double>(numPicks);

				if (percentCommon >= threshold) {
					glass3::util::Logger::log(
						"debug", "CHypoList::addHypo: Existing Hypo with "
						+ glass3::util::to_string_with_precision(percentCommon * 100, 1)
						+ "% common picks found. Existing Hypo: "
						+ aHypo->getID()
						+ "; ot:"
						+ glass3::util::Date::encodeDateTime(aHypo->getTOrigin())
						+ "; lat:"
						+ glass3::util::to_string_with_precision(aHypo->getLatitude(), 3)
						+ "; lon:"
						+ glass3::util::to_string_with_precision(aHypo->getLongitude(), 3)
						+ "; z:"
						+ glass3::util::to_string_with_precision(aHypo->getDepth())
						+ "; picks:"
						+ std::to_string(aHypo->getPickDataSize())
						+ ". Not adding new Hypo: "
						+ hypo->getID()
						+ "; ot:"
						+ glass3::util::Date::encodeDateTime(hypo->getTOrigin())
						+ "; lat:"
						+ glass3::util::to_string_with_precision(hypo->getLatitude(), 3)
						+ "; lon:"
						+ glass3::util::to_string_with_precision(hypo->getLongitude(), 3)
						+ "; z:"
						+ glass3::util::to_string_with_precision(hypo->getDepth())
						+ "; picks:"
						+ std::to_string(numPicks)
						+ " common pick threshold: "
						+ glass3::util::to_string_with_precision(threshold * 100, 1)
						+ "%.");

					if (scheduleProcessing == true) {
						appendToHypoProcessingQueue(aHypo);
					}

					// didn't add the hypo
					return(false);
				}  // end if percentage common above threshold
			}  // end if weak_ptr valid
		}  // end for each hypo in vector
	}  // end if there were close hypos

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

		// check to see if the new hypo is older than the
		// first hypo in multiset
		if (hypo->getTOrigin() <= oldestHypo->getTOrigin()) {
			// it is, don't insert
			// message was processed
			return (true);
		}

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
	// use the pick time minus 3600 seconds to compute the starting index
	// NOTE: Hard coded time delta
	std::vector<std::weak_ptr<CHypo>> hypoList = getHypos(
			pk->getTPick() - k_nHypoSearchPastDurationForPick,
			pk->getTPick() + 10.0);

	// make sure we got any hypos
	if (hypoList.size() == 0) {
		// nope
		return (false);
	}

	std::shared_ptr<CHypo> bestHyp = NULL;
	double bestBayes = -1;

	double sdassoc = CGlass::getAssociationSDCutoff();

	// for each hypo in the list within the time range
	for (int i = 0; i < hypoList.size(); i++) {
		// make sure hypo is still valid before associating
		if (std::shared_ptr<CHypo> hyp = hypoList[i].lock()) {
			// get the affinity this pick has with this hypo
			double bayesValue = hyp->getBayesValue();
			double nucleationThreshold = hyp->getNucleationStackThreshold();

			// move on if it cannot associate
			if (hyp->canAssociate(pk, CGlass::k_dAssociationSecondsPerSigma,
									sdassoc, false, true) == false) {
				continue;
			}

			// add to the list of hypos this pick *can* associate with
			assocHypoList.push_back(hyp);

			// check to see if this hypo is the biggest and valid
			if ((bayesValue >= nucleationThreshold) &&
				(bayesValue > bestBayes)) {
				// remember the biggest hypo
				bestHyp = hyp;
				bestBayes = bayesValue;
			}
		}
	}

	// there were no hypos that the pick associated with
	if (assocHypoList.size() <= 0) {
		return (false);
	}

	// Associate to the hypo that is the biggest
	if (bestHyp != NULL) {
		// link the pick to the hypo
		pk->addHypoReference(bestHyp, true);

		// link the hypo to the pick
		bestHyp->addPickReference(pk);

		glass3::util::Logger::log(
				"debug",
				"CHypoList::associate ASSOC idPick:" + pk->getID()
				+ "; idHypo: " + bestHyp->getID()
				+ "; bayes: " + std::to_string(bestBayes));
	}

	// For each hypo that the pick could associate with
	// schedule it for processing so all the canidates
	// can fight it out
	for (auto q : assocHypoList) {
		// reset the cycle count
		q->setProcessCount(0);

		// add the hypo to the processing queue
		// note that we didn't link the pick to any hypos
		appendToHypoProcessingQueue(q);
	}

	// the pick was associated
	return (true);
}

// ---------------------------------------------------------fitData
bool CHypoList::fitData(std::shared_ptr<CPick> pk) {
	// nullcheck
	if (pk == NULL) {
		glass3::util::Logger::log(
				"warning", "CHypoList::associate: NULL pick provided.");

		return (false);
	}
	bool debug = true;
	std::vector<std::shared_ptr<CHypo>> assocHypoList;

	// compute the list of hypos to associate with
	// (a potential hypo must be before the pick we're associating)
	// use the pick time minus 3600 seconds to compute the starting index
	// NOTE: Hard coded time delta
	std::vector<std::weak_ptr<CHypo>> hypoList = getHypos(
			pk->getTPick() - k_nHypoSearchPastDurationForPick,
			pk->getTPick() + 10.0);

	// make sure we got any hypos
	if (hypoList.size() == 0) {
		if (debug) {
			glass3::util::Logger::log("debug",
				"CHypoList::fitData: No hypos to check Pick: "
				+ pk->getID() + " ("
				+ pk->getSite()->getSCNL() + ")"
				+ " with in range "
				+ std::to_string(pk->getTPick()
					- k_nHypoSearchPastDurationForPick)
				+ " to "
				+ std::to_string(pk->getTPick()
					+ 10.0));
		}
		// nope
		return (false);
	}

	// for each hypo in the list within the time range
	for (int i = 0; i < hypoList.size(); i++) {
		// make sure hypo is still valid before associating
		if (std::shared_ptr<CHypo> hyp = hypoList[i].lock()) {
			// compute ratio to threshold
			double adBayesRatio = (hyp->getBayesValue())
				/ (hyp->getNucleationStackThreshold());

			// check to see if the ratio is high enough
			// NOTE MAKE CONFIGURABLE
			if (adBayesRatio > 6.0) {
				double travelTimeP =
						hyp->getTravelTimeForPhase(pk, "P");
				double travelTimeS =
						hyp->getTravelTimeForPhase(pk, "S");
				double travelTimeObs = pk->getTPick()
										- hyp->getTOrigin();
				double distance = hyp->calculateDistanceToPick(pk);
				double distanceLimit = std::min(17.5,
										hyp->getAssociationDistanceCutoff());

				// check to see if this pick fits between predicted
				// P and predicted S and within the distance limit
				if ((travelTimeObs >= travelTimeP) &&
					(travelTimeObs <= travelTimeS) &&
					(distance <= distanceLimit)) {
					if (debug) {
						glass3::util::Logger::log("debug",
							"CHypoList::fitData: Pick: "
							+ pk->getID() + " ("
							+ pk->getSite()->getSCNL() + ")"
							+ " fits with hypo "
							+ hyp->getID()
							+ " P: "
							+ std::to_string(travelTimeP)
							+ " obs: "
							+ std::to_string(travelTimeObs)
							+ " S: "
							+ std::to_string(travelTimeS)
							+ " distance: "
							+ std::to_string(distance)
							+ " distanceLimit: "
							+ std::to_string(distanceLimit)
							+ " ratio: "
							+ std::to_string(adBayesRatio));
					}
					// this pick 'fit' for a very qualified
					// definition of the word 'fit'
					return(true);
				}
			}
		}
	}

	if (debug) {
		glass3::util::Logger::log("debug",
			"CHypoList::fitData: Pick: "
			+ pk->getID() + " ("
			+ pk->getSite()->getSCNL() + ")"
			+ " did not fit with any hypos");
	}
	// this pick did not 'fit'
	return(false);
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
	m_iMaxAllowableHypoCount = k_nMaxAllowableHypoCountDefault;
}

// ---------------------------------------------------------work
glass3::util::WorkState CHypoList::work() {
	// don't bother if there's nothing to do
	if (getHypoProcessingQueueLength() < 1) {
		// on to the next loop
		return (glass3::util::WorkState::Idle);
	}

	// log the cycle count and queue size
	char sLog[glass3::util::Logger::k_nMaxLogEntrySize];

	// get the next hypo to process
	std::shared_ptr<CHypo> hyp = getNextHypoFromProcessingQueue();

	// check to see if we got a valid hypo
	if (!hyp) {
		// nothing to process, move on
		// on to the next loop
		return (glass3::util::WorkState::Idle);
	}

	// std::lock_guard<std::mutex> hypoGuard(hyp->getProcessingMutex());

	// we need to lock hyp for processing
	int tryCount = 0;
	int tryLimit = 10 * 10;  // 10 second converted to tenths of a second
							 // (the sleep time)
	std::unique_lock<std::mutex> hypoGuard(hyp->getProcessingMutex(),
										std::defer_lock);

	// loop for awhile trying to get lock
	while (!hypoGuard.try_lock()) {
		tryCount++;
		if (tryCount <= tryLimit) {
			// didn't get lock, wait a bit
			std::this_thread::sleep_for(
							std::chrono::milliseconds(100));
			setThreadHealth();
		} else {
			break;
		}
	}

	// last try
	if (!hypoGuard.owns_lock()) {
		if (!hypoGuard.try_lock()) {
			// didn't get it, give up
			snprintf(sLog, sizeof(sLog),
						"CHypoList::work: could not"
						" lock %s for processing after 10s, continuing.",
						hyp->getID().c_str());
			glass3::util::Logger::log(sLog);

			// we can't lock this hypo
			// put it back on the queue
			appendToHypoProcessingQueue(hyp);

			// move on
			return (glass3::util::WorkState::OK);
		}
	}

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
			setThreadHealth();
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

	// Current Workflow is:
	// localize()
	// merge()
	// scavenge(picks) (re-localize() if succesfull)
	// scavenge(correlations) (re-localize() if successful)
	// pruneData() (localize() and re-pruneData() if successful)
	// resolveData()(localize() if successful)
	// cancelCheck() (removeHypo() if cancelled)

	// locate the hypo
	hyp->localize();

	std::chrono::high_resolution_clock::time_point tLocalizeEndTime =
			std::chrono::high_resolution_clock::now();
	double localizeTime = std::chrono::duration_cast<
			std::chrono::duration<double>>(tLocalizeEndTime - tEvolveStartTime)
			.count();

	// now that we've got a location, see if we can merge any proximal events
	// note that if successful findAndMergeMatchingHypos does a localize
	if (findAndMergeMatchingHypos(hyp)) {
		setThreadHealth();

		// we should report this hypo since it has changed
		breport = true;

		// make sure we didn't merge ourself out of existance
		if (hyp->cancelCheck()) {
			glass3::util::Logger::log(
				"debug",
				"CHypoList::processHypo: Canceled sPid:" + pid + " cycle:"
						+ std::to_string(hyp->getProcessCount())
						+ " processCount:"
						+ std::to_string(hyp->getTotalProcessCount())
						+ " after or in merge.");

			// probably already removed, but best be safe
			removeHypo(hyp);

			// return false since the hypo was canceled.
			return(false);
		}
	}

	setThreadHealth();

	std::chrono::high_resolution_clock::time_point tMergeEndTime =
			std::chrono::high_resolution_clock::now();
	double mergeTime =
			std::chrono::duration_cast<std::chrono::duration<double>>(
					tMergeEndTime - tLocalizeEndTime).count();

	// Search for any associable picks that match hypo in the pick list
	// NOTE: This uses the hard coded 3600 second scavenge duration default
	if (CGlass::getPickList()->scavenge(hyp)) {
		setThreadHealth();

		// we should report this hypo since it has changed
		breport = true;
		// relocate the hypo
		hyp->localize();
	}

	setThreadHealth();

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
			std::chrono::duration<double>>(tScavengeEndTime - tMergeEndTime)
			.count();

	// Remove data that no longer fit hypo's association criteria
	if (hyp->pruneData(this)) {
		setThreadHealth();

		// we should report this hypo since it has changed
		breport = true;
		// relocate the hypo
		hyp->localize();

		// Iterate on pruning data
		if (hyp->pruneData(this)) {
			// we should report this hypo since it has changed
			breport = true;
			// relocate the hypo
			hyp->localize();
		}
	}

	setThreadHealth();

	std::chrono::high_resolution_clock::time_point tPruneEndTime =
			std::chrono::high_resolution_clock::now();
	double pruneTime =
			std::chrono::duration_cast<std::chrono::duration<double>>(
					tPruneEndTime - tScavengeEndTime).count();

	// Ensure all remaining data belong to hypo
	if (resolveData(hyp)) {
		setThreadHealth();

		// we should report this hypo out since it has changed
		breport = true;

		// relocate the hypo
		hyp->localize();
	}

	setThreadHealth();

	std::chrono::high_resolution_clock::time_point tResolveEndTime =
			std::chrono::high_resolution_clock::now();
	double resolveTime = std::chrono::duration_cast<
			std::chrono::duration<double>>(tResolveEndTime - tPruneEndTime)
			.count();

	// check to see if this hypo is viable.
	if (hyp->cancelCheck()) {
		std::chrono::high_resolution_clock::time_point tCancelEndTime =
				std::chrono::high_resolution_clock::now();
		double cancelTime = std::chrono::duration_cast<
				std::chrono::duration<double>>(tCancelEndTime - tResolveEndTime)
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
						+ std::to_string(resolveTime) + " pruneTime:"
						+ std::to_string(scavengeTime) + " resolveTime:"
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
					tCancelEndTime - tResolveEndTime).count();

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
	if (hyp->getTotalProcessCount() <= 1) {
		glass3::util::Logger::log(
				"debug",
				"CHypoList::processHypo: Should report new hypo sPid:" + pid
						+ " cycle:" + std::to_string(hyp->getProcessCount())
						+ " processCount:"
						+ std::to_string(hyp->getTotalProcessCount()));

		// we should always report a new hypo (ensure it reports at least once)
		breport = true;
	}

	// if we're supposed to report out a summary of the hypo outside
	// of glasscore
	if (breport == true) {
		// if we CAN report
		if (hyp->reportCheck() == true) {
			// report to anyone listening outside of glasscore
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
					tReportEndTime - tPruneEndTime).count();

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
					+ std::to_string(cancelTime) + " mergeTime:"
					+ std::to_string(localizeTime) + " scavengeTime:"
					+ std::to_string(scavengeTime) + " resolveTime:"
					+ std::to_string(pruneTime) + " cancelTime:"
					+ std::to_string(resolveTime) + " pruneTime:"
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

	std::string primaryID = hypo->getID();
	char sLog[glass3::util::Logger::k_nMaxLogEntrySize];  // logging string
	double distanceCut = CGlass::getHypoMergingDistanceWindow();
	double timeCut = CGlass::getHypoMergingTimeWindow();
	bool merged = false;

	// Get the list of hypos to try merging with with
	// (a potential hypo must be within time cut to consider)
	std::vector<std::weak_ptr<CHypo>> mergeList = getHypos(
			hypo->getTOrigin() - timeCut, hypo->getTOrigin() + timeCut);

	setThreadHealth();

	// make sure we got hypos returned
	if (mergeList.size() == 0) {
		// Log that there were no other events close enough to merge with
		snprintf(sLog, sizeof(sLog),
					"CHypoList::findAndMergeMatchingHypos: No hypos in merge "
					"window %s to %s for %s (%s)",
					glass3::util::Date::encodeDateTime(
						hypo->getTOrigin() - timeCut).c_str(),
					glass3::util::Date::encodeDateTime(
						hypo->getTOrigin() + timeCut).c_str(),
					hypo->getID().c_str(),
					glass3::util::Date::encodeDateTime(
						hypo->getTOrigin()).c_str());
		glass3::util::Logger::log(sLog);
		return (merged);
	} else {
		// snprintf(sLog, sizeof(sLog),
		// "CHypoList::findAndMergeMatchingHypos: %d hypos in merge "
		// "window %.3f to %.3f for %s (%.3f)",
		// static_cast<int>(mergeList.size()),
		// hypo->getTOrigin() - timeCut,
		// hypo->getTOrigin() + timeCut,
		// hypo->getID().c_str(),
		// hypo->getTOrigin());
		// glass3::util::Logger::log(sLog);
	}

	// for each hypo in the mergeList
	for (int i = 0; i < mergeList.size(); i++) {
		setThreadHealth();

		// get canidate hypo from weak pointer
		std::shared_ptr<CHypo> aHypo = mergeList[i].lock();

		// make sure canidate hypo is still valid before merging
		if (aHypo == NULL) {
			continue;
		}
		if (aHypo->getID() == "") {
			continue;
		}

		std::string currentID = aHypo->getID();

		// make sure we're not looking at ourself
		if (primaryID == currentID) {
			// snprintf(
			// sLog, sizeof(sLog),
			// "CHypoList::findAndMergeMatchingHypos: Skipping self"
			// " aHypo %s == hypo %s",
			// primaryID.c_str(), currentID.c_str());
			// glass3::util::Logger::log(sLog);
			continue;
		}

		snprintf(
				sLog, sizeof(sLog),
				"CHypoList::findAndMergeMatchingHypos: Testing merger of"
				" hypo %s and %s",
				primaryID.c_str(), currentID.c_str());
		glass3::util::Logger::log(sLog);

		// lock ahypo for processing (we already have hypo locked)
		// (couldn't get try_lock_until to build)
		int tryCount = 0;
		std::unique_lock<std::mutex> lock(aHypo->getProcessingMutex(),
											std::defer_lock);

		// loop for awhile trying to get lock
		while (!lock.try_lock()) {
			tryCount++;
			if (tryCount <= 5) {
				// didn't get lock, wait a bit
				std::this_thread::sleep_for(
								std::chrono::milliseconds(100));
				setThreadHealth();
			} else {
				break;
			}
		}

		// last try
		if (!lock.owns_lock()) {
			if (!lock.try_lock()) {
				// didn't get it, give up
				snprintf(sLog, sizeof(sLog),
							"CHypoList::findAndMergeMatchingHypos: could not"
							" lock %s for merging after 500ms, continuing.",
							currentID.c_str());
				glass3::util::Logger::log(sLog);

				// we don't have this hypo locked
				// move on
				continue;
			}
		}

		// prefer to merge into the hypo that has already been published
		std::shared_ptr<CHypo> intoHypo;  // The hypo we are merging into
		std::shared_ptr<CHypo> fromHypo;  // The hypo we are merging from
		if ((hypo->getHypoGenerated() == false)
				&& (aHypo->getHypoGenerated() == true)) {
			intoHypo = aHypo;
			fromHypo = hypo;
		} else if ((hypo->getHypoGenerated() == true)
				&& (aHypo->getHypoGenerated() == false)) {
			intoHypo = hypo;
			fromHypo = aHypo;
		} else {
			// otherwise prefer the "older" event
			// if (hypo->getPickDataSize() >= aHypo->getPickDataSize()) {
			if (hypo->getTCreate() <= aHypo->getTCreate()) {
				intoHypo = hypo;
				fromHypo = aHypo;
			} else {
				intoHypo = aHypo;
				fromHypo = hypo;
			}
		}

		// skip deleted hypos
		if ((intoHypo->getID() == "") || (fromHypo->getID() == "")) {
			continue;
		}

		// Log info on the two hypos
		snprintf(sLog, sizeof(sLog),
				"CHypoList::findAndMergeMatchingHypos: intoHypo:%s lat:%.3f, "
				"lon:%.3f, depth:%.3f, time:%s, bayes: %.3f, nPicks:%d "
				"created: %.3f, pub: %s",
				intoHypo->getID().c_str(), intoHypo->getLatitude(),
				intoHypo->getLongitude(), intoHypo->getDepth(),
				glass3::util::Date::encodeDateTime(
					intoHypo->getTOrigin()).c_str(), intoHypo->getBayesValue(),
				static_cast<int>(intoHypo->getPickData().size()),
				intoHypo->getTCreate(),
				intoHypo->getHypoGenerated() ? "true" : "false");
		glass3::util::Logger::log(sLog);

		snprintf(sLog, sizeof(sLog),
				"CHypoList::findAndMergeMatchingHypos: fromHypo:%s lat:%.3f, "
				"lon:%.3f, depth:%.3f, time:%s, bayes: %.3f, nPicks:%d "
				"created: %.3f, pub: %s",
				fromHypo->getID().c_str(), fromHypo->getLatitude(),
				fromHypo->getLongitude(), fromHypo->getDepth(),
				glass3::util::Date::encodeDateTime(
					fromHypo->getTOrigin()).c_str(), fromHypo->getBayesValue(),
				static_cast<int>(fromHypo->getPickData().size()),
				fromHypo->getTCreate(),
				fromHypo->getHypoGenerated() ? "true" : "false");
		glass3::util::Logger::log(sLog);

		// get geo objects
		glass3::util::Geo fromGeo;
		fromGeo.setGeographic(fromHypo->getLatitude(),
								fromHypo->getLongitude(),
								glass3::util::Geo::k_EarthRadiusKm);

		glass3::util::Geo toGeo;
		toGeo.setGeographic(intoHypo->getLatitude(), intoHypo->getLongitude(),
							glass3::util::Geo::k_EarthRadiusKm);

		// calculate distance between hypos
		double distanceDiff = toGeo.delta(&fromGeo)
				/ glass3::util::GlassMath::k_DegreesToRadians;

		// check distance between hypos
		if (distanceDiff > distanceCut) {
			// didn't get it, give up
			snprintf(sLog, sizeof(sLog),
						"CHypoList::findAndMergeMatchingHypos: distance"
						" between fromHypo %s into intoHypo %s is %.3f"
						" which is greater than cutoff %.3f, continuing",
						fromHypo->getID().c_str(), intoHypo->getID().c_str(),
						distanceDiff, distanceCut);
			glass3::util::Logger::log(sLog);
			continue;
		}

		// resolve and prune both hypos before starting merging so
		// we know they're not sharing picks.
		intoHypo->pruneData(this);
		resolveData(intoHypo);
		snprintf(sLog, sizeof(sLog),
				"CHypoList::findAndMergeMatchingHypos: %d picks"
				" in intoHypo %s after resolve",
				static_cast<int>(intoHypo->getPickData().size()),
				intoHypo->getID().c_str());
		glass3::util::Logger::log(sLog);

		fromHypo->pruneData(this);
		resolveData(fromHypo);
		snprintf(sLog, sizeof(sLog),
				"CHypoList::findAndMergeMatchingHypos: %d picks"
				" in fromHypo %s after resolve",
				static_cast<int>(fromHypo->getPickData().size()),
				fromHypo->getID().c_str());
		glass3::util::Logger::log(sLog);

		// check hypos to see if resolve removed enough
		// of the picks to kill the hypo,
		// if so, remove the hypo
		bool resolveRemoved = false;
		if (intoHypo->cancelCheck() == true) {
			removeHypo(intoHypo);

			// we've merged a hypo, kinda
			merged = true;
			resolveRemoved = true;
		}
		if (fromHypo->cancelCheck() == true) {
			removeHypo(fromHypo);

			// we've merged a hypo, sorta
			merged = true;
			resolveRemoved = true;
		}

		// if we've removed a hypo via resolve,
		// no point in continuing, we either have
		// nothing to merge into, or nothing to
		// merge from
		if (resolveRemoved == true) {
			// if we've removed the primary hypo
			// we're done with findAndMerge
			if (hypo->getID() == "") {
				snprintf(sLog, sizeof(sLog),
					"CHypoList::findAndMergeMatchingHypos: Primary"
					" hypo %s removed, (0 phases after resolve)"
					" returning", primaryID.c_str());
				glass3::util::Logger::log(sLog);

				return(merged);
			} else {
				// otherwise continue on
				// to the next hypo in the merge list
				snprintf(sLog, sizeof(sLog),
					"CHypoList::findAndMergeMatchingHypos: "
					" Current Hypo %s removed, (0 phases after"
					" resolve) continuing", currentID.c_str());
				glass3::util::Logger::log(sLog);
				continue;
			}
		}

		// get picks
		auto intoPicks = intoHypo->getPickData();
		auto fromPicks = fromHypo->getPickData();

		// get bayes values, calculate them since we just revolved
		double intoBayes = intoHypo->calculateCurrentBayes();
		double fromBayes = fromHypo->calculateCurrentBayes();

		// add all picks from fromHypo into intoHypo
		int addPickCount = 0;
		for (auto pick : fromPicks) {
			if (intoHypo->addPickReference(pick) == true) {
				addPickCount++;
			}
		}

		snprintf(sLog, sizeof(sLog),
				"CHypoList::findAndMergeMatchingHypos: Added %d picks"
				" from fromHypo %s to intoHypo %s fromHypo pick count"
				" now %d", addPickCount, fromHypo->getID().c_str(),
				intoHypo->getID().c_str(),
				static_cast<int>(intoHypo->getPickData().size()));
		glass3::util::Logger::log(sLog);

		// initial localization attempt of intoHypo after adding picks
		intoHypo->localize();

		// Remove picks from intoHypo that do not fit initial location
		if (intoHypo->pruneData(this)) {
			// relocate the intoHypo if we pruned
			intoHypo->localize();
		}

		snprintf(sLog, sizeof(sLog),
				"CHypoList::findAndMergeMatchingHypos: %d picks"
				" in intoHypo %s after localize and prune",
				static_cast<int>(intoHypo->getPickData().size()),
				intoHypo->getID().c_str());
		glass3::util::Logger::log(sLog);

		setThreadHealth();

		// get the new bayes value and calculate threshold
		double newBayes = intoHypo->calculateCurrentBayes();
		double threshold = std::max(intoBayes, fromBayes)
							+ (k_dMergeStackImprovementRatio
							* std::min(intoBayes, fromBayes));

		snprintf(
				sLog, sizeof(sLog),
				"CHypoList::findAndMergeMatchingHypos: Merge Check:"
				" newBayes:%.3f > threshold:%.3f, else"
				" newBayes:%.3f >= intoHypo Bayes:%.3f"
				" (* k_dMinimumRoundingProtectionRatio)",
				newBayes, threshold, newBayes,
				(intoBayes * k_dMinimumRoundingProtectionRatio));
		glass3::util::Logger::log(sLog);

		// check that the new bayes is better than either of the original
		// bayes values
		if (newBayes > threshold) {
			snprintf(
					sLog, sizeof(sLog),
					"CHypoList::findAndMergeMatchingHypos: merged fromHypo "
					"%s into intoHypo %s because intoHypo is better "
					"than fromHypo and original intoHypo, newBayes:%.3f > "
					"intoHypo Bayes:%.3f, fromHypo bayes:%.3f",
					fromHypo->getID().c_str(), intoHypo->getID().c_str(),
					newBayes, intoBayes, fromBayes);
			glass3::util::Logger::log(sLog);

			if (fromHypo->getTCreate() < intoHypo->getTCreate()) {
				intoHypo->setTCreate(fromHypo->getTCreate());
			}

			// intoHypo has effectively been replaced by the result
			// of the combination of intoHypo and fromHypo. So now remove
			// fromHypo, since it was successfully merged
			removeHypo(fromHypo);

			// we've merged a hypo, move on to the next candidate
			merged = true;
		} else if (newBayes
				>= intoBayes * k_dMinimumRoundingProtectionRatio) {
			// error or minor issue in location?
			// the new Hypo is at least as good as the old intoHypo but it
			// hasn't improved significantly. This could be because either
			// fromHypo has a subset of the picks of the intoHypo, or because
			// the two hypos are unrelated and share no picks. Resolve
			// duplicate picks and see if the weaker hypo can still stand.
			if (resolveData(intoHypo)) {
				// relocate the intoHypo if we resolved to get an updated
				// bayes value
				intoHypo->localize();
			}

			// relocate and more importantly re-calc statistics for the
			// fromHypo, now that we've potentially ripped picks away from
			// it.
			fromHypo->localize();

			// Resolve duplicate picks, using our increased relative
			// strength as we've stripped dup's away from fromHypo.
			if (resolveData(intoHypo)) {
				// relocate the intoHypo if we resolved to get an updated
				// bayes value
				intoHypo->localize();
			}

			// check to see if fromHypo is still healthy
			if (fromHypo->cancelCheck()) {
				snprintf(
						sLog,
						sizeof(sLog),
						"CHypoList::findAndMergeMatchingHypos: merged "
						"fromHypo %s into intoHypo %s because fromHypo failed "
						"cancelCheck, intoHypo:Bayes %.3f, fromHypo:bayes %.3f",
						fromHypo->getID().c_str(), intoHypo->getID().c_str(),
						intoHypo->getBayesValue(), fromHypo->getBayesValue());
				glass3::util::Logger::log(sLog);

				if (fromHypo->getTCreate() < intoHypo->getTCreate()) {
					intoHypo->setTCreate(fromHypo->getTCreate());
				}

				// fromHypo isn't strong enough to stand on it's own after
				// we've resolved picks with intoHypo.  Merge essentially
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
				// belong to both it and intoHypo forever, so...
				// And if there were NO overlapping picks between the two
				// events, then we haven't done anything to fromHypo at
				// all...
				// log something here?

				// the merged hypo (intoHypo) was not better, revert intoHypo.
				// and fromHypo
				snprintf(
						sLog, sizeof(sLog),
						"CHypoList::findAndMergeMatchingHypos: keeping "
						"modified hypos %s and %s because fromHypo passed "
						"cancelCheck, intoHypo Bayes:%.3f, fromHypo "
						"bayes:%.3f",
						intoHypo->getID().c_str(), fromHypo->getID().c_str(),
						intoHypo->getBayesValue(), fromHypo->getBayesValue());
				glass3::util::Logger::log(sLog);
			}  // end else (fromHypo->cancelCheck())
		} else {
			// the merged hypo (intoHypo) was not better, revert intoHypo.
			snprintf(
					sLog,
					sizeof(sLog),
					"CHypoList::findAndMergeMatchingHypos: reverting original "
					"hypo %s, newBayes:%.3f, intoHypo Bayes:%.3f",
					intoHypo->getID().c_str(),
					newBayes, intoBayes);
			glass3::util::Logger::log(sLog);

			// reset intoHypo to where it was
			intoHypo->clearPickReferences();

			// add the original intoHypo picks
			for (auto pick : intoPicks) {
				intoHypo->addPickReference(pick);
			}

			// relocate intoHypo
			intoHypo->localize();
		}  // end else is toHypoBetter
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

	setThreadHealth();

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

	// log performance info
	const HypoAuditingPerformanceStruct * perfInfo = hypo
			->getHypoAuditingPerformanceInfo();
	glass3::util::Logger::log(
			"info",
			"CHypoList::removeHypo Final Audit for: " + hypo->getID() + " "
					+ std::to_string(hypo->getLatitude()) + " "
					+ std::to_string(hypo->getLongitude()) + " "
					+ std::to_string(hypo->getDepth()) + " "
					+ std::to_string(hypo->getTOrigin()) + " "
					+ std::to_string(perfInfo->dtOrigin) + " "
					+ std::to_string(perfInfo->dtNucleated) + " "
					+ std::to_string(perfInfo->dtNucleationPickInsertion) + " "
					+ std::to_string(perfInfo->dtLastBigMove) + " "
					+ std::to_string(perfInfo->nMaxPhasesBeforeMove) + " "
					+ std::to_string(perfInfo->dMaxStackBeforeMove) + " "
					+ std::to_string(perfInfo->nMaxPhasesSinceMove) + " "
					+ std::to_string(perfInfo->dMaxStackSinceMove) + " "
					+ std::to_string(perfInfo->dtFirstEventMessage) + " "
					+ std::to_string(perfInfo->dtFirstHypoMessage) + " "
					+ std::to_string(perfInfo->dLatPrev) + " "
					+ std::to_string(perfInfo->dLonPrev) + " "
					+ std::to_string(perfInfo->dDepthPrev) + " "
					+ std::to_string(hypo->getProcessCount()) + " "
					+ std::to_string(hypo->getBayesValue()) + " "
					+ std::to_string(hypo->getPickDataSize()) + " "
					+ std::to_string(hypo->getInitialBayesValue()) + " "
					+ std::to_string(hypo->getMinDistance()) + " "
					+ std::to_string(hypo->getMedianDistance()) + " "
					+ std::to_string(hypo->getGap()) + " "
					+ std::to_string(hypo->getDistanceSD()) + " "
					+ std::to_string(hypo->getAssociationDistanceCutoff()) + " "
					+ std::to_string(hypo->getWebResolution()) + "  ABC");

	// Send cancellation message for this hypo
	if (reportCancel == true) {
		// only if we've sent an event message
		if (hypo->getEventGenerated()) {
			// create cancellation message
			CGlass::sendExternalMessage(hypo->generateCancelMessage());
		}
	}

	setThreadHealth();

	// remove from from multiset
	eraseFromMultiset(hypo);

	// erase this hypo from the map
	m_HypoListMutex.lock();
	m_mHypo.erase(hypo->getID());
	m_HypoListMutex.unlock();

	setThreadHealth();

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

	// resolve all data prior to generating the hypo
	if (resolveData(hyp)) {
		// relocate the hypo
		hyp->localize();
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
	return (hyp->resolveData(hyp, allowStealing, this));
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

	setThreadHealth();

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

	setThreadHealth();

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
		setThreadHealth();

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
		setThreadHealth();

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

#include <json.h>
#include <string>
#include <memory>
#include <utility>
#include <algorithm>
#include <vector>
#include <map>
#include <ctime>
#include <random>
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
CHypoList::CHypoList(int numThreads, int sleepTime, int checkInterval)
		: glass3::util::ThreadBaseClass("HypoList", sleepTime, numThreads,
										checkInterval) {
	// seed the random number generator
	std::random_device randomDevice;
	m_RandomGenerator.seed(randomDevice());

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
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypoList::addHypo: NULL hypo provided.");

		return (false);
	}

	// set some basic hypo values from pGlass if we have it
	if (pGlass) {
		hypo->setGlass(pGlass);
		hypo->setDistanceCutoffFactor(pGlass->getDistanceCutoffFactor());
		hypo->setDistanceCutoffPercentage(
				pGlass->getDistanceCutoffPercentage());
		hypo->setMinDistanceCutoff(pGlass->getMinDistanceCutoff());
	}

	// lock for this scope
	std::lock_guard<std::recursive_mutex> listGuard(m_vHypoMutex);

	// Add hypo to cache (mHypo) and time sorted
	// index (vHypo). If vHypo has reached its
	// maximum capacity (nHypoMax), then the
	// first hypo in the vector is removed and the
	// corresponding entry in the cache is erased.
	nHypoTotal++;

	// get maximum number of hypos
	// use max picks from pGlass if we have it
	if (pGlass) {
		nHypoMax = pGlass->getMaxNumHypos();
	}

	// create pair for insertion
	std::pair<double, std::string> p(hypo->getTOrigin(), hypo->getID());

	// remove oldest hypo if this new one
	// pushes us over the limit
	if (vHypo.size() == nHypoMax) {
		// get first hypo in vector
		std::pair<double, std::string> pdx;
		pdx = vHypo[0];
		std::shared_ptr<CHypo> firstHypo = mHypo[pdx.second];

		// send expiration message
		firstHypo->expire();

		// remove it
		remHypo(firstHypo, false);

		glassutil::CLogit::log(
				glassutil::log_level::debug,
				"CHypoList::addHypo: Current: "
						+ std::to_string(static_cast<int>(vHypo.size()))
						+ " Max: " + std::to_string(nHypoMax)
						+ " Removing Hypo: " + pdx.second);
	}

	// Insert new hypo in proper time sequence into hypo vector
	// get the index of the new hypo
	int ihypo = indexHypo(hypo->getTOrigin());
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
	mHypo[hypo->getID()] = hypo;

	// Schedule this hypo for refinement. Note that this
	// hypo will be the first one in the queue, and will be the
	// first one processed.
	if (scheduleProcessing == true) {
		pushFifo(hypo);
	}

	// done
	return (true);
}

// ---------------------------------------------------------associate
bool CHypoList::associate(std::shared_ptr<CPick> pk) {
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

	std::vector<std::shared_ptr<CHypo>> viper;

	// compute the list of hypos to associate with
	// (a potential hypo must be before the pick we're associating)
	// use the pick time minus 2400 seconds to compute the starting index
	// NOTE: Hard coded time delta
	std::vector<std::weak_ptr<CHypo>> hypoList = getHypos(pk->getTPick() - 2400,
															pk->getTPick());

	// make sure we got any hypos
	if (hypoList.size() == 0) {
		glassutil::CLogit::log(
				glassutil::log_level::debug,
				"CHypoList::associate NOASSOC idPick:"
						+ std::to_string(pk->getIdPick())
						+ "; No Usable Hypos");
		// nope
		return (false);
	}

	std::shared_ptr<CHypo> bestHyp;
	double sdassoc = pGlass->getAssociationSDCutoff();

	// for each hypo in the list within the time range
	for (int i = 0; i < hypoList.size(); i++) {
		// make sure hypo is still valid before associating
		if (std::shared_ptr<CHypo> hyp = hypoList[i].lock()) {
			// check to see if the pick will associate with
			// this hypo
			// NOTE: The sigma value passed into associate is hard coded
			if (hyp->associate(pk, 1.0, sdassoc)) {
				// add to the list of hypos this pick can associate with
				viper.push_back(hyp);

				// remember this hypo
				bestHyp = hyp;
			}
		}
	}

	// there were no hypos that the pick associated with
	if (viper.size() < 1) {
		glassutil::CLogit::log(
				glassutil::log_level::debug,
				"CHypoList::associate NOASSOC idPick:"
						+ std::to_string(pk->getIdPick()));

		return (false);
	}

	// there was only one hypo that the pick associated with
	if (viper.size() == 1) {
		// log
		/*
		 char sLog[1024];
		 snprintf(
		 sLog, sizeof(sLog), "ASS %s %s %s (%d)\n",
		 hyp->getPid().c_str(),
		 glassutil::CDate::encodeDateTime(pk->getTPick()).c_str(),
		 pk->getSite()->getScnl().c_str(),
		 static_cast<int>(hyp->getVPickSize()));
		 glassutil::CLogit::Out(sLog);
		 */

		// link the pick to the hypo
		pk->addHypo(bestHyp, "", true);

		// link the hypo to the pick
		bestHyp->addPick(pk);

		glassutil::CLogit::log(
				glassutil::log_level::debug,
				"CHypoList::associate (pick) sPid:" + bestHyp->getID()
						+ " resetting cycle count due to new association");

		// reset the cycle count
		bestHyp->setCycleCount(0);

		// add to the processing queue
		pushFifo(bestHyp);

		glassutil::CLogit::log(
				glassutil::log_level::debug,
				"CHypoList::associate ASSOC idPick:"
						+ std::to_string(pk->getIdPick()) + "; numHypos:1");

		// the pick was associated
		return (true);
	}

	// For each hypo that the pick could associate with
	for (auto q : viper) {
		glassutil::CLogit::log(
				glassutil::log_level::debug,
				"CHypoList::associate (pick) sPid:" + q->getID()
						+ " resetting cycle count due to new association");

		// reset the cycle count
		q->setCycleCount(0);

		// add the hypo to the processing queue
		// note that we didn't link the pick to any hypos
		pushFifo(q);
	}

	glassutil::CLogit::log(
			glassutil::log_level::debug,
			"CHypoList::associate ASSOC idPick:"
					+ std::to_string(pk->getIdPick()) + "; numHypos:"
					+ std::to_string(viper.size()));

	// the pick was associated
	return (true);
}

// ---------------------------------------------------------associate
bool CHypoList::associate(std::shared_ptr<CCorrelation> corr) {
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

	bool bass = false;
	std::vector<std::shared_ptr<CHypo>> viper;

	// compute the index range to search for hypos to associate with
	// (a potential hypo must be before the pick we're associating)
	// use the correlation time minus correlationMatchingTWindow to compute the
	// starting index
	std::vector<std::weak_ptr<CHypo>> hypoList = getHypos(
			corr->getTCorrelation()
					- pGlass->getCorrelationMatchingTimeWindow(),
			corr->getTCorrelation()
					- pGlass->getCorrelationMatchingTimeWindow());

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
			if (hyp->associate(
					corr, pGlass->getCorrelationMatchingTimeWindow(),
					pGlass->getCorrelationMatchingDistanceWindow())) {
				// add to the list of hypos this correlation can associate with
				viper.push_back(hyp);

				// remember this hypo for later
				bestHyp = hyp;
			}
		}
	}

	// there were no hypos that the correlation associated with
	if (viper.size() < 1) {
		return (false);
	}

	// there was only one hypo that the correlation associated with
	if (viper.size() == 1) {
		// log
		/*
		 char sLog[1024];
		 snprintf(
		 sLog,
		 sizeof(sLog),
		 "C-ASS %s %s %s (%d)\n",
		 hyp->getPid().substr(0, 4).c_str(),
		 glassutil::CDate::encodeDateTime(corr->getTCorrelation())
		 .c_str(),
		 corr->getSite()->getScnl().c_str(),
		 static_cast<int>(hyp->getVCorrSize()));
		 glassutil::CLogit::Out(sLog);
		 */

		// link the correlation to the hypo
		corr->addHypo(bestHyp, true);

		// link the hypo to the correlation
		bestHyp->addCorrelation(corr);

		glassutil::CLogit::log(
				glassutil::log_level::debug,
				"CHypoList::associate (correlation) sPid:" + bestHyp->getID()
						+ " resetting cycle count due to new association");

		// reset the cycle count
		bestHyp->setCycleCount(0);

		// add to the processing queue
		pushFifo(bestHyp);

		// the pick was associated
		return (true);
	}

	// For each hypo that the correlation could associate with
	for (auto q : viper) {
		glassutil::CLogit::log(
				glassutil::log_level::debug,
				"CHypoList::associate (correlation) sPid:" + q->getID()
						+ " resetting cycle count due to new association");

		// reset the cycle count
		q->setCycleCount(0);

		// add the hypo to the processing queue
		// note that we didn't link the correlation to any hypos
		pushFifo(q);
	}

	// the pick was associated
	return (true);
}

// ---------------------------------------------------------clear
void CHypoList::clear() {
	std::lock_guard<std::recursive_mutex> hypoListGuard(m_HypoListMutex);

	clearHypos();
	pGlass = NULL;
}

// ---------------------------------------------------------clearHypos
void CHypoList::clearHypos() {
	std::lock_guard<std::mutex> queueGuard(m_QueueMutex);
	qFifo.clear();

	std::lock_guard<std::recursive_mutex> listGuard(m_vHypoMutex);
	vHypo.clear();
	mHypo.clear();

	// reset
	nHypoTotal = 0;
	nHypoMax = 100;
}

// ---------------------------------------------------------work
glass3::util::WorkState CHypoList::work() {
	// make sure we have a pGlass
	if (pGlass == NULL) {
		// on to the next loop
		return (glass3::util::WorkState::Idle);
	}

	// don't bother if there's nothing to do
	if (getFifoSize() < 1) {
		// on to the next loop
		return (glass3::util::WorkState::Idle);
	}

	// log the cycle count and queue size
	char sLog[1024];

	// get the next hypo to process
	std::shared_ptr<CHypo> hyp = popFifo();

	// check to see if we got a valid hypo
	if (!hyp) {
		// nothing to process, move on
		// on to the next loop
		return (glass3::util::WorkState::Idle);
	}

	std::lock_guard<std::mutex> hypoGuard(hyp->getProcessingMutex());

	try {
		// log the hypo we're working on
		glassutil::CLogit::log(
				glassutil::log_level::debug,
				"CHypoList::darwin Processing Hypo sPid:" + hyp->getID()
						+ " Cycle:" + std::to_string(hyp->getCycleCount())
						+ " Fifo Size:" + std::to_string(getFifoSize()));

		// check to see if this hypo is viable.
		if (hyp->cancelCheck()) {
			// this hypo is no longer viable
			// log
			glassutil::CLogit::log(
					glassutil::log_level::debug,
					"CHypoList::darwin canceling sPid:" + hyp->getID()
							+ " processCount:"
							+ std::to_string(hyp->getProcessCount()));

			// remove hypo from the hypo list
			remHypo(hyp);
			sort();

			// done with processing
			return (glass3::util::WorkState::OK);
		}

		// check to see if we've hit the iCycle Limit for this
		// hypo
		if (hyp->getCycleCount() >= pGlass->getCycleLimit()) {
			// log
			glassutil::CLogit::log(
					glassutil::log_level::debug,
					"CHypoList::darwin skipping sPid:" + hyp->getID()
							+ " at cycle limit:"
							+ std::to_string(hyp->getCycleCount())
							+ +" processCount:"
							+ std::to_string(hyp->getProcessCount()));

			return (glass3::util::WorkState::OK);
		}

		// process this hypocenter
		evolve(hyp);

		// resort the hypocenter list to maintain
		// time order
		sort();
	} catch (const std::exception &e) {
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"CHypoList::processHypos: Exception during work(): "
						+ std::string(e.what()));
		return (glass3::util::WorkState::Error);
	}

	// done with processing
	return (glass3::util::WorkState::OK);
}

// ---------------------------------------------------------Dispatch
bool CHypoList::dispatch(std::shared_ptr<json::Object> com) {
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

// ---------------------------------------------------------evolve
bool CHypoList::evolve(std::shared_ptr<CHypo> hyp) {
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

	std::string pid = hyp->getID();
	int OriginalPicks = hyp->getPickDataSize();
	std::chrono::high_resolution_clock::time_point tEvolveStartTime =
			std::chrono::high_resolution_clock::now();

	hyp->incrementProcessCount();
	hyp->setCycleCount(hyp->getCycleCount() + 1);

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
	if (pGlass->getPickList()->scavenge(hyp)) {
		// we should report this hypo since it has changed
		breport = true;
		// relocate the hypo
		hyp->localize();
	}

	// search for any associable correlations that match hypo in the correlation
	// list
	if (pGlass->getCorrelationList()->scavenge(hyp)) {
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

	// Iterate on pruning data
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

	// check to see if this hypo is viable.
	if (hyp->cancelCheck()) {
		std::chrono::high_resolution_clock::time_point tCancelEndTime =
				std::chrono::high_resolution_clock::now();
		double cancelTime = std::chrono::duration_cast<
				std::chrono::duration<double>>(tCancelEndTime - tPruneEndTime)
				.count();

		remHypo(hyp);
		sort();

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
						+ std::to_string(hyp->getCycleCount())
						+ " processCount:"
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

	// if event is all good check if proximal events can be merged.
	if (mergeCloseEvents(hyp)) {
		return (false);
	}

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
	if (hyp->getProcessCount() < 2) {
		glassutil::CLogit::log(
				glassutil::log_level::debug,
				"CHypoList::evolve: Should report new hypo sPid:" + pid
						+ " cycle:" + std::to_string(hyp->getCycleCount())
						+ " processCount:"
						+ std::to_string(hyp->getProcessCount()));

		// we should always report a new hypo (ensure it reports at least once)
		breport = true;
	}

	// if we're supposed to report
	if (breport == true) {
		// if we CAN report
		if (hyp->reportCheck() == true) {
			// report
			hyp->event();

			glassutil::CLogit::log(
					glassutil::log_level::debug,
					"CHypoList::evolve: Reported hypo sPid:" + pid + " cycle:"
							+ std::to_string(hyp->getCycleCount())
							+ " processCount:"
							+ std::to_string(hyp->getProcessCount()));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::debug,
					"CHypoList::evolve: hypo sPid:" + pid + " processCount:"
							+ std::to_string(hyp->getProcessCount())
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

	glassutil::CLogit::log(
			glassutil::log_level::debug,
			"CHypoList::evolve: Finished sPid:" + pid + " cycle:"
					+ std::to_string(hyp->getCycleCount()) + " processCount:"
					+ std::to_string(hyp->getProcessCount())
					+ " Evolve Timing: localizeTime:"
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
	 glassutil::CLogit::log(
	 glassutil::log_level::debug,
	 "CHypoList::evolve: Picks changed for sPid:" + pid
	 + " old picks:" + std::to_string(OriginalPicks)
	 + " new picks:" + std::to_string(hyp->getVPickSize()));
	 pushFifo(hyp);
	 }
	 */
	// the hypo survived, so return true
	return (true);
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

// ---------------------------------------------------------getFifoSize
int CHypoList::getFifoSize() {
	std::lock_guard<std::mutex> queueGuard(m_QueueMutex);

	// return the current size of the queue
	int size = qFifo.size();
	return (size);
}

const CGlass* CHypoList::getGlass() const {
	std::lock_guard<std::recursive_mutex> hypoListGuard(m_HypoListMutex);
	return (pGlass);
}

// ---------------------------------------------------------getHypos
std::vector<std::weak_ptr<CHypo>> CHypoList::getHypos(double t1, double t2) {
	std::vector<std::weak_ptr<CHypo>> hypos;

	if (t1 == t2) {
		return (hypos);
	}

	std::lock_guard<std::recursive_mutex> listGuard(m_vHypoMutex);

	// don't bother if the list is empty
	if (vHypo.size() == 0) {
		return (hypos);
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
		return (hypos);
	}

	// get the index of the ending time of the selection range
	int ix2 = indexHypo(t2);

	// check ending index
	if (ix2 < 0) {
		// end time is before the start of the list
		// no hypos to find
		return (hypos);
	}

	// for each hypo in the list within the
	// time range
	for (int it = ix1; it <= ix2; it++) {
		// get this hypo id
		std::string pid = vHypo[it].second;

		std::shared_ptr<CHypo> aHypo = mHypo[pid];

		if (aHypo != NULL) {
			std::weak_ptr<CHypo> awHypo = mHypo[pid];

			// add to the list of hypos
			hypos.push_back(awHypo);
		}
	}

	// return the list of hypos we found
	return (hypos);
}

// ---------------------------------------------------------getNHypoMax
int CHypoList::getNHypoMax() const {
	return (nHypoMax);
}

// ---------------------------------------------------------getNHypoTotal
int CHypoList::getNHypoTotal() const {
	return (nHypoTotal);
}

// ---------------------------------------------------------getVHypoSize
int CHypoList::getVHypoSize() const {
	std::lock_guard<std::recursive_mutex> vHypoGuard(m_vHypoMutex);
	return (vHypo.size());
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

// ---------------------------------------------------------jobSleep
bool CHypoList::mergeCloseEvents(std::shared_ptr<CHypo> hypo) {
	if (pGlass == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypoList::merge: NULL pGlass.");
		return (false);
	}

	char sLog[1024];  // logging string
	double distanceCut = 3.0;  // distance difference to try merging events
	// in degrees
	double timeCut = 30.;  // origin time difference to merge events
	double delta;  // this holds delta distance

	// this events pick list
	auto hVPick = hypo->getPickData();

	// set up a geo object for this hypo
	glassutil::CGeo geo;
	geo.setGeographic(hypo->getLatitude(), hypo->getLongitude(), EARTHRADIUSKM);

	// compute the list of hypos to try merging with with
	// (a potential hypo must be within time cut to consider)
	sort();
	std::vector<std::weak_ptr<CHypo>> hypoList = getHypos(
			hypo->getTOrigin() - timeCut, hypo->getTOrigin() + timeCut);

	// make sure we got hypos returned
	if (hypoList.size() == 0) {
		// print not events to merge message
		snprintf(sLog, sizeof(sLog),
					"CHypoList::merge: No events returned in ot time frame for "
					"merger of %s, Skipping",
					hypo->getID().c_str());
		glassutil::CLogit::log(sLog);
		return (false);
	}

	// only this hypo was returned
	if (hypoList.size() == 1) {
		// print not events to merge message
		std::shared_ptr<CHypo> thypo = hypoList[0].lock();
		snprintf(
				sLog, sizeof(sLog),
				"CHypoList::merge: Only event returned in ot time frame for "
				"merger of %s was %s, Skipping",
				hypo->getID().c_str(), thypo->getID().c_str());
		glassutil::CLogit::log(sLog);
		return (false);
	}

	// for each hypo in the list within the time range
	for (int i = 0; i < hypoList.size(); i++) {
		// make sure hypo is still valid before associating
		if (std::shared_ptr<CHypo> hypo2 = hypoList[i].lock()) {
			// make sure we're not looking at ourself
			if (hypo->getID() == hypo2->getID()) {
				continue;
			}

			// check to see if hypo2 is locked
			if (hypo2->isLockedForProcessing()) {
				continue;
			}

			std::lock_guard<std::mutex> hypoGuard(hypo2->getProcessingMutex());

			// check to make sure hypo2 is still good
			if (hypo2->cancelCheck() == true) {
				remHypo(hypo2);
				// hypo2->unlockAfterProcessing();
				continue;
			}

			// get hypo2's picks
			auto h2VPick = hypo2->getPickData();

			// check time difference
			double diff = std::fabs(hypo->getTOrigin() - hypo2->getTOrigin());

			if (diff < timeCut) {
				glassutil::CGeo geo2;
				geo2.setGeographic(hypo2->getLatitude(), hypo2->getLongitude(),
				EARTHRADIUSKM);

				// check distance between events
				delta = geo.delta(&geo2) / DEG2RAD;

				if (delta < distanceCut) {
					snprintf(
							sLog, sizeof(sLog),
							"CHypoList::merge: Testing merger of %s and %s\n",
							hypo->getID().c_str(), hypo2->getID().c_str());
					glassutil::CLogit::log(sLog);

					// Log info on two events
					snprintf(
							sLog, sizeof(sLog),
							"CHypoList::merge: %s: %.3f, %.3f, %.3f, %.3f\n",
							hypo->getID().c_str(), hypo->getLatitude(),
							hypo->getLongitude(), hypo->getDepth(),
							hypo->getTOrigin());
					glassutil::CLogit::log(sLog);

					snprintf(
							sLog, sizeof(sLog),
							"CHypoList::merge: %s: %.3f, %.3f, %.3f, %.3f\n",
							hypo2->getID().c_str(), hypo2->getLatitude(),
							hypo2->getLongitude(), hypo2->getDepth(),
							hypo2->getTOrigin());
					glassutil::CLogit::log(sLog);

					// create a new merged event hypo3
					std::shared_ptr<CHypo> hypo3 = std::make_shared<CHypo>(
							(hypo2->getLatitude() + hypo->getLatitude()) / 2.,
							(hypo2->getLongitude() + hypo->getLongitude()) / 2.,
							(hypo2->getDepth() + hypo->getDepth()) / 2.,
							(hypo2->getTOrigin() + hypo->getTOrigin()) / 2.,
							glassutil::CPid::pid(), "Merged Hypo", 0.0,
							hypo->getNucleationStackThreshold(),
							hypo->getNucleationDataThreshold(),
							hypo->getNucleationTravelTime1(),
							hypo->getNucleationTravelTime2(),
							pGlass->getAssociationTravelTimes(),
							hypo->getWebResolution(), hypo->getAzimuthTaper(),
							hypo->getMaxDepth());

					// set hypo glass pointer and such
					hypo3->setGlass(pGlass);
					hypo3->setDistanceCutoffFactor(
							pGlass->getDistanceCutoffFactor());
					hypo3->setDistanceCutoffPercentage(
							pGlass->getDistanceCutoffPercentage());
					hypo3->setMinDistanceCutoff(pGlass->getMinDistanceCutoff());

					// add all picks for other two events
					for (auto pick : hVPick) {
						hypo3->addPick(pick);
					}

					for (auto pick : h2VPick) {
						hypo3->addPick(pick);
					}

					// First localization attempt after nucleation
					hypo3->anneal(10000, (distanceCut / 2.) * DEG2KM,
									(distanceCut / 100.) * DEG2KM,
									(timeCut / 2.), .01);

					// Remove picks that do not fit hypo 3
					/*
					 if (pGlass->getPickList()->scavenge(hypo3)) {
					 // relocate the hypo
					 hypo3->localize();
					 }
					 */
					// Remove picks that do not fit hypo 3
					if (hypo3->prune()) {
						// relocate the hypo
						hypo3->localize();
					}

					int npick = hypo3->getPickDataSize();

					// check that the number of picks is sufficient to create new event
					if (hypo3->getBayesValue()
							> (std::max(hypo->getBayesValue(),
										hypo2->getBayesValue()))
									+ (.1
											* std::min(hypo->getBayesValue(),
														hypo2->getBayesValue()))) {
						snprintf(
								sLog,
								sizeof(sLog),
								"CHypoList::merge: keeping new event %s which"
								" scavanged %d picks (%lu, %lu picks in old events), "
								"New Bayes %.3f, old bayes"
								"%.3f and %.3f",
								hypo3->getID().c_str(), npick, (hVPick.size()),
								(h2VPick.size()), hypo3->getBayesValue(),
								hypo->getBayesValue(), hypo2->getBayesValue());

						glassutil::CLogit::log(sLog);

						remHypo(hypo);
						remHypo(hypo2);
						addHypo(hypo3);

						snprintf(sLog, sizeof(sLog),
									"CHypoList::merge: Removing %s\n",
									hypo->getID().c_str());
						glassutil::CLogit::Out(sLog);

						snprintf(sLog, sizeof(sLog),
									"CHypoList::merge: Removing %s\n",
									hypo2->getID().c_str());
						glassutil::CLogit::Out(sLog);

						return (true);
					} else {
						// otherwise do nothing (don't add the new event to the
						// hypo list, don't delete the unmerged hypos)
						snprintf(
								sLog, sizeof(sLog),
								"CHypoList::merge: -- canceling potential new"
								" event %s which associated %d picks of %d + %d"
								" potential picks",
								hypo3->getID().c_str(), npick,
								(static_cast<int>(hypo->getPickDataSize())),
								(static_cast<int>(hypo2->getPickDataSize())));
						glassutil::CLogit::log(sLog);
					}
				}
			}
			// hypo2->unlockAfterProcessing();
		}
	}
	return (false);
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
	// don't use a lock guard for queue mutex and vhypo mutex,
	// to avoid a deadlock when both mutexes are locked
	m_QueueMutex.lock();
	int size = qFifo.size();
	m_QueueMutex.unlock();

	// nullcheck
	if (hyp == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypoList::pushFifo: NULL hypo provided.");
		return (size);
	}

	// get this hypo's id
	std::string pid = hyp->getID();

	// use the map to get see if this hypo is even on the hypo list
	m_vHypoMutex.lock();
	if (mHypo[pid] == NULL) {
		// it's not, we can't really process a hypo we don't have
		m_vHypoMutex.unlock();

		return (size);
	}
	m_vHypoMutex.unlock();

	// is this id already on the queue?
	m_QueueMutex.lock();
	if (std::find(qFifo.begin(), qFifo.end(), pid) == qFifo.end()) {
		// it is not, add it
		qFifo.push_back(pid);
	}
	m_QueueMutex.unlock();

	glassutil::CLogit::log(
			glassutil::log_level::debug,
			"CHypoList::pushFifo: sPid:" + pid + " " + std::to_string(size)
					+ " hypos in queue.");

	return (size);
}

// ---------------------------------------------------------popFifo
std::shared_ptr<CHypo> CHypoList::popFifo() {
	// don't use a lock guard for queue mutex and hypomutex,
	// to avoid a deadlock when both mutexes are locked
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
	// Does not throw unless an exception is thrown by the assignment operator
	// of T.
	qFifo.erase(qFifo.begin());

	m_QueueMutex.unlock();

	m_vHypoMutex.lock();

	// use the map to get the hypo based on the id
	std::shared_ptr<CHypo> hyp = mHypo[pid];

	m_vHypoMutex.unlock();

	// return the hypo
	return (hyp);
}

// ---------------------------------------------------------remHypo
// Remove and unmap Hypocenter from vector, map, and pick
void CHypoList::remHypo(std::shared_ptr<CHypo> hypo, bool reportCancel) {
	// nullcheck
	if (hypo == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CHypoList::remHypo: NULL hypo provided.");

		return;
	}

	std::lock_guard<std::recursive_mutex> listGuard(m_vHypoMutex);

	// get the id
	std::string pid = hypo->getID();

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

			// Send cancellation message for this hypo
			if (reportCancel == true) {
				// only if we've sent an event message
				if (hypo->getEventSent()) {
					// create cancellation message
					hypo->cancel();
				}
			}
			// done
			break;
		}
	}

	// erase this hypo from the map
	mHypo.erase(pid);
}

// ---------------------------------------------------------ReqHypo
bool CHypoList::reqHypo(std::shared_ptr<json::Object> com) {
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

	std::lock_guard<std::recursive_mutex> listGuard(m_vHypoMutex);

	// get the hypo
	std::shared_ptr<CHypo> hyp = mHypo[sPid];

	// check the hypo
	if (!hyp) {
		glassutil::CLogit::log(
				glassutil::log_level::warn,
				"HypoList::reqHypo: Could not find hypo for pid " + sPid);

		// return true even if we didn't find anything, since
		// we've handled the message
		return (true);
	}

	// generate the hypo message
	hyp->hypo();

	// done
	return (true);
}

// ---------------------------------------------------------resolve
bool CHypoList::resolve(std::shared_ptr<CHypo> hyp) {
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

	// this lock guard exists to avoid a deadlock that occurs
	// when it isn't present
	std::lock_guard<std::recursive_mutex> listGuard(m_vHypoMutex);

	// return whether we've changed the pick set
	return (hyp->resolve(hyp));
}

// ---------------------------------------------------------setGlass
void CHypoList::setGlass(CGlass* glass) {
	std::lock_guard<std::recursive_mutex> hypoListGuard(m_HypoListMutex);
	pGlass = glass;
}

// ---------------------------------------------------------setNHypoMax
void CHypoList::setNHypoMax(int hypoMax) {
	std::lock_guard<std::recursive_mutex> hypoListGuard(m_HypoListMutex);
	nHypoMax = hypoMax;
}

// ---------------------------------------------------------sort
void CHypoList::sort() {
	std::lock_guard<std::recursive_mutex> listGuard(m_vHypoMutex);
	// sort hypos
	std::sort(vHypo.begin(), vHypo.end(), sortHypo);
}
}  // namespace glasscore

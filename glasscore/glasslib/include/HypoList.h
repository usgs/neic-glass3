/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef HYPOLIST_H
#define HYPOLIST_H

#include <json.h>
#include <threadbaseclass.h>
#include <vector>
#include <list>
#include <queue>
#include <map>
#include <set>
#include <memory>
#include <utility>
#include <string>
#include <mutex>
#include <thread>
#include <random>
#include "Glass.h"
#include "Hypo.h"

namespace glasscore {

// forward declarations
class CSite;
class CPick;
class CCorrelation;

/**
 * \brief CHypoList comparison function
 *
 * HypoCompare contains the comparison function used by std::multiset when
 * inserting, sorting, and retrieving hypos.
 */
struct HypoCompare {
	bool operator()(const std::shared_ptr<CHypo> &lhs,
					const std::shared_ptr<CHypo> &rhs) const {
		// sort by tSort
		if (lhs->getTSort() < rhs->getTSort()) {
			return (true);
		}
		return (false);
	}
};

/**
 * \brief glasscore hypocenter list class
 *
 * The CHypoList class is the class that maintains a std::map of all the
 * earthquake hypocenters being considered by glasscore.
 *
 * CHypoList also maintains a std::vector mapping the double hypo origin time
 * (in julian seconds) to the std::string hypo id
 *
 * CHypoList also maintains a std::vector of std::string ids of hypos to be
 * processed
 *
 * CHypoList contains functions to support hypocenter refinement, new data
 * association, and requesting output data.
 *
 * CHypoList uses smart pointers (std::shared_ptr).
 */
class CHypoList : public glass3::util::ThreadBaseClass {
 public:
	/**
	 * \brief CHypoList constructor
	 *
	 * The constructor for the CHypoList class.
	 * \param numThreads - An integer containing the number of
	 * threads in the pool.  Default 1
	 * \param sleepTime - An integer containing the amount of
	 * time to sleep in milliseconds between jobs.  Default 50
	 * \param checkInterval - An integer containing the amount of time in
	 * seconds between status checks. -1 to disable status checks.  Default 300.
	 */
	explicit CHypoList(int numThreads = 1, int sleepTime = 50,
						int checkInterval = 300);

	/**
	 * \brief CHypoList destructor
	 *
	 * The destructor for the CHypoList class.
	 */
	~CHypoList();

	/**
	 * \brief Add hypo to list
	 *
	 * Add the given hypocenter to the vector and map, if the new
	 * hypocenter causes the number of hypocenters in the vector/map
	 * to exceed the configured maximum, remove the oldest hypocenter
	 * from the list/map.
	 *
	 * Adds the hypocenter to the processing queue, and optionally schedules it
	 * for processing
	 *
	 * \param hypo - A std::shared_ptr to the hypocenter to add
	 * \param scheduleProcessing - A boolean flag indicating whether to
	 * automatically schedule processing when the hypo is added, defaults to
	 * true
	 * \return Returns true if the hypo was added, false otherwise.
	 */
	bool addHypo(std::shared_ptr<CHypo> hypo, bool scheduleProcessing = true);

	/**
	 * \brief Try to associate pick to a hypo in the list
	 *
	 * Attempt to associate the given pick to a hypocenter in the list
	 *
	 * Adds the hypocenter to the processing queue if a pick was associated.
	 *
	 * \param pk - A std::shared_ptr to the pick to associate.
	 * \return Returns true if the pick associated with a hypo, false otherwise
	 */
	bool associateData(std::shared_ptr<CPick> pk);

	/**
	 * \brief Try to associate correlation to a hypo in the list
	 *
	 * Attempt to associate the given correlation to a hypocenter in the list
	 *
	 * Adds the hypocenter to the processing queue if a correlation was
	 * associated.
	 *
	 * \param corr - A std::shared_ptr to the correlation to associate.
	 * \return  Returns true if the correlation associated with a hypo, false
	 * otherwise
	 */
	bool associateData(std::shared_ptr<CCorrelation> corr);

	/**
	 * \brief CHypoList clear function
	 */
	void clear() override;

	/**
	 * \brief CHypoList communication receiving function
	 *
	 * The function used by CHypoList to receive communication
	 * (such as configuration or input data), from outside the
	 * glasscore library, or it's parent CGlass.
	 *
	 * Supports the ReqHypo (generate output hypocenter message) input.
	 *
	 * \param com - A pointer to a json::object containing the
	 * communication.
	 * \return Returns true if the communication was handled by CGlass,
	 * false otherwise
	 */
	bool receiveExternalMessage(std::shared_ptr<json::Object> com);

	/**
	 * \brief Process provided hypocenter
	 *
	 * Process the provided hypocenter by localizing, scavenging, pruning,
	 * performing cancel checks, and output message generation. The hypocenter
	 * is put back on the processing queue if the hypo is changed and not
	 * canceled.
	 *
	 * \param hyp - A std::shared_ptr to the hypocenter to have its pick
	 * assocations resolved.
	 * \return Returns true if hypocenter survives
	 */
	bool processHypo(std::shared_ptr<CHypo> hyp);

	/**
	 * \brief Get the current length of the hypocenter processing queue
	 * \return Returns an integer value containing the current length of the
	 * processing queue
	 */
	int getHypoProcessingQueueLength();

	/**
	 * \brief Get list of CHypos in given time range
	 *
	 * Get a list of hypocenters in vHypo with origin time within given range
	 *
	 * \param t1 - Starting time of selection range
	 * \param t2 - Ending time of selection range
	 * \return A vector of std::weak_ptr to CHypos withing range,
	 * or empty list if none fit in the time range.
	 */
	std::vector<std::weak_ptr<CHypo>> getHypos(double t1, double t2);

	/**
	 * \brief Gets the maximum number of hypocenters this list will hold
	 * \return Returns an integer containing the maximum number of hypocenters
	 * this list will hold
	 */
	int getMaxAllowableHypoCount() const;

	/**
	 * \brief Get the total number of hypos processed by this list
	 * \return Return an integer containing the total number of hypos processed
	 * by this list
	 */
	int getCountOfTotalHyposProcessed() const;

	/**
	 * \brief Get the current number of hypos contained in this list
	 * \return Return an integer containing the current number of hypos
	 * contained in this list
	 */
	int length() const;

	/**
	 * \brief Merge hypos close in space time
	 *
	 * This function attempts to create a new hypo from picks of the given
	 * hypo and other hypos within a time/distance range. If a new hypo can be
	 * created, and the resultant stack value is high enough then the new merged
	 * hypo is added to the list, and the original pair of hypos are canceled /
	 * removed
	 *
	 * \param hyp - a shared_ptr to the CHypo to start the merge process with
	 * \return Returns true if hypos were merged, false otherwise
	 */
	bool findAndMergeMatchingHypos(std::shared_ptr<CHypo> hyp);

	/**
	 * \brief Append hypo to processing queue
	 *
	 * Append the given hypocenter to the processing queue if it is not already
	 * in the queue.
	 *
	 * \param hyp - A std::shared_ptr to the hypocenter to add
	 * \return Returns the current size of the processing queue
	 */
	int appendToHypoProcessingQueue(std::shared_ptr<CHypo> hyp);

	/**
	 * \brief Get first hypo from processing queue
	 *
	 * Get the first valid hypocenter from the processing queue.
	 *
	 * \return Returns a std::shared_ptr to the hypocenter retrieved
	 * from the queue.
	 */
	std::shared_ptr<CHypo> getNextHypoFromProcessingQueue();

	/**
	 * \brief Remove hypo from list
	 *
	 * Remove given hypocenter from the underlying multiset. Also, unlink any
	 * associated data, and optionally generate a cancelation message.
	 *
	 * \param hypo - A std::shared_ptr to the hypocenter to remove
	 * \param reportCancel A boolean flag indicating whether to report a cancel
	 * message when the hypo is removed
	 */
	void removeHypo(std::shared_ptr<CHypo> hypo, bool reportCancel = true);

	/**
	 * \brief Cause CHypoList to generate a Hypo message for a hypocenter
	 *
	 * Causes CHypoList to generate a json formatted hypocenter message
	 * for the id in the given ReqHypo message and send a pointer to this object
	 * to CGlass (and out of glasscore) by calling the hypo's Hypo() function
	 *
	 * \param com - A pointer to a json::object containing the id of the
	 * hypocenter to use
	 * \return Returns true if the Hypo message was generated, false otherwise
	 */
	bool requestHypo(std::shared_ptr<json::Object> com);

	/**
	 * \brief Ensure all supporting belong to hypo
	 *
	 * Search through all supporting data in the given hypocenter's lists,
	 * using the hypo affinity functions to determine whether the data best
	 * fits the hypocenter or not.
	 *
	 * Note that this function is in hypolist for threading deadlock reasons
	 *
	 * \param hypo - A shared_ptr to a CHypo to resolve
	 * \param allowStealing - A boolean flag indicating whether to allow
	 * resolveData to steal data, defaults to true
	 * \return Returns true if the hypocenter's pick list was changed,
	 * false otherwise.
	 */
	bool resolveData(std::shared_ptr<CHypo> hypo, bool allowStealing = true);

	/**
	 * \brief Set the maximum number of hypos that this list will support
	 * \param hypoMax - an integer containing the  maximum number of hypos that
	 * this list will support
	 */
	void setMaxAllowableHypoCount(int hypoMax);

	/**
	 * \brief Hypolist work function
	 *
	 * Process the next hypo on the processing queue.  Typically hypocenters are
	 * added to the processing queue because they are either new, or have
	 * been modified by another part of glasscore.
	 *
	 * A process count is set when a hypocenter is first scheduled (by addHypo()
	 * or associate()) for processing. Hypocenters are only processed a limited
	 * number of times, up to the overall glasscore process limit. Any
	 * additional after this requires that new supporting data be added to the
	 * hypo via associateData()
	 *
	 * \return returns glass3::util::WorkState::OK if work was successful,
	 * glass3::util::WorkState::Error if not.
	 */
	glass3::util::WorkState work() override;

 private:
	/**
	 * \brief A HypoList function that updates the position of the given hypo
	 * in the multiset
	 * \param hyp - A shared_ptr to the hypo that needs a position update
	 */
	void updatePosition(std::shared_ptr<CHypo> hyp);

	/**
	 * \brief A HypoList function that removes the given hypo from the multiset
	 * \param hyp - A shared_ptr to the hypo to be remvoed
	 */
	void eraseFromMultiset(std::shared_ptr<CHypo> hyp);

	/**
	 * \brief An integer containing the maximum number of hypocenters stored by
	 * CHypoList
	 */
	std::atomic<int> m_iMaxAllowableHypoCount;

	/**
	 * \brief An integer containing the total number of hypocenters
	 * ever added to CHypoList
	 */
	std::atomic<int> m_iCountOfTotalHyposProcessed;

	/**
	 * \brief A std::list of weak_ptrs to the hypocenters that need to be
	 * processed. This list is managed in a way to make it act as a FIFO
	 * queue
	 */
	std::list<std::weak_ptr<CHypo>> m_lHypoProcessingQueue;

	/**
	 * \brief the std::mutex for accessing m_vHyposToProcess
	 */
	std::mutex m_HypoProcessingQueueMutex;

	/**
	 * \brief A std::multiset containing each hypo in the list in sequential
	 * time order from oldest to youngest.
	 */
	std::multiset<std::shared_ptr<CHypo>, HypoCompare> m_msHypoList;

	/**
	 * \brief A std::map containing a std::shared_ptr to each hypocenter
	 * in CHypoList indexed by the std::string hypo id.
	 */
	std::map<std::string, std::shared_ptr<CHypo>> m_mHypo;

	/**
	 * \brief A recursive_mutex to control threading access to CHypoList.
	 * NOTE: recursive mutexes are frowned upon, so maybe redesign around it
	 * see: http://www.codingstandard.com/rule/18-3-3-do-not-use-stdrecursive_mutex/
	 * However a recursive_mutex allows us to maintain the original class
	 * design as delivered by the contractor.
	 */
	mutable std::recursive_mutex m_HypoListMutex;
};
}  // namespace glasscore
#endif  // HYPOLIST_H

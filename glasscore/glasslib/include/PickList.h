/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef PICKLIST_H
#define PICKLIST_H

#include <threadbaseclass.h>

#include <json.h>
#include <set>
#include <vector>
#include <memory>
#include <string>
#include <utility>
#include <mutex>
#include <thread>
#include <queue>
#include <random>
#include <atomic>

#include "Glass.h"
#include "Pick.h"

namespace glasscore {

// forward declarations
class CSite;
class CSiteList;
class CHypo;

/**
 * \brief CPickList comparison function
 *
 * PickCompare contains the comparison function used by std::multiset when
 * inserting, sorting, and retrieving picks.
 */
struct PickCompare {
	bool operator()(const std::shared_ptr<CPick> &lhs,
					const std::shared_ptr<CPick> &rhs) const {
		if (lhs->getTSort() < rhs->getTSort()) {
			return (true);
		}
		return (false);
	}
};

/**
 * \brief glasscore pick list class
 *
 * The CPickList class is the class that maintains a std::multiset of all the
 * waveform arrival picks being considered by glasscore.
 *
 *
 * CPickList contains functions to support pick parsing, scavenging, and nucleation.
 *
 * CPickList uses smart pointers (std::shared_ptr).
 */
class CPickList : public glass3::util::ThreadBaseClass {
 public:
	/**
	 * \brief CPickList constructor
	 *
	 * The constructor for the CPickList class.
	 * \param numThreads - An integer containing the number of
	 * threads in the pool.  Default 1
	 * \param sleepTime - An integer containing the amount of
	 * time to sleep in milliseconds between jobs.  Default 50
	 * \param checkInterval - An integer containing the amount of time in
	 * seconds between status checks. -1 to disable status checks.  Default 300.
	 */
	explicit CPickList(int numThreads = 1, int sleepTime = 50,
						int checkInterval = 300);

	/**
	 * \brief CPickList destructor
	 *
	 * The destructor for the CPickList class.
	 */
	~CPickList();

	/**
	 * \brief CPickList clear function
	 */
	void clear() override;

	/**
	 * \brief CPickList communication receiving function
	 *
	 * The function used by CPickList to receive communication
	 * (such as configuration or input data), from outside the
	 * glasscore library, or it's parent CGlass.
	 *
	 * Supports Pick (add pick data to list) input.
	 *
	 * \param com - A pointer to a json::object containing the
	 * communication.
	 * \return Returns true if the communication was handled by CPickList,
	 * false otherwise
	 */
	bool receiveExternalMessage(std::shared_ptr<json::Object> com);

	/**
	 * \brief CPickList add pick function
	 *
	 * The function used by CPickList to add a pick to the multiset, if the new
	 * pick causes the number of picks in the multiset to exceed the configured
	 * maximum, remove the oldest pick from the multiset, as well as try to
	 * remove it from the shorter list of picks in CSite.
	 *
	 * This function will generate a json formatted request for site
	 * (station) information if the pick is from an unknown site via the
	 * CSiteList getSite() function.
	 *
	 * This function will first attempt to associate the pick with
	 * an existing hypocenter via calling the CPickList::associate()
	 * function.  If association is unsuccessful, the pick is nucleated
	 * using the CPick::Nucleate() function.
	 *
	 * \param pick - A pointer to a json::object containing the
	 * pick.
	 * \return Returns true if the pick was usable and added by CPickList,
	 * false otherwise
	 */
	bool addPick(std::shared_ptr<json::Object> pick);

	/**
	 * \brief Checks if the provided pick time is a duplicate
	 *
	 * Compares the given pick time with the existing pick list times, in order
	 * to determine  whether the given pick is a duplicate of an existing pick.
	 *
	 * \param newTPick - A double containing the arrival time of the pick
	 * \param newSCNL - A std::string containing the scnl of the new pick
	 * \param tDuration - A double containing the allowable matching time window
	 * duration in seconds
	 * \return Returns a std::shared_ptr<CPick> to the first existing pick if
	 * there is a duplicate, NULL otherwise
	 */
	std::shared_ptr<CPick> getDuplicate(double newTPick, std::string newSCNL,
										double tDuration);

	/**
	 * \brief Search for any associable picks that match hypo
	 *
	 * Search through all picks within a provided number seconds from the origin
	 * time of the given hypocenter, adding any picks that meet association
	 * criteria to the given hypocenter.
	 *
	 * \param hyp - A shared_ptr to a CHypo object containing the hypocenter
	 * to attempt to associate to.
	 * \param tWindow - A double value containing the window to search picks
	 * from origin time in seconds, defaults to 2400.0
	 * \return Returns true if any picks were associated to the hypocenter,
	 * false otherwise.
	 */
	bool scavenge(std::shared_ptr<CHypo> hyp, double tWindow = 2400.0);

	/**
	 * \brief Get the CSiteList pointer used by this pick list for site lookups
	 * \return Return a pointer to the CSiteList class used by this pick list
	 */
	const CSiteList* getSiteList() const;

	/**
	 * \brief Set the CSiteList pointer used by this pick list for site lookups
	 * \param siteList - a pointer to the CSiteList class used by this pick list
	 */
	void setSiteList(CSiteList* siteList);

	/**
	 * \brief Get the maximum allowed size of this pick list
	 * \return Return an integer containing the maximum allowed size of this
	 * pick list
	 */
	int getMaxAllowablePickCount() const;

	/**
	 * \brief Set the maximum allowed size of this pick list
	 * \param pickMax -  an integer containing the maximum allowed size of this
	 * pick list
	 */
	void setMaxAllowablePickCount(int pickMax);

	/**
	 * \brief Get the total number of picks processed by this list
	 * \return Return an integer containing the total number of picks processed
	 * by this list
	 */
	int getCountOfTotalPicksProcessed() const;

	/**
	 * \brief Get the current number of picks contained in this list
	 * \return Return an integer containing the current number of picks
	 * contained in this list
	 */
	int length() const;

	/**
	 * \brief Get a vector of picks that fall within a time window
	 *
	 * Get a vector of picks that fall within the provided time window from t1
	 * to t2
	 *
	 * \param t1 - A double value containing the beginning of the time window in
	 * julian seconds
	 * \param t2 - A double value containing the end of the time window in
	 * julian seconds
	 * \return Return a std::vector of std::weak_ptrs to the picks within the
	 * time window
	 */
	std::vector<std::weak_ptr<CPick>> getPicks(double t1, double t2);

	/**
	 * \brief PickList work function
	 *
	 * Attempts to associate and nuclate the next pick on the queue.
	 * \return returns glass3::util::WorkState::OK if work was successful,
	 * glass3::util::WorkState::Error if not.
	 */
	glass3::util::WorkState work() override;

 private:
	/**
	 * \brief A PickList function that updates the position of the given pick
	 * in the multiset
	 * \param pick - A shared_ptr to the pick that needs a position update
	 */
	void updatePosition(std::shared_ptr<CPick> pick);

	/**
	 * \brief A PickList function that removes the given pick from the multiset
	 * \param pick - A shared_ptr to the pick to be removed
	 */
	void eraseFromMultiset(std::shared_ptr<CPick> pick);

	/**
	 * \brief A pointer to a CSiteList object containing all the sites for
	 * lookups
	 */
	CSiteList * m_pSiteList;

	/**
	 * \brief An integer containing the maximum number of picks allowed in
	 * CPickList. This value is overridden by pGlass->nPickMax if provided.
	 * Defaults to 10000.
	 */
	int m_iMaxAllowablePickCount;

	/**
	 * \brief An integer containing the total number of picks ever added to
	 * CPickList
	 */
	int m_iCountOfTotalPicksProcessed;

	/**
	 * \brief A std::multiset containing each pick in the list in sequential
	 * time order from oldest to youngest.
	 */
	std::multiset<std::shared_ptr<CPick>, PickCompare> m_msPickList;

	/**
	 * \brief A std::queue containing a std::shared_ptr to each pick in that
	 * needs to be processed
	 */
	std::queue<std::shared_ptr<CPick>> m_qPicksToProcess;

	/**
	 * \brief the std::mutex for m_qPicksToProcess
	 */
	std::mutex m_PicksToProcessMutex;

	/**
	 * \brief A recursive_mutex to control threading access to CPickList.
	 * NOTE: recursive mutexes are frowned upon, so maybe redesign around it
	 * see: http://www.codingstandard.com/rule/18-3-3-do-not-use-stdrecursive_mutex/
	 * However a recursive_mutex allows us to maintain the original class
	 * design as delivered by the contractor.
	 */
	mutable std::recursive_mutex m_PickListMutex;

	/**
	 * \brief A shared_ptr to a pick used to represent the lower value in
	 * getPicks() calls
	 */
	std::shared_ptr<CPick> m_LowerValue;

	/**
	 * \brief A shared_ptr to a pick used to represent the upper value in
	 * getPicks() calls
	 */
	std::shared_ptr<CPick> m_UpperValue;
};
}  // namespace glasscore
#endif  // PICKLIST_H

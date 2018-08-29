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
class CGlass;
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
    	if (lhs->getTPick() < rhs->getTPick()) {
    		return(true);
    	}
        return (false);
    }
};

/**
 * \brief glasscore pick list class
 *
 * The CPickList class is the class that maintains a std::map of all the
 * waveform arrival picks being considered by glasscore.
 *
 * CPickList also maintains a std::vector mapping the double pick arrival time
 * (in julian seconds) to the std::string pick id
 *
 * CPickList contains functions to support pick scavenging, resoultion, rogue
 * tracking, and new data input.
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
	 * \brief Remove all picks from pick list
	 *
	 * Clears all picks from the vector and map
	 */
	void clearPicks();

	/**
	 * \brief CPickList communication receiving function
	 *
	 * The function used by CPickList to receive communication
	 * (such as configuration or input data), from outside the
	 * glasscore library, or it's parent CGlass.
	 *
	 * Supports Pick (add pick data to list) and ClearGlass
	 * (clear all pick data) inputs.
	 *
	 * \param com - A pointer to a json::object containing the
	 * communication.
	 * \return Returns true if the communication was handled by CPickList,
	 * false otherwise
	 */
	bool dispatch(std::shared_ptr<json::Object> com);

	/**
	 * \brief CPickList add pick function
	 *
	 * The function used by CPickList to add a pick to the vector
	 * and map, if the new pick causes the number of picks in the vector/map
	 * to exceed the configured maximum, remove the oldest pick
	 * from the list/map, as well as the list of picks in CSite.
	 *
	 * This function will generate a json formatted request for site
	 * (station) information if the pick is from an unknown site.
	 *
	 * This function will first attempt to associate the pick with
	 * an existing hypocenter via calling the CHypoList::associate()
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
	 * \brief Checks if picks is duplicate
	 *
	 * Takes a new pick and compares with list of picks.
	 * True if pick is a duplicate
	 */
	bool checkDuplicate(double newTPick, std::string newSCNL, double window);

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
	 * \brief CGlass getter
	 * \return the CGlass pointer
	 */
	const CGlass* getGlass() const;

	/**
	 * \brief CGlass setter
	 * \param glass - the CGlass pointer
	 */
	void setGlass(CGlass* glass);

	/**
	 * \brief CSiteList getter
	 * \return the CSiteList pointer
	 */
	const CSiteList* getSiteList() const;

	/**
	 * \brief CSiteList setter
	 * \param siteList - the CSiteList pointer
	 */
	void setSiteList(CSiteList* siteList);

	/**
	 * \brief nPickMax getter
	 * \return the nPickMax
	 */
	int getPickMax() const;

	/**
	 * \brief nPickMax Setter
	 * \param picknMax - the nPickMax
	 */
	void setPickMax(int picknMax);

	/**
	 * \brief nPickTotal getter
	 * \return the nPickTotal
	 */
	int getPickTotal() const;

	/**
	 * \brief Get the current size of the pick list
	 */
	int size() const;

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
	 * \brief A pointer to the parent CGlass class, used to look up site
	 * information, configuration values, call association functions, and debug
	 * flags
	 */
	CGlass * m_pGlass;

	/**
	 * \brief A pointer to a CSiteList object containing all the sites for
	 * lookups
	 */
	CSiteList * m_pSiteList;

	/**
	 * \brief An integer containing the total number of picks ever added to
	 * CPickList
	 */
	int m_iPickTotal;

	/**
	 * \brief An integer containing the maximum number of picks allowed in
	 * CPickList. This value is overridden by pGlass->nPickMax if provided.
	 * Defaults to 10000.
	 */
	int m_iPickMax;

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
	 * \brief the std::mutex for qProcessList
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
};
}  // namespace glasscore
#endif  // PICKLIST_H

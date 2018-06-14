/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef HYPOLIST_H
#define HYPOLIST_H

#include <json.h>
#include <vector>
#include <queue>
#include <map>
#include <memory>
#include <utility>
#include <string>
#include <mutex>
#include <thread>
#include <random>
#include "Glass.h"

namespace glasscore {

// forward declarations
class CGlass;
class CSite;
class CHypo;
class CPick;
class CCorrelation;

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
class CHypoList {
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
	 * Adds the hypocenter to the processing queue, and calls darwin()
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
	 * Adds the hypocenter to the processing queue if a pick was associated,
	 * but does not call darwin to perform processing.
	 *
	 * \param pk - A std::shared_ptr to the pick to associate.
	 * \return Always returns true.
	 */
	bool associate(std::shared_ptr<CPick> pk);

	/**
	 * \brief Try to associate correlation to a hypo in the list
	 *
	 * Attempt to associate the given correlation to a hypocenter in the list
	 *
	 * Adds the hypocenter to the processing queue if a correlation was associated,
	 * but does not call darwin to perform processing.
	 *
	 * \param corr - A std::shared_ptr to the correlation to associate.
	 * \return Always returns true.
	 */
	bool associate(std::shared_ptr<CCorrelation> corr);

	/**
	 * \brief CHypoList clear function
	 */
	void clear();

	/**
	 * \brief Remove all hypos from hypo list
	 *
	 * Clears all hypo from the vector, map, and queue
	 */
	void clearHypos();

	/**
	 * \brief Process all hypocenters in the queue
	 *
	 * Refine any/all hypocenters in the list that are on the
	 * processing queue.  Typically hypocenters are added to the
	 * processing queue because they are either new, or have
	 * been modified by another part of glasscore.  This function effectively
	 * calls itself recursively as a result of calling CHypoList::evolve or
	 * CPickList::resolve as part of the resolution of data associations which
	 * adds hypocenters to the processing queue.
	 *
	 * A cycle count is set when a hypocenter is first scheduled (by addHypo()
	 * or associate()). Hypocenters are only processed through the entire
	 * cycle once each cycle, any additional processing is restricted
	 * to localization and culling.
	 *
	 * Note that glasscore will not accept input data until all hypocenters
	 * in the list have been refined, including the recursively scheduled calls
	 */
	void darwin();

	/**
	 * \brief CHypoList communication receiving function
	 *
	 * The function used by CHypoList to receive communication
	 * (such as configuration or input data), from outside the
	 * glasscore library, or it's parent CGlass.
	 *
	 * Supports the Zombee (testing hypocenters), ClearGlass
	 * (clear all hypocenter data), and ReqHypo (generate output hypocenter
	 * message) inputs.
	 *
	 * \param com - A pointer to a json::object containing the
	 * communication.
	 * \return Returns true if the communication was handled by CGlass,
	 * false otherwise
	 */
	bool dispatch(std::shared_ptr<json::Object> com);

	/**
	 * \brief Evolve provided hypocenter
	 *
	 * Evolve the provided hypocenter by localizing, scavenging, pruning,
	 * performing cancel checks, and output message generation. The hypocenter
	 * is put back on the processing queue if the hypo is changed and not
	 * canceled.
	 *
	 * \param hyp - A std::shared_ptr to the hypocenter to have its pick
	 * assocations resolved.
	 * \return Returns true if hypocenter survives
	 */
	bool evolve(std::shared_ptr<CHypo> hyp);

	/**
	 * \brief Find CHypo in given time range
	 *
	 * Use a binary search to find a hypocenter in
	 * vHypo with origin time within given range
	 *
	 * \param t1 - Starting time of selection range in gregorian seconds
	 * \param t2 - Ending time of selection range in gregorian seconds
	 * \return First CHypo in vHypo withing range,
	 * or NULL if none fit in the time range.
	 */
	std::shared_ptr<CHypo> findHypo(double t1, double t2);

	/**
	 * \brief Get the current size of the hypocenter processing queue
	 */
	int getFifoSize();

	/**
	 * \brief CGlass getter
	 * \return the CGlass pointer
	 */
	const CGlass* getGlass() const;

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
	 * \brief nHypo getter
	 * \return the nHypo
	 */
	int getNHypo() const;

	/**
	 * \brief nHypoMax getter
	 * \return the nHypoMax
	 */
	int getNHypoMax() const;

	/**
	 * \brief nHypoTotal getter
	 * \return the nHypoTotal
	 */
	int getNHypoTotal() const;

	/**
	 * \brief Get the current size of the hypocenter list
	 */
	int getVHypoSize() const;

	/**
	 * \brief Get insertion index for hypo
	 *
	 * This function looks up the proper insertion index for the vector given an
	 * origin time using a binary search to identify the index element
	 * is less than the time provided, and the next element is greater.
	 *
	 * \param tOrg - A double value containing the origin time to use, in
	 * julian seconds of the hypo to add.
	 * \return Returns the insertion index, if the insertion is before
	 * the beginning, -1 is returned, if insertion is after the last element,
	 * the index of the last element is returned, if the vector is empty,
	 * -2 is returned.
	 */
	int indexHypo(double tOrg);

	/**
	 * \brief Print basic values to screen for hypocenter list
	 *
	 * Causes CHypoList to print basic values to the console for
	 * each hypocenter in the list
	 */
	void listHypos();

	/** \brief Try to merge events close in space time
	 *
	 * 	Tries to created a new event from picks of two nearby events
	 * 	If it can merge, and the resultant stack value is high enough
	 * 	then it creates a new event and cancels the two merged events
	 *
	 */
	bool mergeCloseEvents(std::shared_ptr<CHypo> hyp);

	/**
	 * \brief Add hypo to processing queue
	 *
	 * Add the given hypocenter to the processing queue if it
	 * is not already in the queue.
	 *
	 * \param hyp - A std::shared_ptr to the hypocenter to add
	 * \return Returns the current size of the processing queue
	 */
	int pushFifo(std::shared_ptr<CHypo> hyp);

	/**
	 * \brief Get hypo from processing queue
	 *
	 * Get the first hypocenter from the processing queue.
	 *
	 * \return Returns a std::shared_ptr to the hypocenter retrieved
	 * from the queue.
	 */
	std::shared_ptr<CHypo> popFifo();

	/**
	 * \brief Remove hypo from list
	 *
	 * Remove given hypocenter to the vector and map. Also, unlink
	 * any associated picks.
	 *
	 * \param hypo - A std::shared_ptr to the hypocenter to remove
	 * \param reportCancel A boolean flag indicating whether to report a
	 * cancel message when the hypo is removed
	 * \return void.
	 */
	void remHypo(std::shared_ptr<CHypo> hypo, bool reportCancel = true);

	/**
	 * \brief Cause CHypoList to generate a Hypo message for a hypocenter
	 *
	 * Causes CHypoList to generate a json formatted hypocenter message
	 * for the id in the given ReqHypo message and send a pointer to this object
	 * to CGlass (and out of glasscore) by calling the hypo's Hypo() function
	 *
	 * \param com - A pointer to a json::object containing the id of the
	 * hypocenter to use
	 */
	bool reqHypo(std::shared_ptr<json::Object> com);

	/**
	 * \brief Ensure all picks in the hypo belong to hypo
	 *
	 * Search through all picks in the given hypocenter's pick list, using the
	 * hypo's affinity function to determine whether the pick belongs to the
	 * given hypocenter or not.
	 *
	 * \return Returns true if the hypocenter's pick list was changed,
	 * false otherwise.
	 */
	bool resolve(std::shared_ptr<CHypo> hyp);

	/**
	 * \brief CGlass setter
	 * \param glass - the CGlass pointer
	 */
	void setGlass(CGlass* glass);

	/**
	 * \brief nHypoMax Setter
	 * \param hypoMax - the nHypoMax
	 */
	void setNHypoMax(int hypoMax);

	/**
	 * \brief check to see if each thread is still functional
	 *
	 * Checks each thread to see if it is still responsive.
	 */
	bool statusCheck();

 private:
	/**
	 * \brief the job sleep
	 *
	 * The function that performs the sleep between jobs
	 */
	void jobSleep();

	/**
	 * \brief Process the next pick on the queue
	 *
	 * Attempts to run evolve for every pending hypo.
	 */
	void processHypos();

	/**
	 * \brief thread status update function
	 *
	 * Updates the status for the current thread
	 * \param status - A boolean flag containing the status to set
	 */
	void setStatus(bool status);

	/**
	 * \brief HypoList sort function
	 */
	void sort();

	/**
	 * \brief A pointer to the parent CGlass class, used to send output,
	 * encode/decode time, get configuration values, and debug flags
	 */
	CGlass *pGlass;

	/**
	 * \brief An integer containing the total number of hypocenters
	 * ever added to CHypoList
	 */
	int nHypoTotal;

	/**
	 * \brief An integer containing the maximum number of hypocenters stored by
	 * CHypoList
	 */
	int nHypoMax;

	/**
	 * \brief Also an integer containing the total number of hypocenters
	 * ever added to CHypoList, but one larger
	 */
	int nHypo;

	/**
	 * \brief A std::vector containing the queue of hypocenters that need
	 * to be processed
	 */
	std::vector<std::string> qFifo;

	/**
	 * \brief the std::mutex for qFifo
	 */
	std::mutex m_QueueMutex;

	/**
	 * \brief A std::vector mapping the origin time of each hypocenter
	 * in CHypoList to it's std::string hypo id.
	 *
	 * Note that the origin time is never updated after the hypocenter is
	 * first added to CHypoList
	 */
	std::vector<std::pair<double, std::string>> vHypo;

	/**
	 * \brief A std::map containing a std::shared_ptr to each hypocenter
	 * in CHypoList indexed by the std::string hypo id.
	 */
	std::map<std::string, std::shared_ptr<CHypo>> mHypo;

	/**
	 * \brief A recursive_mutex to control threading access to vHypo.
	 * NOTE: recursive mutexes are frowned upon, so maybe redesign around it
	 * see: http://www.codingstandard.com/rule/18-3-3-do-not-use-stdrecursive_mutex/
	 * However a recursive_mutex allows us to maintain the original class
	 * design as delivered by the contractor.
	 */
	mutable std::recursive_mutex m_vHypoMutex;

	/**
	 * \brief A recursive_mutex to control threading access to CHypoList.
	 * NOTE: recursive mutexes are frowned upon, so maybe redesign around it
	 * see: http://www.codingstandard.com/rule/18-3-3-do-not-use-stdrecursive_mutex/
	 * However a recursive_mutex allows us to maintain the original class
	 * design as delivered by the contractor.
	 */
	mutable std::recursive_mutex m_HypoListMutex;

	/**
	 * \brief the std::vector of std::threads
	 */
	std::vector<std::thread> vProcessThreads;

	/**
	 * \brief An integer containing the number of
	 * threads in the pool.
	 */
	int m_iNumThreads;

	/**
	 * \brief A std::map containing the status of each thread
	 */
	std::map<std::thread::id, bool> m_ThreadStatusMap;

	/**
	 * \brief An integer containing the amount of
	 * time to sleep in milliseconds between picks.
	 */
	int m_iSleepTimeMS;

	/**
	 * \brief the std::mutex for m_ThreadStatusMap
	 */
	std::mutex m_StatusMutex;

	/**
	 * \brief the integer interval in seconds after which the work thread
	 * will be considered dead. A negative check interval disables thread
	 * status checks
	 */
	int m_iStatusCheckInterval;

	/**
	 * \brief the time_t holding the last time the thread status was checked
	 */
	time_t tLastStatusCheck;

	/**
	 * \brief the boolean flags indicating that the process threads
	 * should keep running.
	 */
	bool m_bRunProcessLoop;

	/**
	 * \brief A random engine used to generate random numbers
	 */
	std::default_random_engine m_RandomGenerator;
};
}  // namespace glasscore
#endif  // HYPOLIST_H

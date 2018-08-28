/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef HYPOLIST_H
#define HYPOLIST_H

#include <threadbaseclass.h>

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
	void clear() override;

	/**
	 * \brief Remove all hypos from hypo list
	 *
	 * Clears all hypo from the vector, map, and queue
	 */
	void clearHypos();

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
	 * \brief Get the current size of the hypocenter processing queue
	 */
	int getHyposToProcessSize();

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
	 * \brief nHypoMax getter
	 * \return the nHypoMax
	 */
	int getHypoMax() const;

	/**
	 * \brief nHypoTotal getter
	 * \return the nHypoTotal
	 */
	int getHypoTotal() const;

	/**
	 * \brief Get the current size of the hypocenter list
	 */
	int size() const;

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
	int addHypoToProcess(std::shared_ptr<CHypo> hyp);

	/**
	 * \brief Get hypo from processing queue
	 *
	 * Get the first hypocenter from the processing queue.
	 *
	 * \return Returns a std::shared_ptr to the hypocenter retrieved
	 * from the queue.
	 */
	std::shared_ptr<CHypo> getHypoToProcess();

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
	 */
	bool requestHypo(std::shared_ptr<json::Object> com);

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
	void setHypoMax(int hypoMax);

	/**
	 * \brief Hypolist work function
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
	 * \return returns glass3::util::WorkState::OK if work was successful,
	 * glass3::util::WorkState::Error if not.
	 */
	glass3::util::WorkState work() override;

 private:
	/**
	 * \brief HypoList sort function
	 */
	void sort();

	/**
	 * \brief A pointer to the parent CGlass class, used to send output,
	 * encode/decode time, get configuration values, and debug flags
	 */
	CGlass * m_pGlass;

	/**
	 * \brief An integer containing the total number of hypocenters
	 * ever added to CHypoList
	 */
	std::atomic<int> m_iHypoTotal;

	/**
	 * \brief An integer containing the maximum number of hypocenters stored by
	 * CHypoList
	 */
	std::atomic<int> m_iHypoMax;

	/**
	 * \brief A std::vector containing the queue of hypocenters that need
	 * to be processed
	 */
	std::vector<std::string> m_vHyposToProcess;

	/**
	 * \brief the std::mutex for m_vHyposToProcess
	 */
	std::mutex m_vHyposToProcessMutex;

	/**
	 * \brief A std::vector mapping the origin time of each hypocenter
	 * in CHypoList to it's std::string hypo id.
	 *
	 * Note that the origin time is never updated after the hypocenter is
	 * first added to CHypoList
	 */
	std::vector<std::pair<double, std::string>> m_vHypo;

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

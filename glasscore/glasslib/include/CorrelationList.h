/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef CORRELATIONLIST_H
#define CORRELATIONLIST_H

#include <json.h>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <utility>
#include <atomic>

namespace glasscore {

// forward declarations
class CSite;
class CSiteList;
class CCorrelation;
class CHypo;
class CGlass;

/**
 * \brief glasscore correlation list class
 *
 * The CCorrelationList class is the class that encapsulates everything
 * necessary to represent a waveform arrival correlation, including arrival
 * time, phase id,and an unique identifier.  The CCorrelationList class is also
 * a node in the detection graph database.
 *
 * CCorrelationList contains functions to support nucleation of a new event
 * basedon the correlation.
 *
 * CCorrelationList maintains a graph database link between it and the the site
 * (station)the correlation was made at.
 *
 * CCorrelationList also maintains a vector of CHypo objects represent the graph
 * database links between  this correlation and various hypocenters.  A single
 * correlation may belinked to multiple hypocenters
 *
 * CCorrelationList uses smart pointers (std::shared_ptr).
 */
class CCorrelationList {
 public:
	/**
	 * \brief CCorrelationList constructor
	 */
	CCorrelationList();

	/**
	 * \brief CCorrelationList destructor
	 */
	~CCorrelationList();

	/**
	 * \brief CCorrelationList clear function
	 */
	void clear();

	/**
	 * \brief Remove all correlations from correlation list
	 *
	 * Clears all correlations from the vector and map
	 */
	void clearCorrelations();

	/**
	 * \brief CCorrelationList communication receiving function
	 *
	 * The function used by CCorrelationList to receive communication
	 * (such as configuration or input data), from outside the
	 * glasscore library, or it's parent CGlass.
	 *
	 * Supports processing Correlation messages
	 *
	 * \param com - A pointer to a json::object containing the
	 * communication.
	 * \return Returns true if the communication was handled by CCorrelationList,
	 * false otherwise
	 */
	bool dispatch(std::shared_ptr<json::Object> com);

	/**
	 * \brief CCorrelationList add correlation function
	 *
	 * The function used by CCorrelationList to add a correlation to the vector
	 * and map, if the new correlation causes the number of correlations in the
	 * vector/map to exceed the configured maximum, remove the oldest
	 * correlation from the list/map, as well as the list of correlations in
	 * CSite.
	 *
	 * This function will generate a json formatted request for site
	 * (station) information if the correlation is from an unknown site.
	 *
	 * This function will first attempt to associate the correlation with
	 * an existing hypocenter via calling the CHypoList::associate()
	 * function.  If association is unsuccessful, the correlation creates a new
	 * hypocenter
	 *
	 * \param com - A pointer to a json::object containing the correlation.
	 * \return Returns true if the correlation was usable and added by
	 * CCorrelationList, false otherwise
	 */
	bool addCorrelationFromJSON(std::shared_ptr<json::Object> com);

	/**
	 * \brief CCorrelationList get correlation function
	 *
	 * Given the integer id of a correlation get a shared_ptr to that correlation.
	 *
	 * \param idCorrelation - An integer value containing the id of the
	 * correlation to get
	 * \return Returns a shared_ptr to the found CCorrelation, or null if no
	 * correlation found.
	 */
	std::shared_ptr<CCorrelation> getCorrelation(int idCorrelation);

	/**
	 * \brief Get insertion index for correlation
	 *
	 * This function looks up the proper insertion index for the vector given an
	 * arrival time using a binary search to identify the index element
	 * is less than the time provided, and the next element is greater.
	 *
	 * \param tCorrelation - A double value containing the arrival time to use, in
	 * julian seconds of the correlation to add.
	 * \return Returns the insertion index, if the insertion is before
	 * the beginning, -1 is returned, if insertion is after the last element,
	 * the id of the last element is returned, if the vector is empty,
	 * -2 is returned.
	 */
	int getInsertionIndex(double tCorrelation);

	/**
	 * \brief Checks if correlation is duplicate
	 *
	 * Takes a new correlation and compares with list of correlations.
	 *
	 * \param newCorrelation - A shared pointer to the correlation to check
	 * \param tWindow - A double containing the allowable matching time window
	 * in seconds
	 * \param xWindow - A double containing the allowable matching distance
	 * window in degrees
	 * returns true if correlation is a duplicate, false otherwise
	 */
	bool checkDuplicate(CCorrelation * newCorrelation, double tWindow,
						double xWindow);

	/**
	 * \brief Search for any associable correlations that match hypo
	 *
	 * Search through all correlations within a provided number seconds from the
	 * origin time of the given hypocenter, adding any correlations that meet
	 * association criteria to the given hypocenter.
	 *
	 * \param hyp - A shared_ptr to a CHypo object containing the hypocenter
	 * to attempt to associate to.
	 * \param tDuration - A double value containing the duration to search picks
	 * from origin time in seconds, defaults to 2.5
	 * \return Returns true if any picks were associated to the hypocenter,
	 * false otherwise.
	 */
	bool scavenge(std::shared_ptr<CHypo> hyp, double tDuration = 2.5);

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
	 * \brief nCorrelationMax getter
	 * \return the nCorrelationMax
	 */
	int getCorrelationMax() const;

	/**
	 * \brief nCorrelationMax Setter
	 * \param correlationMax - the nCorrelationMax
	 */
	void setCorrelationMax(int correlationMax);

	/**
	 * \brief nCorrelationTotal getter
	 * \return the nCorrelationTotal
	 */
	int getCorrelationTotal() const;

	/**
	 * \brief Get the current size of the correlation list
	 */
	int size() const;

 private:
	/**
	 * \brief A pointer to the parent CGlass class, used to send output,
	 * look up site information, encode/decode time, get configuration values,
	 * call association functions, and debug flags
	 */
	CGlass * m_pGlass;

	/**
	 * \brief A pointer to a CSiteList object containing all the sites for
	 * lookups
	 */
	CSiteList * m_pSiteList;

	/**
	 * \brief An integer containing the maximum number of correlations allowed in
	 * CCorrelationList. This value is overridden by pGlass->nPickMax if provided.
	 * Defaults to 10000.
	 */
	std::atomic<int> m_iCorrelationMax;

	/**
	 * \brief An integer containing the total number of correlations ever added
	 * to CCorrelationList
	 */
	std::atomic<int> m_iCorrelationTotal;

	/**
	 * \brief A std::vector mapping the arrival time of each correlation in
	 * CCorrelationListto it's integer correlation id. The elements in this
	 * vector object are inserted in a manner to keep it in sequential time
	 * order from oldest to youngest.
	 */
	std::vector<std::pair<double, int>> m_vCorrelation;

	/**
	 * \brief A std::map containing a std::shared_ptr to each correlation in
	 * CCorrelationList indexed by the integer correlation id.
	 */
	std::map<int, std::shared_ptr<CCorrelation>> m_mCorrelation;

	/**
	 * \brief A recursive_mutex to control threading access to vCorrelation.
	 * NOTE: recursive mutexes are frowned upon, so maybe redesign around it
	 * see: http://www.codingstandard.com/rule/18-3-3-do-not-use-stdrecursive_mutex/
	 * However a recursive_mutex allows us to maintain the original class
	 * design as delivered by the contractor.
	 */
	mutable std::recursive_mutex m_vCorrelationMutex;

	/**
	 * \brief A recursive_mutex to control threading access to CCorrelationList.
	 * NOTE: recursive mutexes are frowned upon, so maybe redesign around it
	 * see: http://www.codingstandard.com/rule/18-3-3-do-not-use-stdrecursive_mutex/
	 * However a recursive_mutex allows us to maintain the original class
	 * design as delivered by the contractor.
	 */
	mutable std::recursive_mutex m_CorrelationListMutex;
};
}   // namespace glasscore
#endif  // CORRELATIONLIST_H

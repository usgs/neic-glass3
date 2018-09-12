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
#include <set>
#include <vector>
#include <utility>
#include <atomic>
#include "Correlation.h"

namespace glasscore {

// forward declarations
class CSite;
class CSiteList;
class CHypo;
class CGlass;

/**
 * \brief CCorrelationList comparison function
 *
 * CorrelationCompare contains the comparison function used by std::multiset when
 * inserting, sorting, and retrieving correlations.
 */
struct CorrelationCompare {
	bool operator()(const std::shared_ptr<CCorrelation> &lhs,
					const std::shared_ptr<CCorrelation> &rhs) const {
		if (lhs->getTCorrelation() < rhs->getTCorrelation()) {
			return (true);
		}
		return (false);
	}
};

/**
 * \brief glasscore correlation list class
 *
 * The CCorrelationList class is the class that maintains a std::multiset of all
 * the correlations being considered by glasscore.
 *
 * CCorrelationList contains functions to support correlation parsing,
 * scavenging, and the creation of a new event hypocenter based on the correlation.
 *
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
	 * The function used by CCorrelationList to add a correlation to multiset,
	 * if the new correlation causes the number of correlations in the
	 * multiset to exceed the configured maximum, remove the oldest
	 * correlation from the multiset.
	 *
	 * This function will generate a json formatted request for site
	 * (station) information if the correlation is from an unknown site via the
	 * CSiteList getSite() function.
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
	 * \brief Checks if the provided correlation is sduplicate
	 *
	 * Compares the given correlation with the existing correlation list, in
	 * order to determine whether the given correlation is a duplicate of an
	 * existing correlation.
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
	 * \param tWindow - A double value containing the window to search picks
	 * from origin time in seconds, defaults to 2.5
	 * \return Returns true if any picks were associated to the hypocenter,
	 * false otherwise.
	 */
	bool scavenge(std::shared_ptr<CHypo> hyp, double tWindow = 2.5);

	/**
	 * \brief Get the CSiteList pointer used by this correlation list for site
	 * lookups
	 * \return Return a pointer to the CSiteList class used by this correlation
	 * list
	 */
	const CSiteList* getSiteList() const;

	/**
	 * \brief Set the CSiteList pointer used by this correlation list for site
	 * lookups
	 * \param siteList - a pointer to the CSiteList class used by this correlation
	 * list
	 */
	void setSiteList(CSiteList* siteList);

	/**
	 * \brief Get the maximum allowed size of this correlation list
	 * \return Return an integer containing the maximum allowed size of this
	 * correlation list
	 */
	int getCorrelationMax() const;

	/**
	 * \brief Set the maximum allowed size of this correlation list
	 * \param correlationMax -  an integer containing the maximum allowed size
	 * of this correlation list
	 */
	void setCorrelationMax(int correlationMax);

	/**
	 * \brief Get the total number of correlations processed by this list
	 * \return Return an integer containing the total number of correlations
	 * processed by this list
	 */
	int getCorrelationTotal() const;

	/**
	 * \brief Get the current number of correlations contained in this list
	 * \return Return an integer containing the current number of correlations
	 * contained in this list
	 */
	int size() const;

	/**
	 * \brief Get a vector of correlations that fall within a time window
	 *
	 * Get a vector of correlations that fall within the provided time window
	 * from t1 to t2
	 *
	 * \param t1 - A double value containing the beginning of the time window in
	 * julian seconds
	 * \param t2 - A double value containing the end of the time window in
	 * julian seconds
	 * \return Return a std::vector of std::weak_ptrs to the correlations within
	 * the time window
	 */
	std::vector<std::weak_ptr<CCorrelation>> getCorrelations(double t1,
																double t2);

 private:
	/**
	 * \brief A pointer to a CSiteList object containing all the sites for
	 * lookups
	 */
	CSiteList * m_pSiteList;

	/**
	 * \brief An integer containing the maximum number of correlations allowed in
	 * CCorrelationList. This value is overridden by pGlass->getMaxNumCorrelations()
	 * if provided. Defaults to 10000.
	 */
	std::atomic<int> m_iCorrelationMax;

	/**
	 * \brief An integer containing the total number of correlations ever added
	 * to CCorrelationList
	 */
	std::atomic<int> m_iCorrelationTotal;

	/**
	 * \brief A std::multiset containing each correlation in the list in sequential
	 * time order from oldest to youngest.
	 */
	std::multiset<std::shared_ptr<CCorrelation>, CorrelationCompare> m_msCorrelationList;  // NOLINT

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

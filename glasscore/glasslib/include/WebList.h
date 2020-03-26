/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef WEBLIST_H
#define WEBLIST_H

#include <json.h>
#include <vector>
#include <memory>
#include <mutex>

namespace glasscore {

// forward declarations
class CSite;
class CSiteList;
class CWeb;

/**
 * \brief glasscore detection node class
 * The detection fabric consists of a collection of nets which
 * in turn contain a list of nodes, each of which is capable of
 * detecting and nucleating a new event. The individual nets are
 * added to the detection fabric (CWeb) by such commands as 'Grid'
 * or 'Global'.
 *
 * CWebList uses smart pointers (std::shared_ptr).
 */
class CWebList {
 public:
	/**
	 * \brief CWebList constructor
	 */
	explicit CWebList(int numThreads = 0);

	/**
	 * \brief CWebList destructor
	 */
	~CWebList();

	/**
	 * \brief CWebList clear function
	 *
	 * The clear function for the CWebList class.
	 */
	void clear();

	/**
	 * \brief CWebList communication receiving function
	 *
	 * The function used by CWebList to receive communication
	 * (such as configuration or input data), from outside the
	 * glasscore library, or it's parent CGlass.
	 *
	 * Supports Global (generate global grid) Shell (generate grid at
	 * single depth), Grid (generate local grid), Single (generate
	 * single node), GetWeb (generate output message detailing detection
	 * graph database) inputs.
	 *
	 * \param com - A pointer to a json::object containing the
	 * communication.
	 * \return Returns true if the communication was handled by CWebList,
	 * false otherwise
	 */
	bool receiveExternalMessage(std::shared_ptr<json::Object> com);

	/**
	 * \brief Add web ('Grid', etc) to list using provided configuration
	 *
	 * This method creates a web and then configures it by passing the
	 * configuration to the 'dispatch' message in the CWeb
	 *
	 * This capability is to add a local aftershock (or induced seismicity) web
	 * quickly (and without restarting anything) after a main shock, and then to
	 * remove it (again without a restart) once the seismicity dies down,
	 * through some glass configuration control channel (i.e. kafka). The belief
	 * going forward is that this will be a relatively common occurrence.
	 *
	 * \param com - A pointer to a json::object containing the web configuration
	 *
	 * \return Always returns true
	 */
	bool addWeb(std::shared_ptr<json::Object> com);

	/**
	 * \brief Remove web ('Grid', etc) from list by name
	 *
	 * This method removes a web from the list
	 *
	 * This capability is to add a local aftershock (or induced seismicity) web
	 * quickly (and without restarting anything) after a main shock, and then to
	 * remove it (again without a restart) once the seismicity dies down,
	 * through some glass configuration control channel (i.e. kafka). The belief
	 * going forward is that this will be a relatively common occurrence.
	 * \param com - A pointer to a json::object containing name of the web
	 * to be removed.
	 *
	 * \return Always returns true
	 */
	bool removeWeb(std::shared_ptr<json::Object> com);

	/**
	 * \brief Update a site in the webs
	 * This function updates the given site in all appropriate webs
	 * in the list of webs
	 *
	 * \param site - A shared_ptr to a CSite object containing the site to add
	 */
	void updateSite(std::shared_ptr<CSite> site);

	/**
	 * \brief Check if the webs have a site
	 * This function checks to see if the given site is used in any web in the
	 * list
	 *
	 * \param site - A shared pointer to a CSite object containing the site to
	 * check
	 */
	bool hasSite(std::shared_ptr<CSite> site);

	/**
	 * \brief check to see if each web thread is still functional
	 *
	 * Checks each web thread to see if it is still responsive.
	 */
	bool healthCheck();

	/**
	 * \brief Get the CSiteList pointer used by this web list for site lookups
	 * \return Return a pointer to the CSiteList class used by this web list
	 */
	const CSiteList* getSiteList() const;

	/**
	 * \brief Set the CSiteList pointer used by this web list for site lookups
	 * \param siteList - a pointer to the CSiteList class used by this web list
	 */
	void setSiteList(CSiteList* siteList);

	/**
	 * \brief Get the current size of the web list
	 */
	int size() const;

 private:
	/**
	 * \brief A pointer to a CSiteList object containing all the sites for
	 * lookups
	 */
	CSiteList * m_pSiteList;

	/**
	 * \brief A vector of shared pointers to all currently defined
	 * webs.
	 */
	std::vector<std::shared_ptr<CWeb>> m_vWebs;

	/**
	 * \brief An integer indicating how many threads each web should have
	 */
	int m_iNumThreads;

	/**
	 * \brief A recursive_mutex to control threading access to CCorrelationList.
	 * NOTE: recursive mutexes are frowned upon, so maybe redesign around it
	 * see: http://www.codingstandard.com/rule/18-3-3-do-not-use-stdrecursive_mutex/
	 * However a recursive_mutex allows us to maintain the original class
	 * design as delivered by the contractor.
	 */
	mutable std::recursive_mutex m_WebListMutex;
};
}  // namespace glasscore
#endif  // WEBLIST_H

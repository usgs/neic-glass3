/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef SITELIST_H
#define SITELIST_H

#include <json.h>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <mutex>

namespace glasscore {

// forward declarations
class CSite;
class CGlass;

/**
 * \brief glasscore site list class
 *
 * The CSiteList class is the class that maintains a std::map of all the seismic
 * stations used by glasscore.
 *
 * CSiteList also maintains a std::vector mapping the string SCNL id
 * to CSite objects
 *
 * CSiteList contains functions to support new data input and
 * clearing the list
 *
 * CSiteList uses smart pointers (std::shared_ptr).
 */
class CSiteList {
 public:
	/**
	 * \brief CSiteList constructor
	 *
	 * The constructor for the CSiteList class.
	 * Initializes members to default values.
	 */
	CSiteList();

	/**
	 * \brief CSiteList destructor
	 *
	 * The destructor for the CSiteList class.
	 */
	~CSiteList();

	/**
	 * \brief CSiteList clear function
	 *
	 */
	void clear();

	/**
	 * \brief CSiteList vector and map clear function
	 *
	 */
	void clearSites();

	/**
	 * \brief CSiteList communication receiving function
	 *
	 * The function used by CSiteList to receive communication
	 * (such as configuration or input data), from outside the
	 * glasscore library, or it's parent CGlass.
	 *
	 * Supports Site (add site data to list) SiteUse (enable or
	 * disable a site) and ClearGlass (clear all Site data) inputs.
	 *
	 * \param com - A pointer to a json::object containing the
	 * communication.
	 * \return Returns true if the communication was handled by CSiteList,
	 * false otherwise
	 */
	bool dispatch(std::shared_ptr<json::Object> com);

	/**
	 * \brief CSiteList add/update  site function
	 *
	 * The function used by CSiteList to add a site to, or update a site in the
	 * vector and map.
	 *
	 * \param com - A pointer to a json::object containing the site to add or
	 * update.
	 * \return Returns true if the site was complete and added by CSiteList,
	 * false otherwise
	 */
	bool addSite(std::shared_ptr<json::Object> com);

	/**
	 * \brief CSiteList add list of sites function
	 *
	 * The function used by CSiteList to add a whole list of sites
	 *
	 * \param com - A pointer to a json::array containing the list of
	 * sites to add
	 * \return Returns true if the site was complete and added by CSiteList,
	 * false otherwise
	 */
	bool addSiteList(std::shared_ptr<json::Object> com);

	/**
	 * \brief CSiteList  add/update  site function
	 *
	 * The function used by CSiteList to add a site to, or update a site in the
	 * vector and map.
	 *
	 * \param site - A shared pointer pointer to the site to add or update.
	 * \return Returns true if the site was complete and added by CSiteList,
	 * false otherwise
	 */
	bool addSite(std::shared_ptr<CSite> site);

	/**
	 * \brief CSiteList remove site function
	 *
	 * The function used by CSiteList to remove a from the vector and map.
	 *
	 * \param site - A pointer to a json::object containing the site to remove.
	 * \return Returns true if the site was removed, false otherwise
	 */
	// bool remSite(json::Object *com);
	/**
	 * \brief CSiteList remove site function
	 *
	 * The function used by CSiteList to remove a from the vector and map.
	 *
	 * \param site - A shared pointer pointer to the site to remove.
	 * \return Returns true if the site was removed, false otherwise
	 */
	// bool remSite(shared_ptr<CSite> site);
	/**
	 * \brief CSiteList enable/disable site function
	 *
	 * The function is used by CSiteList to change the state of the usage
	 * flag for a given site
	 *
	 * \param com - A pointer to a json::object containing the
	 * siteuse message.
	 * \return Returns true if the site was complete and added by CSiteList,
	 * false otherwise
	 */
	// bool useSite(json::Object *com);
	/**
	 * \brief CSiteList Site count function
	 * \return Returns an integer variable containing the number of sites in
	 * CSiteList
	 */
	int getSiteCount();

	/**
	 * \brief Get site by index
	 * Gets a specific site using the given index.
	 *
	 * \param ix - An integer variable containing the index
	 * \return Returns a shared_ptr to the CSite object containing the desired
	 * site.
	 */
	std::shared_ptr<CSite> getSite(int ix);

	/**
	 * \brief Get site by scnl
	 * Gets a specific site using the given scnl id.
	 *
	 * \param scnl - A std::string containing the scnl
	 * \return Returns a shared_ptr to the CSite object containing the desired
	 * site.
	 */
	std::shared_ptr<CSite> getSite(std::string scnl);

	/**
	 * \brief Get site by scnl
	 * Gets a specific site using the given station, component, network, and
	 * location
	 *
	 * \param site - A std::string containing the station
	 * \param comp - A std::string containing the component
	 * \param net - A std::string containing the network
	 * \param loc - A std::string containing the location
	 * \param lookup - A boolean value indicating whether to send a lookup
	 * message if the site is not found.
	 * \return Returns a shared_ptr to the CSite object containing the desired
	 * site.
	 */
	std::shared_ptr<CSite> getSite(std::string site, std::string comp,
									std::string net, std::string loc,
									bool lookup = true);

	std::vector<std::shared_ptr<CSite>> getSiteList();

	/**
	 * \brief CSiteList get site list function
	 * \return Returns true if successful, false otherwise
	 */
	bool reqSiteList();

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
	 * \brief Get the current size of the site list
	 */
	int getVSiteSize() const;

 private:
	/**
	 * \brief A pointer to the main CGlass class, used to pass this information
	 * to sites added to CSiteList
	 */
	CGlass *pGlass;

	/**
	 * \brief A mutex to control threading access to vSite.
	 */
	mutable std::mutex vSiteMutex;

	/**
	 * \brief A std::vector of all the sites in CSiteList.
	 */
	std::vector<std::shared_ptr<CSite>> vSite;

	/**
	 * \brief A std::map containing a std::shared_ptr to each site
	 * in CSiteList indexed by the std::string scnl id.
	 */
	std::map<std::string, std::shared_ptr<CSite>> mSite;

	/**
	 * \brief A std::map containing a std::shared_ptr to each site
	 * in CSiteList indexed by the std::string scnl id.
	 */
	std::map<std::string, int> mLookup;

	/**
	 * \brief A recursive_mutex to control threading access to CSiteList.
	 * NOTE: recursive mutexes are frowned upon, so maybe redesign around it
	 * see: http://www.codingstandard.com/rule/18-3-3-do-not-use-stdrecursive_mutex/
	 * However a recursive_mutex allows us to maintain the original class
	 * design as delivered by the contractor.
	 */
	mutable std::recursive_mutex m_SiteListMutex;
};
}  // namespace glasscore
#endif  // SITELIST_H

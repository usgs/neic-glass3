/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef SITELIST_H
#define SITELIST_H

#include <threadbaseclass.h>

#include <json.h>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>

namespace glasscore {

// forward declarations
class CSite;

/**
 * \brief glasscore site list class
 *
 * The CSiteList class is the class that maintains a vector and :map of all the
 * seismic stations used by glasscore.
 *
 * CSiteList also maintains a std::vector mapping the string SCNL id
 * to CSite objects
 *
 * CSiteList contains functions to support new data input and
 * clearing the list
 *
 * CSiteList uses smart pointers (std::shared_ptr).
 */
class CSiteList : public glass3::util::ThreadBaseClass {
 public:
	/**
	 * \brief CSiteList constructor
	 *
	 * The constructor for the CSiteList class.
	 * Initializes members to default values.
	 */
	explicit CSiteList(int numThreads = 1, int sleepTime = 100,
						int checkInterval = 60);

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
	void clear() override;

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
	bool receiveExternalMessage(std::shared_ptr<json::Object> com);

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
	bool addSiteFromJSON(std::shared_ptr<json::Object> com);

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
	bool addListOfSitesFromJSON(std::shared_ptr<json::Object> com);

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
	 * \brief CSiteList Site count function
	 * \return Returns an integer variable containing the number of sites in
	 * CSiteList
	 */
	int size() const;

	/**
	 * \brief Get site by scnl
	 *
	 * Gets a specific site using the given scnl id.
	 *
	 * \param scnl - A std::string containing the scnl
	 * \return Returns a shared_ptr to the CSite object containing the desired
	 * site.
	 */
	std::shared_ptr<CSite> getSite(std::string scnl);

	/**
	 * \brief Get site by station, component, network, and location
	 *
	 * Gets a specific site using the given station, component, network, and
	 * location
	 *
	 * \param site - A std::string containing the station
	 * \param comp - A std::string containing the component
	 * \param net - A std::string containing the network
	 * \param loc - A std::string containing the location
	 * \return Returns a shared_ptr to the CSite object containing the desired
	 * site.
	 */
	std::shared_ptr<CSite> getSite(std::string site, std::string comp,
									std::string net, std::string loc);

	/**
	 * \brief Get current list of sites
	 *
	 * Gets the current list of sites contained within this sitelist
	 *
	 * \return Returns a vector of shared_ptr's to the CSite objects contained
	 * in this list
	 */
	std::vector<std::shared_ptr<CSite>> getListOfSites();

	/**
	 * \brief Generate SiteList message
	 *
	 * Generate a json object representing all thie sites in this SiteList in the
	 * "SiteList" format and optionally send a pointer to this object to CGlass
	 * (and out of glasscore) using the CGlass send function (pGlass->send)
	 *
	 * \param send - A boolean flag indicating that in addition to generating
	 * the "SiteList" format message, the function should also send it. Defaults
	 * to true
	 * \return Returns the generated json object in the "SiteList" format.
	 */
	std::shared_ptr<json::Object> generateSiteListMessage(bool send = true);

	/**
	 * \brief Get the maximum hours without picking before a site is declared
	 * nonresponsive and unused, a -1 disables this metric
	 * \return Return an integer containing the maximum hours without picking
	 *  allowed
	 */
	int getMaxHoursWithoutPicking() const;

	/**
	 * \brief Set the maximum hours without picking before a site is declared
	 * nonresponsive and unused, a -1 disables this metric
	 * \param hoursWithoutPicking - an integer containing the maximum hours
	 * without picking allowed
	 */
	void setMaxHoursWithoutPicking(int hoursWithoutPicking);

	/**
	 * \brief Get the maximum hours between requesting site information from
	 * outside glasscore, a -1 disables this process
	 * \return Return an integer containing the maximum hours between requesting
	 * site information
	 */
	int getHoursBeforeLookingUp() const;

	/**
	 * \brief Set the maximum hours between requesting site information from
	 * outside glasscore, a -1 disables this process. If enabled, glass will
	 * always re-request the coordinates and properties of this station every
	 * hoursBeforeLookingUp hours, regardless if the site is currently known.
	 * \param hoursBeforeLookingUp - an integer containing the maximum hours
	 * between requesting site information
	 */
	void setHoursBeforeLookingUp(int hoursBeforeLookingUp);

	/**
	 * \brief Get the maximum picks per hour before a site is declared too
	 * noisy to use, a -1 disables this metric
	 * \return Return an integer containing the  maximum picks per hour allowed
	 */
	int getMaxPicksPerHour() const;

	/**
	 * \brief Set the maximum picks per hour before a site is declared too
	 * noisy to use, a -1 disables this metric
	 * \param maxPicksPerHour - an integer containing the maximum picks per
	 * hour allowed
	 */
	void setMaxPicksPerHour(int maxPicksPerHour);

	/**
	 * \brief Get last time in epoch seconds the site list was modified
	 * \return Return the last time in epoch seconds the site list was modified
	 */
	int getLastUpdated() const;

	/**
	 * \brief SiteList work function
	 *
	 * checks sites
	 * \return returns glass3::util::WorkState::OK if work was successful,
	 * glass3::util::WorkState::Error if not.
	 */
	glass3::util::WorkState work() override;

 private:
	/**
	 * \brief A std::vector of all the sites in CSiteList.
	 */
	std::vector<std::shared_ptr<CSite>> m_vSite;

	/**
	 * \brief A std::map containing a std::shared_ptr to each site
	 * in CSiteList indexed by the std::string scnl id.
	 */
	std::map<std::string, std::shared_ptr<CSite>> m_mSite;

	/**
	 * \brief A std::map the last time in epoch seconds each site in CSiteList
	 * was looked up, indexed by the std::string scnl id.
	 */
	std::map<std::string, int> m_mLastTimeSiteLookedUp;

	/**
	 * \brief A recursive_mutex to control threading access to CSiteList.
	 * NOTE: recursive mutexes are frowned upon, so maybe redesign around it
	 * see: http://www.codingstandard.com/rule/18-3-3-do-not-use-stdrecursive_mutex/
	 * However a recursive_mutex allows us to maintain the original class
	 * design as delivered by the contractor.
	 */
	mutable std::recursive_mutex m_SiteListMutex;

	/**
	 * \brief A double value containing the last time the site list was checked
	 * in epoch seconds
	 */
	std::atomic<double> m_tLastChecked;

	/**
	 * \brief An integer containing the maximum hours between requesting site
	 * information from outside glasscore, a -1 disables this process
	 */
	std::atomic<int> m_iHoursBeforeLookingUp;

	/**
	 * \brief An integer containing the maximum hours without picking before a
	 * site is declared nonresponsive and unused, a -1 disables this metric
	 */
	std::atomic<int> m_iMaxHoursWithoutPicking;

	/**
	 * \brief An integer containing the maximum picks per hour allowed before a
	 * site is declared too noisy to use, a -1 disables this metric
	 */
	std::atomic<int> m_iMaxPicksPerHour;

	/**
	 * \brief An integer containing the epoch time that this list was last 
	 * modified.
	 */
	std::atomic<int> m_tLastUpdated;

	/**
	 * \brief An integer containing the epoch time that this list was created.
	 */
	std::atomic<int> m_tCreated;

	// constants
	/**
	 * \brief The factor used to convert hours to seconds
	 */
	static const int k_nHoursToSeconds = 3600;
};
}  // namespace glasscore
#endif  // SITELIST_H

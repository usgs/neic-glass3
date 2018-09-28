/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef WEB_H
#define WEB_H

#include <threadbaseclass.h>

#include <json.h>
#include <utility>
#include <string>
#include <tuple>
#include <memory>
#include <vector>
#include <mutex>
#include <thread>
#include <queue>
#include <map>
#include <atomic>

#include "TravelTime.h"
#include "ZoneStats.h"

namespace glasscore {

// forward declarations
class CSite;
class CSiteList;
class CNode;
class CPick;

/**
 * \brief glasscore detection web class
 *
 * The CWeb class represents a detection graph database.  CWeb contains
 * the list of all the detection nodes.
 *
 * CWeb uses a background thread to handle live reconfiguration / updates
 * of the web.
 *
 * CWeb uses smart pointers (std::shared_ptr).
 */
class CWeb : public glass3::util::ThreadBaseClass {
 public:
	/**
	 * \brief CWeb constructor
	 *
	 * The constructor for the CWeb class.
	 * \param numThreads - An integer containing the desired number of background
	 * threads to process web updates.
	 * \param sleepTime - An integer containing the amount of
	 * time to sleep in milliseconds between jobs.  Default 10
	 * \param checkInterval - An integer containing the amount of time in
	 * seconds between status checks. -1 to disable status checks.  Default 300.
	 */
	explicit CWeb(int numThreads = 0, int sleepTime = 100, int checkInterval =
							60);

	/**
	 * \brief CWeb advanced constructor
	 *
	 * The advanced constructor for the CWeb class.
	 * Initializes members to provided values.
	 * \param name - A std::string containing the name of the web.
	 * \param thresh - A double value containing this web's nucleation
	 * threshold.
	 * \param numDetect - An integer value containing the number of sites to
	 * link with each node in the web.
	 * \param numNucleate - An integer value containing the number of sites
	 * required for the web to nucleate an event.
	 * \param resolution - A double value containing the desired resolution
	 * for this web
	 * \param update - A boolean flag indicating whether this web is allowed to
	 * update
	 * \param save - A boolean flag indicating whether this web should save it's
	 * nodes to a file
	 * \param firstTrav - A shared pointer to the first CTravelTime object to
	 * use for travel time lookups.
	 * \param secondTrav - A shared pointer to the second CTravelTime object to
	 * use for travel time lookups.
	 * \param numThreads - An integer containing the desired number of background
	 * threads to process web updates, if set to 0, glass will
	 * halt until the web update is completed. Default 0.
	 * \param sleepTime - An integer containing the amount of
	 * time to sleep in milliseconds between jobs.  Default 10
	 * \param checkInterval - An integer containing the amount of time in
	 * seconds between status checks. -1 to disable status checks.  Default 60.
	 * \param aziTaper = A double value containing the azimuth taper to be used,
	 * defaults to 360
	 * \param maxDepth = A double value containing the maximum allowable depth
	 * defaults to 800 km.
	 */
	CWeb(std::string name, double thresh, int numDetect, int numNucleate,
			int resolution, bool update, bool save,
			std::shared_ptr<traveltime::CTravelTime> firstTrav,
			std::shared_ptr<traveltime::CTravelTime> secondTrav,
			int numThreads = 0, int sleepTime = 100, int checkInterval = 60,
			double aziTaper = 360.0, double maxDepth = 800.0);

	/**
	 * \brief CWeb destructor
	 *
	 * The destructor for the CWeb class.
	 */
	~CWeb();

	/**
	 * \brief CWeb clear function
	 *
	 * The clear function for the CWeb class.
	 */
	void clear() override;

	/**
	 * \brief CWeb communication recieveing function
	 *
	 * The function used by CWeb to recieve communication
	 * (such as configuration or input data), from outside the
	 * glasscore library, or it's parent CGlass.
	 *
	 * Supports Global (genrate global grid) Shell (generate grid at
	 * single depth), Grid (generate local grid), Single (generate
	 * single node), GetWeb (generate output message detailing detection
	 * graph database) inputs.
	 *
	 * \param com - A pointer to a json::object containing the
	 * communication.
	 * \return Returns true if the communication was handled by CWeb,
	 * false otherwise
	 */
	bool receiveExternalMessage(std::shared_ptr<json::Object> com);

	/**
	 * \brief CWeb initialization function
	 *
	 * \param name - A std::string containing the name of the web.
	 * \param thresh - A double value containing this web's nucleation
	 * threshold.
	 * \param numDetect - An integer value containing the number of sites to
	 * link with each node in the web.
	 * \param numNucleate - An integer value containing the number of sites
	 * required for the web to nucleate an event.
	 * \param resolution - A double value containing the desired resolution
	 * for this web
	 * \param update - A boolean flag indicating whether this web is allowed to
	 * update
	 * \param save - A boolean flag indicating whether this web should save it's
	 * nodes to a file
	 * \param firstTrav - A shared pointer to the first CTravelTime object to
	 * use for travel time lookups.
	 * \param secondTrav - A shared pointer to the second CTravelTime object to
	 * use for travel time lookups.
	 * \param aziTaper = A double value containing the azimuth taper to be used,
	 * defaults to 360
	 * \param maxDepth = A double value containing the maximum allowable depth
	 * defaults to 800 km.
	 * \return Returns true if successful, false otherwise
	 */
	bool initialize(std::string name, double thresh, int numDetect,
					int numNucleate, int resolution, bool update, bool save,
					std::shared_ptr<traveltime::CTravelTime> firstTrav,
					std::shared_ptr<traveltime::CTravelTime> secondTrav,
					double aziTaper = 360.0, double maxDepth = 800.0);

	/**
	 * \brief Generate a local detection grid
	 *
	 * This function initializes this web to the provided configuration and
	 * generates a local detection grid of nodes
	 *
	 * \param gridConfiguration - A pointer to a json::object containing desired
	 * node grid configuration
	 * \return Returns true if successful, false if a grid was not created.
	 */
	bool generateLocalGrid(std::shared_ptr<json::Object> gridConfiguration);

	/**
	 * \brief Generate a detection grid with explicit nodes
	 *
	 * This function initializes this web to the provided configuration and
	 * generates a detection grid of explicitly defined nodes
	 *
	 * \param gridConfiguration - A pointer to a json::object containing desired
	 * node configuration
	 * \return Returns true if successful, false if a grid was not created.
	 */
	bool generateExplicitGrid(std::shared_ptr<json::Object> gridConfiguration);

	/**
	 * \brief Generate a global detection grid
	 *
	 * This function initializes this web to the provided configuration and
	 * generates a global detection grid of nodes
	 *
	 * \param gridConfiguration - A pointer to a json::object containing desired
	 * node configuration
	 * \return Returns true if successful, false if a grid was not created.
	 */
	bool generateGlobalGrid(std::shared_ptr<json::Object> gridConfiguration);

	/**
	 * \brief Load the common grid configuration
	 *
	 * This function initializes this web to the provided configuration
	 *
	 * \param gridConfiguration - A pointer to a json::object containing desired
	 * configuration
	 * \return Returns true if successful, false otherwise.
	 */
	bool loadGridConfiguration(std::shared_ptr<json::Object> gridConfiguration);

	/**
	 * \brief Load the travel times for this web
	 *
	 * This function loads the travel times used by this web using the provided
	 * configuration
	 *
	 * \param gridConfiguration - A pointer to a json::object containing the web
	 * configuration
	 * \return Returns true if successful, false otherwise.
	 */
	bool loadTravelTimes(json::Object *gridConfiguration);

	/**
	 * \brief Load Site filters
	 *
	 * This function loads the station and network filter lists used in
	 * selecting eligible sites from the provided cofiguration
	 *
	 * \param gridConfiguration - A pointer to a json::object containing the web
	 * configuration
	 * \return Returns true if successful, false otherwise.
	 */
	bool loadSiteFilters(std::shared_ptr<json::Object> gridConfiguration);

	/**
	 * \brief Load web site list
	 *
	 * This function loads the list of eligible sites (stations) to be used
	 * by this web, filtering out unused/disabled sites and sites excluded by
	 * the site filters via isSiteAllowed(). This list is stored in vSite.
	 *
	 * \return Always returns true
	 */
	bool loadWebSiteList();

	/**
	 * \brief Sort site list
	 *
	 * This function sorts a list of sites stored in vSite in increasing
	 * distance from the given location at 0 depth.  The function also populates
	 * the distance between the given location and the site as part of the
	 * std::pair in vSite.
	 *
	 * \param lat - A double varible containing the latitude to use
	 * \param lon - A double varible containing the longitude to use
	 */
	void sortSiteListForNode(double lat, double lon);

	/**
	 * \brief Create new node
	 *
	 * This function creates a new node centered on the provided latitude,
	 * longitude, depth, and spatial resolution.  The new node is linked to
	 * the N closest sites (stations) where N is defined by nDetect.
	 *
	 * \param lat - A double variable containing the latitude to use
	 * \param lon - A double variable containing the longitude to use
	 * \param z - A double variable containing the depth to use
	 * \param resol - A double variable containing the spatial resolution to use
	 * \return Returns a std::shared_ptr to the newly created node.
	 */
	std::shared_ptr<CNode> generateNode(double lat, double lon, double z,
										double resol);

	/**
	 * \brief Add node to list
	 *
	 * This function adds a new node to the list of nodes
	 *
	 * \param node - A std::shared_ptr to the node to add
	 * \return Returns true if successful, false otherwise
	 */
	bool addNode(std::shared_ptr<CNode> node);

	/**
	 * \brief Create list of sites for node
	 *
	 * This function links a node to the N closest sites (stations) where N is
	 * defined by nDetect.
	 *
	 * \param node - A std::shared_ptr to the node to link sites to
	 * \return Returns a std::shared_ptr to the updated node.
	 */
	std::shared_ptr<CNode> generateNodeSites(std::shared_ptr<CNode> node);

	/**
	 * \brief Add site to this web
	 * This function adds the given site to the list of nodes linked to this
	 * web and restructure node site lists
	 *
	 * \param site - A shared_ptr to a CSite object containing the site to add
	 */
	void addSite(std::shared_ptr<CSite> site);

	/**
	 * \brief Remove site from this web
	 * This function removes the given site to the list of nodes linked to this
	 * web and restructure node site lists
	 *
	 * \param site - A shared pointer to a CSite object containing the site to
	 * remove
	 */
	void removeSite(std::shared_ptr<CSite> site);

	/**
	 * \brief Check if this web has a site
	 * This function checks to see if the given site is used for this web,
	 * used by CWebList
	 *
	 * \param site - A shared pointer to a CSite object containing the site to
	 * check
	 */
	bool hasSite(std::shared_ptr<CSite> site);

	/**
	 * \brief Check to see if site allowed
	 * This function checks to see if a given site is allowed in the web by
	 * checking the given site against the configured site filters
	 *
	 * \param site - A shared pointer to a CSite object containing the site to
	 * check
	 * \return returns true if it is allowed, false otehrwise
	 */
	bool isSiteAllowed(std::shared_ptr<CSite> site);

	/**
	 * \brief add a job
	 *
	 * Adds a job to the queue of jobs to be run by the background
	 * thread
	 * \param newjob - A std::function<void()> bound to the function
	 * containing the job to run
	 */
	void addJob(std::function<void()> newjob);

	/**
	 * \brief Get the azimuth taper used for this web
	 * \return Returns a double value containing the taper to use
	 */
	double getAzimuthTaper() const;

	/**
	 * \brief Get the maximum depth used for this web
	 * \return Returns a double value containing the maximum depth to use
	 */
	double getMaxDepth() const;

	/**
	 * \brief Get the CSiteList pointer used by this web for site lookups
	 * \return Return a pointer to the CSiteList class used by this web
	 */
	const CSiteList* getSiteList() const;

	/**
	 * \brief Set the CSiteList pointer used by this web for site lookups
	 * \param siteList - a pointer to the CSiteList class used by this web
	 */
	void setSiteList(CSiteList* siteList);

	/**
	 * \brief Gets a flag indicating whether this web supports updates
	 * \return Returns a boolean flag indicating whether this web supports
	 * updates, true if it does, false otherwise
	 */
	bool getUpdate() const;

	/**
	 * \brief Gets a flag indicating whether this web supports saving its nodes
	 * to an external file
	 * \return Returns a boolean flag indicating whether this web supports
	 * saving it's nodes, true if it does, false otherwise
	 */
	bool getSaveGrid() const;

	/**
	 * \brief Gets the node resolution of this web
	 * \return Returns a double value containing the node resolution of this web
	 * in kilometers
	 */
	double getNodeResolution() const;

	/**
	 * \brief Gets the nucleation minimum stack threshold used for this web
	 * \return Returns a double value containing the nucleation minimum stack
	 * threshold
	 */
	double getNucleationStackThreshold() const;

	/**
	 * \brief Gets the number of stations to link to each node in this web
	 * \return Returns an integer value containing the number of stations to
	 * link to each node
	 */
	int getNumStationsPerNode() const;

	/**
	 * \brief Gets the nucleation data minimum threshold used for this web
	 * \return Returns an integer value containing the nucleation data minimum
	 * threshold
	 */
	int getNucleationDataCountThreshold() const;

	/**
	 * \brief Gets the name of this web
	 * \return Returns a std::string containing the name of this web
	 */
	const std::string& getName() const;

	/**
	 * \brief Gets the primary nucleation travel time for this web
	 * \return Returns a shared_ptr to the CTravelTime containing the primary
	 * nucleation travel time
	 */
	const std::shared_ptr<traveltime::CTravelTime>& getNucleationTravelTime1() const;  // NOLINT

	/**
	 * \brief Gets the secondary nucleation travel time for this web
	 * \return Returns a shared_ptr to the CTravelTime containing the secondary
	 * nucleation travel time
	 */
	const std::shared_ptr<traveltime::CTravelTime>& getNucleationTravelTime2() const;  // NOLINT

	/**
	 * \brief Get the number of network filters for this web
	 * \return Returns an integer containing the number of network filters
	 */
	int getNetworksFilterSize() const;

	/**
	 * \brief Get the number of site filters for this web
	 * \return Returns an integer containing the number of site filters
	 */
	int getSitesFilterSize() const;

	/**
	 * \brief Gets the flag indicating whether this web should only use sites
	 * flagged as UseForTeleseismic
	 * \return Returns a boolean flag indicating hether this web should only
	 * use sites  lagged as UseForTeleseismic
	 */
	bool getUseOnlyTeleseismicStations() const;

	/**
	 * \brief Get the number of nodes in this web
	 * \return Returns an integer value containing the number of nodes in this
	 * web
	 */
	int size() const;

	/**
	 * \brief Web work function
	 *
	 * Update a web in the background
	 * \return returns glass3::util::WorkState::OK if work was successful,
	 * glass3::util::WorkState::Error if not.
	 */
	glass3::util::WorkState work() override;

 private:
	/**
	 * \brief A pointer to the CSiteList class, used get sites (stations)
	 */
	CSiteList * m_pSiteList;

	/**
	 * \brief A std::vector containing the network names to be included
	 * in this web. This needs to be saved to support dynamic addition
	 * and removal of sites as their usage status is changed.
	 */
	std::vector<std::string> m_vNetworksFilter;

	/**
	 * \brief A std::vector containing the SCNL names to be included
	 * in this web. This needs to be saved to support dynamic addition
	 * and removal of sites as their usage status is changed.
	 */
	std::vector<std::string> m_vSitesFilter;

	/**
	 * \brief A boolean flag indicating whether to only use sites marked
	 * "UseForTeleseismic" This needs to be saved to support dynamic addition
	 * and removal of sites as their usage status is changed.
	 */
	std::atomic<bool> m_bUseOnlyTeleseismicStations;

	/**
	 * \brief A std::vector containing a std::pair for each the sites to use
	 * in node generation and the distance to the current node location.
	 * vSite is populated by genSiteList(), and sorted by sortSiteList() (which
	 * also fills in the distance), and used by genNode() during a Single(),
	 * Shell(), Grid(), or Global() call
	 */
	std::vector<std::pair<double, std::shared_ptr<CSite>>>m_vSitesSortedForCurrentNode;  // NOLINT

	/**
	 * \brief A std::vector containing a std::shared_ptr to each node in this
	 * web
	 */
	std::vector<std::shared_ptr<CNode>> m_vNode;

	/**
	 * \brief the std::mutex for accessing m_vNode
	 */
	mutable std::mutex m_vNodeMutex;

	/**
	 * \brief The name identifying this web
	 */
	std::string m_sName;

	/**
	 * \brief A double value containing the number of closest stations to use
	 * when generating a node for this detection array. This number overrides
	 * the default Glass parameter if it is provided
	 */
	std::atomic<int> m_iNumStationsPerNode;

	/**
	 * \brief A double value containing the number picks of that need to be
	 * gathered to trigger the nucleation of an event. This number overrides the
	 * default Glass parameter if it is provided
	 */
	std::atomic<int> m_iNucleationDataCountThreshold;

	/**
	 * \brief A double value containing the viability threshold needed to
	 * exceed for a nucleation to be successful. This value overrides the
	 * default Glass parameter if it is provided
	 */
	std::atomic<double> m_dNucleationStackThreshold;

	/**
	 * \brief A double value containing the inter-node resolution for this
	 * detection array
	 */
	std::atomic<double> m_dNodeResolution;

	/**
	 * \brief A double which describes the web specific value where the locator
	 * should start down weighting for azimuthal gap
	 **/
	std::atomic<double> m_dAzimuthTaper;

	/**
	 * \brief A double which describes the web specific maximum allowable event
	 * depth
	 **/
	std::atomic<double> m_dMaxDepth;

	/**
	 * \brief A double which describes the web depth layer resolution
	 **/
	std::atomic<double> m_dDepthResolution;

	/**
	 * \brief A boolean flag that stores whether to update this web when a
	 * station has changed.
	 */
	std::atomic<bool> m_bUpdate;

	/**
	 * \brief A boolean flag that stores whether this web should save it's
	 * nodes to a file
	 */
	std::atomic<bool> m_bSaveGrid;

	/**
	 * \brief A pointer to a CTravelTime object containing
	 * travel times for the first phase used by this web for nucleation
	 */
	std::shared_ptr<traveltime::CTravelTime> m_pNucleationTravelTime1;

	/**
	 * \brief A pointer to a CTravelTime object containing
	 * travel times for the second phase used by this web for nucleation
	 */
	std::shared_ptr<traveltime::CTravelTime> m_pNucleationTravelTime2;

	/**
	 * \brief A mutex to control threading access to vSite.
	 */
	std::mutex m_vSiteMutex;

	/**
	 * \brief the std::queue of std::function<void() jobs
	 */
	std::queue<std::function<void()>> m_JobQueue;

	/**
	 * \brief the std::mutex for m_QueueMutex
	 */
	std::mutex m_QueueMutex;

	/**
	 * \brief string containing the filename of ZoneStats file.  Empty = no
	 * zonestats
	 */
	std::string m_sZoneStatsFileName;
	/**
	 * \brief shared pointer to ZoneStats info
	 */
	std::shared_ptr<traveltime::CZoneStats> m_pZoneStats;

	/**
	 * \brief A recursive_mutex to control threading access to CWeb.
	 * NOTE: recursive mutexes are frowned upon, so maybe redesign around it
	 * see: http://www.codingstandard.com/rule/18-3-3-do-not-use-stdrecursive_mutex/
	 * However a recursive_mutex allows us to maintain the original class
	 * design as delivered by the contractor.
	 */
	mutable std::recursive_mutex m_WebMutex;
};
}  // namespace glasscore
#endif  // WEB_H

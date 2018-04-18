/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef WEB_H
#define WEB_H

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
#include "TravelTime.h"

namespace glasscore {

// forward declarations
class CGlass;
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
 * CWeb uses smart pointers (std::shared_ptr).
 */
class CWeb {
 public:
	/**
	 * \brief CWeb constructor
	 *
	 * The constructor for the CWeb class.
	 * \param numThreads - An integer containing the desired number of background
	 * threads to process web updates, if set to 0, glass will
	 * halt until the web update is completed. Default 0.
	 * \param sleepTime - An integer containing the amount of
	 * time to sleep in milliseconds between jobs.  Default 10
	 * \param checkInterval - An integer containing the amount of time in
	 * seconds between status checks. -1 to disable status checks.  Default 300.
	 */
	CWeb(int numThreads = 0, int sleepTime = 100,
			int checkInterval = 60);

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
	 * \param numRows - An integer value containing the number of rows in this
	 * web.
	 * \param numCols - An integer value containing the number of columns in
	 * this web.
	 * \param numZ - An integer value containing the number of depths in this
	 * web.
	 * \param update - A boolean flag indicating whether this web is allowed to
	 * update
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
	 * seconds between status checks. -1 to disable status checks.  Default 300.
	 */
	CWeb(std::string name, double thresh, int numDetect, int numNucleate,
			int resolution, int numRows, int numCols, int numZ, bool update,
			std::shared_ptr<traveltime::CTravelTime> firstTrav,
			std::shared_ptr<traveltime::CTravelTime> secondTrav,
			int numThreads = 0, int sleepTime = 100,
			int checkInterval = 60);

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
	void clear();

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
	 * graph database), and ClearGlass (clear all Node data) inputs.
	 *
	 * \param com - A pointer to a json::object containing the
	 * communication.
	 * \return Returns true if the communication was handled by CWeb,
	 * false otherwise
	 */
	bool dispatch(std::shared_ptr<json::Object> com);

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
	 * \param numRows - An integer value containing the number of rows in this
	 * web.
	 * \param numCols - An integer value containing the number of columns in
	 * this web.
	 * \param numZ - An integer value containing the number of depths in this
	 * web.
	 * \param update - A boolean flag indicating whether this web is allowed to
	 * update
	 * \param firstTrav - A shared pointer to the first CTravelTime object to
	 * use for travel time lookups.
	 * \param secondTrav - A shared pointer to the second CTravelTime object to
	 * use for travel time lookups.
	 * \return Returns true if successful, false otherwise
	 */
	bool initialize(std::string name, double thresh, int numDetect,
					int numNucleate, int resolution, int numRows, int numCols,
					int numZ, bool update,
					std::shared_ptr<traveltime::CTravelTime> firstTrav,
					std::shared_ptr<traveltime::CTravelTime> secondTrav);

	/**
	 * \brief Generate a local detection grid
	 *
	 * This function generates a local detection grid of nodes, at a single
	 * depth.
	 *
	 * \param com - A pointer to a json::object containing desired node
	 * configuration
	 * \return Returns true if successful, false if a grid was not created.
	 */
	bool grid(std::shared_ptr<json::Object> com);

	/**
	 * \brief Generate a detection grid with explicit nodes
	 *
	 * This function generates a  detection grid of explicitly defined nodes
	 *
	 * \param com - A pointer to a json::object containing desired node
	 * configuration
	 * \return Returns true if successful, false if a grid was not created.
	 */
	bool grid_explicit(std::shared_ptr<json::Object> com);

	/**
	 * \brief Generate a global detection grid
	 *
	 * This function generates a global detection grid of nodes, at multiple
	 * depths.
	 *
	 * \param com - A pointer to a json::object containing desired node
	 * configuration
	 * \return Always returns true
	 */
	bool global(std::shared_ptr<json::Object> com);

	/**
	 * \brief Load the travel times for this web
	 *
	 * This function loads the travel times used by this web.
	 *
	 * \param com - A pointer to a json::object containing the web configuration
	 * \return Returns true if successful, false otherwise.
	 */
	bool loadTravelTimes(json::Object *com);

	/**
	 * \brief Generate Site filters
	 *
	 * This function generates station and network filter lists used in
	 * selecting eligible sites.
	 *
	 * \param com - A pointer to a json::object containing the web configuration
	 * \return Returns true if successful, false otherwise.
	 */
	bool genSiteFilters(std::shared_ptr<json::Object> com);

	/**
	 * \brief Generate node site list
	 *
	 * This function generates a list of eligible sites (stations) to be used
	 * while generating nodes.  This list is stored in vSite.
	 *
	 * \param com - A pointer to a json::object containing desired station
	 * criteria
	 * \return Always returns true
	 */
	bool genSiteList();

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
	void sortSiteList(double lat, double lon);

	/**
	 * \brief Create new node
	 *
	 * This function creates a new node centered on the provided latitude,
	 * longitude, depth, and spatial resolution.  The new node is linked to
	 * the N closest sites (stations) where N is defined by nDetect.
	 *
	 * \param lat - A double varible containing the latitude to use
	 * \param lon - A double varible containing the longitude to use
	 * \param z - A double varible containing the depth to use
	 * \param resol - A double varible containing the spatial resolution to use
	 * \return Returns a std::shared_ptr to the newly created node.
	 */
	std::shared_ptr<CNode> genNode(double lat, double lon, double z,
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
	 * \param trav - A pointer to a CTravelTime to use while linking sites
	 * \return Returns a std::shared_ptr to the updated node.
	 */
	std::shared_ptr<CNode> genNodeSites(std::shared_ptr<CNode> node);

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
	void remSite(std::shared_ptr<CSite> site);

	/**
	 * \brief Check if this web has a site
	 * This function checks to see if the given site is used for this web
	 *
	 * \param site - A shared pointer to a CSite object containing the site to
	 * check
	 */
	bool hasSite(std::shared_ptr<CSite> site);

	/**
	 * \brief Check to see if site allowed
	 * This function checks to see if a given site is allowed in the web
	 *
	 * \param site - A shared pointer to a CSite object containing the site to
	 * check
	 * \return returns true if it is allowed, false otehrwise
	 */
	bool isSiteAllowed(std::shared_ptr<CSite> site);

	/**
	 * \brief Background thread work loop for this web
	 */
	void workLoop();

	/**
	 * \brief check to see if the thread is still functional
	 *
	 * Checks the thread to see if it is still responsive.
	 */
	bool statusCheck();

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
	 * \brief CGlass getter
	 * \return the CGlass pointer
	 */
	CGlass* getGlass() const;

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
	bool getUpdate() const;
	double getResolution() const;
	double getThresh() const;
	int getCol() const;
	int getDetect() const;
	int getNucleate() const;
	int getRow() const;
	int getZ() const;
	const std::string& getName() const;
	const std::shared_ptr<traveltime::CTravelTime>& getTrv1() const;
	const std::shared_ptr<traveltime::CTravelTime>& getTrv2() const;
	int getVNetFilterSize() const;
	int getVSitesFilterSize() const;
	bool getUseOnlyTeleseismicStations() const;
	int getVNodeSize() const;

 private:
	/**
	 * \brief the job sleep
	 *
	 * The function that performs the sleep between jobs
	 */
	void jobSleep();

	/**
	 * \brief thread status update function
	 *
	 * Updates the status for the thread
	 * \param status - A boolean flag containing the status to set
	 */
	void setStatus(bool status);

	/**
	 * \brief A pointer to the main CGlass class, used to send output,
	 * get default values, encode/decode time, and get debug flags
	 */
	CGlass *pGlass;

	/**
	 * \brief A pointer to the CSiteList class, used get sites (stations)
	 */
	CSiteList *pSiteList;

	/**
	 * \brief A std::vector containing the network names to be included
	 * in this web. This needs to be saved to support dynamic addition
	 * and removal of sites as their usage status is changed.
	 */
	std::vector<std::string> vNetFilter;

	/**
	 * \brief A std::vector containing the SCNL names to be included
	 * in this web. This needs to be saved to support dynamic addition
	 * and removal of sites as their usage status is changed.
	 */
	std::vector<std::string> vSitesFilter;

	/**
	 * \brief A boolean flag indicating whether to only use sites marked
	 * "UseForTeleseismic" This needs to be saved to support dynamic addition
	 * and removal of sites as their usage status is changed.
	 */
	bool bUseOnlyTeleseismicStations;

	/**
	 * \brief A std::vector containing a std::pair for each the sites to use
	 * in node generation and the distance to the current node location.
	 * vSite is populated by genSiteList(), and sorted by sortSiteList() (which
	 * also fills in the distance), and used by genNode() during a Single(),
	 * Shell(), Grid(), or Global() call
	 */
	std::vector<std::pair<double, std::shared_ptr<CSite>>> vSite;

	/**
	 * \brief A std::vector containing a std::shared_ptr to each
	 * node in the detection graph database
	 */
	std::vector<std::shared_ptr<CNode>> vNode;

	/**
	 * \brief the std::mutex for m_QueueMutex
	 */
	mutable std::mutex m_vNodeMutex;

	/**
	 * \brief String name of web used in tuning
	 */
	std::string sName;

	/**
	 * \brief A integer containing the number of rows in a detection grid
	 * generated by the Grid() command
	 */
	int nRow;

	/**
	 * \brief A integer containing the number of columns in a detection grid
	 * generated by the Grid() command
	 */
	int nCol;

	/**
	 * \brief A integer containing the number of depths in a detection grid
	 * generated by the Grid() command
	 */
	int nZ;

	/**
	 * \brief A double value containing the number of closest stations to use
	 * when generating a node for this detection array. This number overrides
	 * the default Glass parameter if it is provided
	 */
	int nDetect;

	/**
	 * \brief A double value containing the number picks of that need to be
	 * gathered to trigger the nucleation of an event. This number overrides the
	 * default Glass parameter if it is provided
	 */
	int nNucleate;

	/**
	 * \brief A double value containing the viability threshold needed to
	 * exceed for a nucleation to be successful. This value overrides the
	 * default Glass parameter if it is provided
	 */
	double dThresh;

	/**
	 * \brief A double value containing the inter-node resolution for this
	 * detection array
	 */
	double dResolution;

	/**
	 * \brief A boolean flag that stores whether to update this web when a
	 * station has changed.
	 */
	bool bUpdate;

	/**
	 * \brief A pointer to a CTravelTime object containing
	 * travel times for the first phase used by this web for nucleation
	 */
	std::shared_ptr<traveltime::CTravelTime> pTrv1;

	/**
	 * \brief A pointer to a CTravelTime object containing
	 * travel times for the second phase used by this web for nucleation
	 */
	std::shared_ptr<traveltime::CTravelTime> pTrv2;

	/**
	 * \brief the std::mutex for traveltimes
	 */
	mutable std::mutex m_TrvMutex;

	/**
	 * \brief A mutex to control threading access to vSite.
	 */
	std::mutex vSiteMutex;

	/**
	 * \brief the std::queue of std::function<void() jobs
	 */
	std::queue<std::function<void()>> m_JobQueue;

	/**
	 * \brief the std::mutex for m_QueueMutex
	 */
	std::mutex m_QueueMutex;

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

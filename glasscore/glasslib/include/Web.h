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
	 * seconds between status checks. -1 to disable status checks.  Default 60.
	 * \param aziTaper = A double value containing the azimuth taper to be used,
	 * defaults to 360
	 * \param maxDepth = A double value containing the maximum allowable depth
	 * defaults to 800 km.
	 */
	CWeb(std::string name, double thresh, int numDetect, int numNucleate,
			int resolution, int numRows, int numCols, int numZ, bool update,
			std::shared_ptr<traveltime::CTravelTime> firstTrav,
			std::shared_ptr<traveltime::CTravelTime> secondTrav,
			int numThreads = 0, int sleepTime = 100, int checkInterval = 60,
			double aziTaper = 360., double maxDepth = 800.);

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
	 * \param aziTaper = A double value containing the azimuth taper to be used,
	 * defaults to 360
	 * \param maxDepth = A double value containing the maximum allowable depth
	 * defaults to 800 km.
	 * \return Returns true if successful, false otherwise
	 */
	bool initialize(std::string name, double thresh, int numDetect,
					int numNucleate, int resolution, int numRows, int numCols,
					int numZ, bool update,
					std::shared_ptr<traveltime::CTravelTime> firstTrav,
					std::shared_ptr<traveltime::CTravelTime> secondTrav,
					double aziTap = 360., double maxDep = 800.);

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
	 * \brief add a job
	 *
	 * Adds a job to the queue of jobs to be run by the background
	 * thread
	 * \param newjob - A std::function<void()> bound to the function
	 * containing the job to run
	 */
	void addJob(std::function<void()> newjob);

	/**
	 * \brief aziTapre Getter
	 * \return double with the azi taper start
	 */
	double getAziTaper() const;

	/**
	 * \brief max depth for locator getter
	 * \return double with the maximum allowable depth
	 */
	double getMaxDepth() const;

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

	/**
	 * \brief Update getter
	 * \return a flag indicating whether this web supports updates
	 */
	bool getUpdate() const;

	/**
	 * \brief Resolution getter
	 * \return the web resolution
	 */
	double getResolution() const;

	/**
	 * \brief Nucleation threshold getter
	 * \return the nucleation viability threshold
	 */
	double getThresh() const;

	/**
	 * \brief Col getter
	 * \return the number of columns
	 */
	int getCol() const;

	/**
	 * \brief Default number of detection stations getter
	 * \return the default number of detections used in a node
	 */
	int getDetect() const;

	/**
	 * \brief Default number of picks for nucleation getter
	 * \return the default number of nucleations used in for a detection
	 */
	int getNucleate() const;

	/**
	 * \brief Row getter
	 * \return the number of rows
	 */
	int getRow() const;

	/**
	 * \brief Z getter
	 * \return the number of depths
	 */
	int getZ() const;

	/**
	 * \brief Name getter
	 * \return the name of this web
	 */
	const std::string& getName() const;

	/**
	 * \brief Primary nucleation travel time  etter
	 * \return the primary nucleation travel time
	 */
	const std::shared_ptr<traveltime::CTravelTime>& getTrv1() const;

	/**
	 * \brief Secondary nucleation travel time  etter
	 * \return the secondary nucleation travel time
	 */
	const std::shared_ptr<traveltime::CTravelTime>& getTrv2() const;

	/**
	 * \brief Net Filter Size getter
	 * \return the number network filters
	 */
	int getVNetFilterSize() const;

	/**
	 * \brief Site Filter Size getter
	 * \return the number site filters
	 */
	int getVSitesFilterSize() const;

	/**
	 * \brief Use only teleseismic stations flag getter
	 * \return the use only teleseismic stations flag
	 */
	bool getUseOnlyTeleseismicStations() const;

	/**
	 * \brief Node size getter
	 * \return the number of nodes
	 */
	int getVNodeSize() const;

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
	std::atomic<bool> bUseOnlyTeleseismicStations;

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
	std::atomic<int> nRow;

	/**
	 * \brief A integer containing the number of columns in a detection grid
	 * generated by the Grid() command
	 */
	std::atomic<int> nCol;

	/**
	 * \brief A integer containing the number of depths in a detection grid
	 * generated by the Grid() command
	 */
	std::atomic<int> nZ;

	/**
	 * \brief A double value containing the number of closest stations to use
	 * when generating a node for this detection array. This number overrides
	 * the default Glass parameter if it is provided
	 */
	std::atomic<int> nDetect;

	/**
	 * \brief A double value containing the number picks of that need to be
	 * gathered to trigger the nucleation of an event. This number overrides the
	 * default Glass parameter if it is provided
	 */
	std::atomic<int> nNucleate;

	/**
	 * \brief A double value containing the viability threshold needed to
	 * exceed for a nucleation to be successful. This value overrides the
	 * default Glass parameter if it is provided
	 */
	std::atomic<double> dThresh;

	/**
	 * \brief A double value containing the inter-node resolution for this
	 * detection array
	 */
	std::atomic<double> dResolution;

	/**
	 * \brief A double which describes where the locator should start
	 * down weighting for azimuthal gap
	 **/
	std::atomic<double> aziTaper;

	/**
	 * \brief A double which describes the maximum allowable event depth
	 **/
	std::atomic<double> maxDepth;

	/**
	 * \brief A boolean flag that stores whether to update this web when a
	 * station has changed.
	 */
	std::atomic<bool> bUpdate;

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

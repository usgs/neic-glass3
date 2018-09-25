/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef SITE_H
#define SITE_H

#include <json.h>
#include <geo.h>
#include <set>
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <tuple>
#include <mutex>
#include <atomic>
#include "Link.h"
#include "Pick.h"

namespace glasscore {

// forward declarations
class CNode;
class CTrigger;
class CHypo;

/**
 * \brief CSite comparison function
 *
 * PickCompare contains the comparison function used by std::multiset when
 * SitePickCompare, sorting, and retrieving picks.
 */
struct SitePickCompare {
	bool operator()(const std::shared_ptr<CPick> &lhs,
					const std::shared_ptr<CPick> &rhs) const {
		if (lhs->getTSort() < rhs->getTSort()) {
			return (true);
		}
		return (false);
	}
};

/**
 * \brief glasscore site (station) class
 *
 * The CSite class is the class that encapsulates everything necessary
 * to represent a seismic station (site), including geographic location
 * identifier (SCNL), and use/nouse flag. The CSite class is also a node in the
 * detection graph database.
 *
 * CSite maintains graph database links between it and one or more detection
 * nodes
 *
 * CSite maintains a list of picks made at the site
 *
 * CSite contains function to support nucleation of a new event based
 * on a potential origin time and each of the detection nodes linked to the site.
 *
 * CPick also maintains a vector of CHypo objects represent the graph database
 * links between  this pick and various hypocenters.  A single pick may be
 * linked to multiple hypocenters
 *
 * CSite uses smart pointers (std::shared_ptr).
 */
class CSite {
 public:
	/**
	 * \brief CSite default constructor
	 *
	 * The default constructor for the CSite class.
	 * Initializes members to default values.
	 */
	CSite();

	/**
	 * \brief CSite advanced constructor
	 *
	 * An advanced constructor for the CSite class. This function
	 * initializes members to the provided SCNL, Lat/Lon/elv, qual, and
	 * use-flags values
	 *
	 * \param sta - A string containing the station name for this site.
	 * \param comp - A string containing the component code for this site.
	 * \param net - A string containing the network code for this site.
	 * \param loc - A string containing the location code for this site.
	 * \param lat - A double value containing the geographic latitude of this
	 * site in degrees
	 * \param lon - A double value containing the geographic longitude of this
	 * site in degrees
	 * \param elv - A double value containing the geographic elevation of this
	 * site in meters
	 * \param qual - A double value containing a station quality estimate
	 * \param enable - A boolean flag indicating whether the site is to be
	 * enabled or not
	 * \param useTele - A boolean flag indicating whether the site is to be used
	 * for teleseismic or not
	 */
	CSite(std::string sta, std::string comp, std::string net, std::string loc,
			double lat, double lon, double elv, double qual, bool enable,
			bool useTele);

	/**
	 * \brief CSite advanced constructor
	 *
	 * An advanced constructor for the CSite class. This function
	 * initializing members to the values parsed from the provided json object.
	 *
	 * \param site - A shared_ptr to a json::Object to construct the site from
	 */
	explicit CSite(std::shared_ptr<json::Object> site);

	/**
	 * \brief CSite destructor
	 */
	~CSite();

	/**
	 * \brief CSite clear function
	 */
	void clear();

	/**
	 * \brief CSite update function
	 *
	 * This function updates the CSite with the data contained in the provided
	 * CSite, while not modifying the data list
	 *
	 * \param site - A pointer to a CSite object containing the site to update
	 * from.
	 */
	void update(CSite *site);

	/**
	 * \brief CSite initialization function
	 *
	 * Initializes site class to provided values.
	 *
	 * \param sta - A string containing the station code for this site.
	 * \param comp - A string containing the component code for this site.
	 * \param net - A string containing the network code for this site.
	 * \param loc - A string containing the location code for this site.
	 * \param lat - A double value containing the geographic latitude of this
	 * site in degrees
	 * \param lon - A double value containing the geographic longitude of this
	 * site in degrees
	 * \param elv - A double value containing the geographic elevation of this
	 * site in meters
	 * \param qual - A double value containing a station quality estimate
	 * \param enable - A boolean flag indicating whether the site is to be
	 * enabled or not
	 * \param useTele - A boolean flag indicating whether the site is to be used
	 * for teleseismic or not
	 * \return Returns true if successful, false otherwise
	 */
	bool initialize(std::string sta, std::string comp, std::string net,
					std::string loc, double lat, double lon, double elv,
					double qual, bool enable, bool useTele);

	/**
	 * \brief Set the site's location
	 * This function sets the geographic location of the site to
	 * the provided latitude, longitude, and depth.
	 *
	 * \param lat - A double value containing the latitude to use.
	 * \param lon - A double value containing the longitude to use.
	 * \param z - A double value containing the depth to use.
	 */
	void setLocation(double lat, double lon, double z);

	/**
	 * \brief Get the distance to the site
	 * This function calculates the distance between this site and
	 * the given geographic location in radians.
	 *
	 * \param geo2 - A pointer to a CGeo object containing the geographic
	 * location
	 * \return Returns a double value containing the distance in radians
	 */
	double getDelta(glass3::util::Geo *geo2);

	/**
	 * \brief Get the distance between this site and another in km
	 *
	 * \param site - A pointer to another site
	 * \return Returns a double value containing the distance in km
	 */
	double getDistance(std::shared_ptr<CSite> site);

	/**
	 * \brief Add pick to this site
	 * This function adds the given pick to the list of picks made at this
	 * site
	 *
	 * \param pck - A shared_ptr to a CPick object containing the pick to add
	 */
	void addPick(std::shared_ptr<CPick> pck);

	/**
	 * \brief Remove pick from this site
	 * This function removes the given pick from the list of picks made at this
	 * site
	 *
	 * \param pck - A shared_ptr to a CPick object containing the pick to remove
	 */
	void removePick(std::shared_ptr<CPick> pck);

	/**
	 * \brief Get a vector of picks that fall within a time window
	 *
	 * Get a vector of picks that fall within the provided time window from t1
	 * to t2
	 *
	 * \param t1 - A double value containing the beginning of the time window in
	 * julian seconds
	 * \param t2 - A double value containing the end of the time window in
	 * julian seconds
	 * \return Return a std::vector of std::weak_ptrs to the picks within the
	 * time window
	 */
	std::vector<std::shared_ptr<CPick>> getPicks(double t1, double t2);

	/**
	 * \brief Add node to this site
	 * This function adds the given pick to the list of nodes serviced by this
	 * site
	 *
	 * \param node - A shared_ptr to a CNode object containing the node to add
	 * \param travelTime1 - A double value containing the first travel time
	 * to use
	 * \param travelTime2 - A double value containing the optional second travel
	 * time to use for the link, defaults to -1 (no travel time)
	 */
	void addNode(std::shared_ptr<CNode> node, double distDeg, double travelTime1,
					double travelTime2 = -1);

	/**
	 * \brief Remove pick from this site
	 * This function removes the given node from the list of nodes linked to this
	 *
	 * \param nodeID - A string with the id of the node to remove
	 */
	void removeNode(std::string nodeID);

	/**
	 * \brief Try to nucleate a new event at nodes linked to site
	 * This function cycles through each node linked to this site, computes
	 * the PDF at each node, and identifies the node with the best PDF
	 *
	 * The function uses addTrigger to keep track of triggering nodes
	 *
	 * \param tpick - A double value containing the pick time to nucleate with
	 * in julian seconds
	 * \return Returns a vector of shared_ptrs to the CTriggers generated by
	 * nucleate.
	 */
	std::vector<std::shared_ptr<CTrigger>> nucleate(double tpick);

	/**
	 * \brief Add triggering node to triggered node list if value exceeds
	 * current value of if named node's web is not yet present.
	 *
	 * \param vTrigger - a pointer to a vector of shared_ptr's containing the
	 * current list of triggers
	 * \param trigger - a shared_ptr to the new CTrigger to add to the list
	 */
	void addTriggerToList(std::vector<std::shared_ptr<CTrigger>> *vTrigger,
							std::shared_ptr<CTrigger> trigger);

	/**
	 * \brief Gets the number of nodes linked to this site
	 * \return Returns an integer containing the number of nodes linked to this
	 * site
	 */
	int getNodeLinksCount() const;

	/**
	 * \brief Gets whether this site is enabled. Enable represents whether
	 * this site should be used according to sources outside glasscore
	 * \return Returns a boolean flag indicating whether the site is enabled,
	 * true if enabled, false otherwise
	 */
	bool getEnable() const;

	/**
	 * \brief Sets whether this site is enabled. Enable represents whether
	 * this site should be used according to sources outside glasscore
	 * \param enable - A boolean flag indicating whether the site is enabled,
	 * true if enabled, false otherwise
	 */
	void setEnable(bool enable);

	/**
	 * \brief Gets whether this site should be used for picks and nucleation.
	 * (m_bUse && m_bEnable) Use represents whether the site should be used
	 * according to glasscore's internal metrics
	 * \return Returns a boolean flag indicating whether the site is used,
	 * true if enabled, false otherwise
	 */
	bool getUse() const;

	/**
	 * \brief Sets whether this site should be used for picks and nucleation.
	 * Use represents whether the site should be used according to glasscore's
	 * internal metrics
	 * \param use - a boolean flag indicating whether the site is used,
	 * true if enabled, false otherwise
	 */
	void setUse(bool use);

	/**
	 * \brief Gets whether this site should be used for nucleation web
	 * generation where UseOnlyTeleseismicStations is set for a web.
	 * \return Returns a boolean flag indicating whether the site is used for
	 * nucleation web generation, true if enabled, false otherwise
	 */
	bool getUseForTeleseismic() const;

	/**
	 * \brief Sets whether this site should be used for nucleation web
	 * generation where UseOnlyTeleseismicStations is set for a web.
	 * \param useForTele - a boolean flag indicating whether the site is used
	 * for nucleation web generation, true if enabled, false otherwiseg
	 */
	void setUseForTeleseismic(bool useForTele);

	/**
	 * \brief Get the station quality metric
	 * \return Returns a double value containing the station quality metric
	 */
	double getQuality() const;

	/**
	 * \brief Set the station quality metric
	 * \param qual - a double value containing the station quality metric
	 */
	void setQuality(double qual);

	/**
	 * \brief Get the combined site location (latitude, longitude, elevation) as
	 * a CGeo object
	 * \return Returns a glass3::util::Geo object containing the combined location.
	 */
	glass3::util::Geo &getGeo();

	/**
	 * \brief Get the SCNL identifier for this site
	 * \return Returns a std::string containing the SCNL identifier for this
	 * site
	 */
	const std::string& getSCNL() const;

	/**
	 * \brief Get the Site (station) name for this site
	 * \return Returns a std::string containing the station name for this site
	 */
	const std::string& getSite() const;

	/**
	 * \brief Get the component code for this site
	 * \return Returns a std::string containing the component code for this site
	 */
	const std::string& getComponent() const;

	/**
	 * \brief Get the network code for this site
	 * \return Returns a std::string containing the network code for this site
	 */
	const std::string& getNetwork() const;

	/**
	 * \brief Get the location code for this site
	 * \return Returns a std::string containing the location code for this site
	 */
	const std::string& getLocation() const;

	/**
	 * \brief Get the time the last pick was added to this site
	 * \return Returns a time_t containing the time that the last pick was
	 * added to this site in epoch seconds
	 */
	time_t getTLastPickAdded() const;

	/**
	 * \brief Get the location unit vectors
	 * \return Returns a double pointer containing the location unit vectors in
	 * Cartesian earth coordinates
	 */
	double * getUnitVectors(double * vec);

	/**
	 * \brief Get the count of picks added to this site since the last check
	 * \return Returns an integer containing the count of picks added to this
	 * site since the last check
	 */
	int getPickCountSinceCheck() const;

	/**
	 * \brief Set the count of picks added to this site since the last check
	 * \param count - an integer containing the count of picks added to this
	 * site since the last check
	 */
	void setPickCountSinceCheck(int count);

	/**
	 * \brief Gets the number of picks made at this site
	 * \return Returns an integer containing the number of picks made at this
	 * site
	 */
	int getPickCount() const;

	/**
	 * \brief Gets the lower bound of the pick multiset at the provided time
	 * \param min - a double value containing the desired time to generate
	 * the lower value at in julian seconds
	 * \return Returns an interator pointing to the first pick in the multiset
	 * greater than the provided time
	 */
	std::multiset<std::shared_ptr<CPick>, SitePickCompare>::iterator getLower(
			double min);

	/**
	 * \brief Gets the upper bound of the pick multiset at the provided time
	 * \param max - a double value containing the desired time to generate
	 * the upper value at in julian seconds
	 * \return Returns an interator pointing to the first pick in the multiset
	 * less than the provided time
	 */
	std::multiset<std::shared_ptr<CPick>, SitePickCompare>::iterator getUpper(
			double max);

	/**
	 * \brief Gets the end iterator of the pick multiset
	 * \return Returns an interator pointing to end of the multiset i.e. end()
	 */
	std::multiset<std::shared_ptr<CPick>, SitePickCompare>::iterator getEnd();

	/**
	 * \brief Gets pick multiset mutex
	 * \return Returns the std::mutex protecting the pick multiset
	 */
	std::mutex & getPickMutex();

	/**
	 * \brief A PickList function that updates the position of the given pick
	 * in the multiset
	 * \param pick - A shared_ptr to the pick that needs a position update
	 */
	void updatePosition(std::shared_ptr<CPick> pick);

 private:
	/**
	 * \brief A PickList function that removes the given pick from the multiset
	 * \param pick - A shared_ptr to the pick to be removed
	 */
	void eraseFromMultiset(std::shared_ptr<CPick> pick);

	/**
	 * \brief A mutex to control threading access to vPick.
	 */
	mutable std::mutex vPickMutex;

	/**
	 * \brief A std::multiset containing each pick made at this site in sequential
	 * time order from oldest to youngest.
	 */
	std::multiset<std::shared_ptr<CPick>, SitePickCompare> m_msPickList;

	/**
	 * \brief A std::string containing the SCNL (Site, Component, Network,
	 * Location) identifier for this site.
	 */
	std::string m_sSCNL;

	/**
	 * \brief A std::string containing the Site (station) name for this site.
	 */
	std::string m_sSite;

	/**
	 * \brief A std::string containing the Component code for this site.
	 */
	std::string m_sComponent;

	/**
	 * \brief A std::string containing the Network code for this site.
	 */
	std::string m_sNetwork;

	/**
	 * \brief A std::string containing the Location code for this site.
	 */
	std::string m_sLocation;

	/**
	 * \brief A CGeo object containing the geographic location of this site
	 */
	glass3::util::Geo m_Geo;

	/**
	 * \brief A unit vector in Cartesian earth coordinates used to do a quick
	 * and dirty distance calculation during detection grid formation
	 */
	double m_daUnitVectors[3];

	/**
	 * \brief A boolean flag indicating whether this site is disabled external
	 * to glass. This is different than bUse, which is managed by glass
	 * processes.
	 */
	std::atomic<bool> m_bEnable;

	/**
	 * \brief A boolean flag indicating whether to use this site in calculations.
	 */
	std::atomic<bool> m_bUse;

	/**
	 * \brief A boolean flag indicating whether to use this site for teleseismic
	 * calculations.
	 */
	std::atomic<bool> m_bUseForTeleseismic;

	/**
	 * \brief A double value containing the quality estimate of the station.
	 */
	std::atomic<double> m_dQuality;

	/**
	 * \brief An integer containing the number of picks made at this site since
	 * the last check
	 */
	std::atomic<int> m_iPickCountSinceCheck;

	/**
	 * \brief A mutex to control threading access to vNode.
	 */
	mutable std::mutex m_vNodeMutex;

	/**
	 * \brief A std::vector of tuples linking site to node
	 * {shared node pointer, travel-time 1, travel-time 2}
	 */
	std::vector<NodeLink> m_vNode;

	/**
	 * \brief A recursive_mutex to control threading access to CSite.
	 * NOTE: recursive mutexes are frowned upon, so maybe redesign around it
	 * see: http://www.codingstandard.com/rule/18-3-3-do-not-use-stdrecursive_mutex/
	 * However a recursive_mutex allows us to maintain the original class
	 * design as delivered by the contractor.
	 */
	mutable std::recursive_mutex m_SiteMutex;

	/**
	 * \brief A double value containing the last time a pick was added to this
	 * site in epoch seconds
	 */
	std::atomic<double> m_tLastPickAdded;

	/**
	 * \brief A shared_ptr to a pick used to represent the lower value in
	 * getPicks() calls
	 */
	std::shared_ptr<CPick> m_LowerValue;

	/**
	 * \brief A shared_ptr to a pick used to represent the upper value in
	 * getPicks() calls
	 */
	std::shared_ptr<CPick> m_UpperValue;
};
}  // namespace glasscore
#endif  // SITE_H

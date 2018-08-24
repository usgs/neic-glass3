/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef SITE_H
#define SITE_H

#include <json.h>
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <tuple>
#include <mutex>
#include "Geo.h"
#include "Link.h"

namespace glasscore {

// forward declarations
class CPick;
class CNode;
class CGlass;
class CTrigger;
class CHypo;

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
	 * \brief CSite constructor
	 */
	CSite();

	/**
	 * \brief CSite alternate constructor
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
	 * \param glassPtr - A pointer to the CGlass class
	 */
	CSite(std::string sta, std::string comp, std::string net, std::string loc,
			double lat, double lon, double elv, double qual, bool enable,
			bool useTele, CGlass *glassPtr);

	/**
	 * \brief CSite alternate constructor
	 *
	 * \param site - A pointer to a json::Object to construct the site from
	 * \param glassPtr - A pointer to the CGlass class
	 */
	CSite(std::shared_ptr<json::Object> site, CGlass *glassPtr);

	/**
	 * \brief CSite destructor
	 */
	~CSite();

	/**
	 * \brief CSite clear function
	 */
	void clear();

	void clearVPick();

	/**
	 * \brief CSite update function
	 *
	 * \param site - A pointer to a CSite object containing the site to update
	 * from.
	 */
	void update(CSite *site);

	/**
	 * \brief CSite initialization function
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
	 * \param glassPtr - A pointer to the CGlass class
	 * \return Returns true if successful, false otherwise
	 */
	bool initialize(std::string sta, std::string comp, std::string net,
					std::string loc, double lat, double lon, double elv,
					double qual, bool enable, bool useTele, CGlass *glassPtr);

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
	double getDelta(glassutil::CGeo *geo2);

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
	void remPick(std::shared_ptr<CPick> pck);

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
	void addNode(std::shared_ptr<CNode> node, double travelTime1,
					double travelTime2 = -1);

	/**
	 * \brief Remove pick from this site
	 * This function removes the given node from the list of nodes linked to this
	 *
	 * \param nodeID - A string with the id of the node to remove
	 */
	void remNode(std::string nodeID);

	/**
	 * \brief Try to nucleate a new event at nodes linked to site
	 * This function cycles through each node linked to this site, computes
	 * the PDF at each node, and identifies the node with the best PDF
	 *
	 * The function uses addTrigger to keep track of triggering nodes
	 *
	 * \param tpick - A double value containing the pick time to nucleate with
	 * in julian seconds
	 */
	std::vector<std::shared_ptr<CTrigger>> nucleate(double tpick);

	/**
	 * \brief Add triggering node to triggered node list if value exceeds
	 * current value of if named node's web is not yet present.
	 */
	void addTrigger(std::vector<std::shared_ptr<CTrigger>> *vTrigger,
					std::shared_ptr<CTrigger> trigger);

	/**
	 * \brief Node link count getter
	 * \return the node link count
	 */
	int getNodeLinksCount() const;

	bool getEnable() const;

	void setEnable(bool enable);

	/**
	 * \brief Use flag getter
	 * \return the use flag
	 */
	bool getUse() const;

	/**
	 * \brief Use flag setter
	 * \param use - the use flag
	 */
	void setUse(bool use);

	/**
	 * \brief Use for teleseismic flag getter
	 * \return the use for teleseismic flag
	 */
	bool getUseForTele() const;

	/**
	 * \brief Use for teleseismic flag setter
	 * \param useForTele - the use for teleseismic flag
	 */
	void setUseForTele(bool useForTele);

	/**
	 * \brief Quality getter
	 * \return the quality
	 */
	double getQual() const;

	/**
	 * \brief Quality setter
	 * \param qual - the quality
	 */
	void setQual(double qual);

	/**
	 * \brief CGeo getter
	 * \return the CGeo
	 */
	glassutil::CGeo &getGeo();

	/**
	 * \brief Max picks for site getter
	 * \return the max picks for site
	 */
	int getSitePickMax() const;

	/**
	 * \brief CGlass getter
	 * \return the CGlass pointer
	 */
	CGlass* getGlass() const;

	/**
	 * \brief SCNL getter
	 * \return the SCNL
	 */
	const std::string& getScnl() const;

	/**
	 * \brief Site getter
	 * \return the site
	 */
	const std::string& getSite() const;

	/**
	 * \brief Comp getter
	 * \return the comp
	 */
	const std::string& getComp() const;

	/**
	 * \brief Net getter
	 * \return the net
	 */
	const std::string& getNet() const;

	/**
	 * \brief Loc getter
	 * \return the loc
	 */
	const std::string& getLoc() const;

	/**
	 * \brief vPick getter
	 * \return the vPick
	 */
	const std::vector<std::shared_ptr<CPick>> getVPick() const;

	time_t getTLastPickAdded() const;

	double * getVec(double * vec);

	void setPicksSinceCheck(int count);

	int getPicksSinceCheck() const;

 private:
	/**
	 * \brief A mutex to control threading access to vPick.
	 */
	mutable std::mutex vPickMutex;

	/**
	 * \brief A std::vector of std::shared_ptr to the picks made at this this
	 * CSite. A shared_ptr is used here instead of a weak_ptr (to prevent a
	 * cyclical reference between CPick and CSite) to improve performance
	 */
	std::vector<std::shared_ptr<CPick>> vPick;

	/**
	 * \brief A pointer to the main CGlass class used encode/decode time and
	 * get debugging flags
	 */
	CGlass *pGlass;

	/**
	 * \brief A std::string containing the SCNL (Site, Component, Network,
	 * Location) for this site.
	 */
	std::string sScnl;

	/**
	 * \brief A std::string containing the Site (station) name for this site.
	 */
	std::string sSite;

	/**
	 * \brief A std::string containing the Component name for this site.
	 */
	std::string sComp;

	/**
	 * \brief A std::string containing the Network name for this site.
	 */
	std::string sNet;

	/**
	 * \brief A std::string containing the Location code for this site.
	 */
	std::string sLoc;

	/**
	 * \brief A CGeo object containing the geographic location of this site
	 */
	glassutil::CGeo geo;

	/**
	 * \brief A unit vector in Cartesian earth coordinates used to do a quick
	 * and dirty distance calculation during detection grid formation
	 */
	double dVec[3];

	/**
	 * \brief A boolean flag indicating whether this site is disabled external
	 * to glass. This is different than bUse, which is managed by glass
	 * processes.
	 */
	std::atomic<bool> bEnable;

	/**
	 * \brief A boolean flag indicating whether to use this site in calculations.
	 */
	std::atomic<bool> bUse;

	/**
	 * \brief A boolean flag indicating whether to use this site for teleseismic
	 * calculations.
	 */
	std::atomic<bool> bUseForTele;

	/**
	 * \brief A double value containing the quality estimate of the station.
	 */
	std::atomic<double> dQual;

	/**
	 * \brief An integer containing the maximum number of picks stored by
	 * the vector in this site
	 */
	std::atomic<int> nSitePickMax;

	/**
	 * \brief An integer containing the number of picks made at this site since
	 * the last check
	 */
	std::atomic<int> nPicksSinceCheck;

	/**
	 * \brief A mutex to control threading access to vNode.
	 */
	mutable std::mutex vNodeMutex;

	/**
	 * \brief A std::vector of tuples linking site to node
	 * {shared node pointer, travel-time 1, travel-time 2}
	 */
	std::vector<NodeLink> vNode;

	/**
	 * \brief A recursive_mutex to control threading access to CSite.
	 * NOTE: recursive mutexes are frowned upon, so maybe redesign around it
	 * see: http://www.codingstandard.com/rule/18-3-3-do-not-use-stdrecursive_mutex/
	 * However a recursive_mutex allows us to maintain the original class
	 * design as delivered by the contractor.
	 */
	mutable std::recursive_mutex siteMutex;

	time_t tLastPickAdded;
};
}  // namespace glasscore
#endif  // SITE_H

/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef PICK_H
#define PICK_H

#include <json.h>
#include <memory>
#include <string>
#include <vector>
#include <mutex>

namespace glasscore {

// forward declarations
class CSite;
class CHypo;
class CSiteList;

/**
 * \brief glasscore pick class
 *
 * The CPick class is the class that encapsulates everything necessary
 * to represent a waveform arrival pick, including arrival time, phase id,
 * and an unique identifier.  The CPick class is also a node in the
 * detection graph database.
 *
 * CPick contains functions to support nucleation of a new event based
 * on the pick.
 *
 * CPick maintains a graph database link between it and the the site (station)
 * the pick was made at.
 *
 * CPick also maintains a vector of CHypo objects represent the graph database
 * links between  this pick and various hypocenters.  A single pick may be
 * linked to multiple hypocenters
 *
 * CPick uses smart pointers (std::shared_ptr).
 */
class CPick {
 public:
	/**
	 * \brief CPick constructor
	 *
	 * Constructs an empty CPick
	 */
	CPick();

	/**
	 * \brief CPick alternate constructor
	 *
	 * Constructs a CPick using the provided values
	 *
	 * \param pickSite - A shared pointer to a CSite object that the pick was
	 * made at
	 * \param pickTime - A double containing the pick arrival time
	 * \param pickId - An integer containing the glass pick id (index) to use.
	 * \param pickIdString - A std::string containing the external pick id.
	 * \param backAzimuth - A double containing the optional back azimuth, -1
	 * to omit
	 * \param slowness - A double containing the optional slowness, -1 to omit
	 */
	CPick(std::shared_ptr<CSite> pickSite, double pickTime, int pickId,
			std::string pickIdString, double backAzimuth, double slowness);

	/**
	 * \brief CPick alternate constructor
	 *
	 * Constructs a CPick class from the provided json object and id, using
	 * a CGlass pointer to convert times and lookup stations.
	 *
	 * \param pick - A pointer to a json::Object to construct the pick from
	 * \param pickId - An integer containing the pick id to use.
	 * \param pSiteList - A pointer to the CSiteList class
	 */
	CPick(std::shared_ptr<json::Object> pick, int pickId, CSiteList *pSiteList);

	/**
	 * \brief CPick destructor
	 */
	~CPick();

	/**
	 * \brief CPick clear function
	 */
	void clear();

	/**
	 * \brief CPick initialization function
	 *
	 * Initializes pick class to provided values.
	 *
	 * \param pickSite - A shared pointer to a CSite object that the pick was
	 * made at
	 * \param pickTime - A double containing the pick arrival time
	 * \param pickId - An integer containing the glass pick id (index) to use.
	 * \param pickIdString - A std::string containing the external pick id.
	 * \param backAzimuth - A double containing the optional back azimuth, -1
	 * to omit
	 * \param slowness - A double containing the optional slowness, -1 to omit
	 * \return Returns true if successful, false otherwise.
	 */
	bool initialize(std::shared_ptr<CSite> pickSite, double pickTime,
					int pickId, std::string pickIdString, double backAzimuth,
					double slowness);

	/**
	 * \brief Add hypo reference to this pick
	 *
	 * Adds a shared_ptr reference to the given hypo to this pick,
	 * representing a graph database link between this pick and the hypocenters.
	 *
	 * Note that this pick may or may not also be linked
	 * to other hypocenters
	 *
	 * \param hyp - A std::shared_ptr to an object containing the hypocenter
	 * to link.
	 */
	void addHypo(std::shared_ptr<CHypo> hyp, std::string ass = "", bool force =
							false);

	/**
	 * \brief Remove hypo reference to this pick
	 *
	 * Remove a shared_ptr reference from the given hypo to this pick,
	 * breaking the graph database link between this pick and the hypocenter.
	 *
	 * Note that this pick may or may not be still linked
	 * to other hypocenters
	 *
	 * \param hyp - A std::shared_ptr to an object containing the hypocenter
	 * to unlink.
	 */
	void remHypo(std::shared_ptr<CHypo> hyp);

	/**
	 * \brief Clear any hypo reference to this pick
	 */
	void clearHypo();

	/**
	 * \brief Nucleate new event based on the addition of this pick
	 *
	 * Attempt to nucleate a new event based on the addition of this
	 * pick. First scan all nodes linked to this pick's site. Then, for each
	 * one, calculate the stacked agoric at that node. Then, for each node who's
	 * agoric surpassed the threhold, try to generate a new hypocenter. If that
	 * hypocenter survives, add it to the list.
	 */
	bool nucleate();

	/**
	 * \brief Back azimuth getter
	 * \return the back azimuth
	 */
	double getBackAzimuth() const;

	/**
	 * \brief Slowness getter
	 * \return the slowness
	 */
	double getSlowness() const;

	/**
	 * \brief Pick id getter
	 * \return the pick id
	 */
	int getIdPick() const;

	/**
	 * \brief Json pick getter
	 * \return the json pick
	 */
	const std::shared_ptr<json::Object>& getJPick() const;

	/**
	 * \brief Hypo getter
	 * \return the hypo
	 */
	const std::shared_ptr<CHypo> getHypo() const;

	/**
	 * \brief Site getter
	 * \return the site
	 */
	const std::shared_ptr<CSite>& getSite() const;

	/**
	 * \brief Association string getter
	 * \return the association string
	 */
	const std::string& getAss() const;

	/**
	 * \brief Association string setter
	 * \param ass - the association string
	 */
	void setAss(std::string ass);

	/**
	 * \brief Phase getter
	 * \return the phase
	 */
	const std::string& getPhs() const;

	/**
	 * \brief Pid getter
	 * \return the pid
	 */
	const std::string& getPid() const;

	/**
	 * \brief Pick time getter
	 * \return the pick time
	 */
	double getTPick() const;

 private:
	/**
	 * \brief A std::shared_ptr to a CSite object
	 * representing the link between this pick and the site it was
	 * picked at
	 */
	std::shared_ptr<CSite> pSite;

	/**
	 * \brief A std::weak_ptr to a CHypo object
	 * representing the links between this pick and associated hypocenter
	 */
	std::weak_ptr<CHypo> wpHypo;

	/**
	 * \brief A std::string containing a character representing the action
	 * that caused this pick to be associated
	 */
	std::string sAss;

	/**
	 * \brief A std::string containing the phase name of this pick
	 */
	std::string sPhs;

	/**
	 * \brief A std::string containing the string unique id of this pick
	 */
	std::string sPid;

	/**
	 * \brief A double value containing the back azimuth of the pick
	 */
	double dBackAzimuth;

	/**
	 * \brief A double value containing the slowness of the pick
	 */
	double dSlowness;

	/**
	 * \brief A double value containing the arrival time of the pick
	 */
	double tPick;

	/**
	 * \brief An integer value containing the numeric id of the pick
	 */
	int idPick;

	/**
	 * \brief A std::shared_ptr to a json object
	 * representing the original pick input, used in accessing information
	 * not relevant to glass that are needed for generating outputs.
	 */
	std::shared_ptr<json::Object> jPick;

	/**
	 * \brief A recursive_mutex to control threading access to CPick.
	 * NOTE: recursive mutexes are frowned upon, so maybe redesign around it
	 * see: http://www.codingstandard.com/rule/18-3-3-do-not-use-stdrecursive_mutex/
	 * However a recursive_mutex allows us to maintain the original class
	 * design as delivered by the contractor.
	 */
	mutable std::recursive_mutex pickMutex;
};
}  // namespace glasscore
#endif  // PICK_H

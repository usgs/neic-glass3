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
#include <atomic>

namespace glasscore {

// forward declarations
class CSite;
class CHypo;
class CSiteList;
class CPickList;

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
	 * \brief CPick default constructor
	 *
	 * The default constructor for the CPick class.
	 * Initializes members to default values.
	 */
	CPick();

	/**
	 * \brief CPick advanced constructor
	 *
	 * An advanced constructor for the CPick class. This function
	 * initializes members to the provided values.
	 *
	 * \param pickSite - A shared pointer to a CSite object that the pick was
	 * made at
	 * \param pickTime - A double containing the pick arrival time
	 * \param pickIdString - A std::string containing the external pick id.
	 * \param backAzimuth - A double containing the optional back azimuth, 
	 * std::numeric_limits<double>::quiet_NaN() to omit
	 * \param slowness - A double containing the optional slowness, 
	 * std::numeric_limits<double>::quiet_NaN() to omit
	 */
	CPick(std::shared_ptr<CSite> pickSite, double pickTime,
			std::string pickIdString, double backAzimuth, double slowness);

	/**
	 * \brief CPick advanced constructor
	 *
	 * An advanced constructor for the CPick class. This function
	 * initializes members to the provided values.
	 *
	 * \param pickSite - A shared pointer to a CSite object that the pick was
	 * made at
	 * \param pickTime - A double containing the pick arrival time
	 * \param pickIdString - A std::string containing the external pick id.
	 * \param backAzimuth - A double containing the optional back azimuth, 
	 * std::numeric_limits<double>::quiet_NaN() to omit
	 * \param slowness - A double containing the optional slowness, 
	 * std::numeric_limits<double>::quiet_NaN() to omit
	 * \param phase - A std::string containing the optional classified phase,
	 * empty string to omit.
	 * \param phaseProb - A double containing the optional classified phase 
	 * probability,  std::numeric_limits<double>::quiet_NaN() to omit
	 * \param distance - A double containing the optional classified distance, 
	 * std::numeric_limits<double>::quiet_NaN() to omit
	 * \param distanceProb - A double containing the optional classified distance 
	 * probability,  std::numeric_limits<double>::quiet_NaN() to omit
	 * \param azimuth - A double containing the optional classified azimuth, 
	 * std::numeric_limits<double>::quiet_NaN() to omit
	 * \param azimuthProb - A double containing the optional classified azimuth 
	 * probability,  std::numeric_limits<double>::quiet_NaN() to omit
	 * \param depth - A double containing the optional classified depth, 
	 * std::numeric_limits<double>::quiet_NaN() to omit
	 * \param depthProb - A double containing the optional classified depth 
	 * probability,  std::numeric_limits<double>::quiet_NaN() to omit
	 * \param magnitude - A double containing the optional classified magnitude, 
	 * std::numeric_limits<double>::quiet_NaN() to omit
	 * \param magnitudeProb - A double containing the optional classified 
	 * magnitude probability, std::numeric_limits<double>::quiet_NaN() to omit
	 */
	CPick(std::shared_ptr<CSite> pickSite, double pickTime,
			std::string pickIdString, double backAzimuth, double slowness,
			std::string phase, double phaseProb, double distance,
			double distanceProb, double azimuth, double azimuthProb,
			double depth, double depthProb, double magnitude,
			double magnitudeProb);

	/**
	 * \brief CPick advanced constructor
	 *
	 * An advanced constructor for the CPick class. This function
	 * initializes members to the values parsed from the provided json object
	 * and using the provided pointer to a CSiteList class to lookup the pick
	 * station.
	 *
	 * \param pick - A shared pointer to a json::Object to containing the
	 * data to construct the pick from
	 * \param pSiteList - A pointer to the CSiteList class to use when looking
	 * up the pick station
	 */
	CPick(std::shared_ptr<json::Object> pick, CSiteList *pSiteList);

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
	 * \param pickIdString - A std::string containing the external pick id.
	 * \param backAzimuth - A double containing the optional back azimuth, 
	 * std::numeric_limits<double>::quiet_NaN() to omit
	 * \param slowness - A double containing the optional slowness, 
	 * std::numeric_limits<double>::quiet_NaN() to omit
	 * \param phase - A std::string containing the optional classified phase,
	 * empty string to omit.
	 * \param phaseProb - A double containing the optional classified phase 
	 * probability,  std::numeric_limits<double>::quiet_NaN() to omit
	 * \param distance - A double containing the optional classified distance, 
	 * std::numeric_limits<double>::quiet_NaN() to omit
	 * \param distanceProb - A double containing the optional classified distance 
	 * probability,  std::numeric_limits<double>::quiet_NaN() to omit
	 * \param azimuth - A double containing the optional classified azimuth, 
	 * std::numeric_limits<double>::quiet_NaN() to omit
	 * \param azimuthProb - A double containing the optional classified azimuth 
	 * probability,  std::numeric_limits<double>::quiet_NaN() to omit
	 * \param depth - A double containing the optional classified depth, 
	 * std::numeric_limits<double>::quiet_NaN() to omit
	 * \param depthProb - A double containing the optional classified depth 
	 * probability,  std::numeric_limits<double>::quiet_NaN() to omit
	 * \param magnitude - A double containing the optional classified magnitude, 
	 * std::numeric_limits<double>::quiet_NaN() to omit
	 * \param magnitudeProb - A double containing the optional classified 
	 * magnitude probability, std::numeric_limits<double>::quiet_NaN() to omit
	 * \return Returns true if successful, false otherwise.
	 */
	bool initialize(std::shared_ptr<CSite> pickSite, double pickTime,
					std::string pickIdString, double backAzimuth,
					double slowness, std::string phase, double phaseProb,
					double distance, double distanceProb, double azimuth,
					double azimuthProb, double depth, double depthProb,
					double magnitude, double magnitudeProb);

	/**
	 * \brief Add hypo reference to this pick
	 *
	 * Adds a weak_ptr reference to the given hypo to this pick, representing a
	 * graph database link between this pick and a hypocenter. If the pick is
	 * already linked to a hypocenter, the new link will be ignored unless force
	 * is set to true.
	 *
	 * Note that this pick may or may not also be included in other hypocenter
	 * pick data lists, but this pick will only link to a single
	 * hypocenter
	 *
	 * \param hyp - A std::shared_ptr to an object containing the hypocenter
	 * to link.
	 * \param force - A boolean flag indicating whether to force the association,
	 * defaults to false.
	 */
	void addHypoReference(std::shared_ptr<CHypo> hyp, bool force = false);

	/**
	 * \brief Remove hypo reference to this pick
	 *
	 * Remove a weak_ptr reference to the given hypo from this pick, breaking the
	 * graph database link between this pick and the hypocenter.
	 *
	 * Note that this pick may or may not still be included in other hypocenter
	 * pick data lists.
	 *
	 * \param hyp - A std::shared_ptr to an object containing the hypocenter
	 * to unlink.
	 */
	void removeHypoReference(std::shared_ptr<CHypo> hyp);

	/**
	 * \brief Remove hypo specific reference to this pick by id
	 *
	 * Remove a weak_ptr reference to the given hypo id from this pick, breaking
	 * the graph database link between this pick and the hypocenter.
	 *
	 * Note that this pick may or may not still be included in other hypocenter
	 * pick data lists.
	 *
	 * \param pid - A std::string identifying the the hypocenter to unlink.
	 */
	void removeHypoReference(std::string pid);

	/**
	 * \brief Attempt to nucleate new hypo based on the addition of this pick
	 *
	 * Attempt to nucleate a new hypo based on the addition of this pick. By
	 * scanning all nodes linked to this pick's site, producing a list of
	 * triggers. Then, for the best trigger for each web, try to generate a new
	 * hypo, performing a fast location/prune using hypo->anneal
	 * \param parent - A pointer to the parent CPickList thread to allow for
	 * thread status updates
	 * \return Returns true if successful, false otherwise
	 */
	bool nucleate(CPickList* parent);

	/**
	 * \brief Get the optional back azimuth related to this pick
	 * \return Returns a double value containing the optional back azimuth, or
	 * std::numeric_limits<double>::quiet_NaN() if no back azimuth was provided
	 */
	double getBackAzimuth() const;

	/**
	 * \brief Get the optional slowness related to this pick
	 * \return Returns a double value containing the optional slowness, or
	 * std::numeric_limits<double>::quiet_NaN() if no slowness was provided
	 */
	double getSlowness() const;

	/**
	 * \brief Get input JSON pick message
	 * \return Return a shared_ptr to a json::Object containing the pick message
	 */
	const std::shared_ptr<json::Object>& getJSONPick() const;

	/**
	 * \brief Get the current hypo referenced by this pick
	 *
	 * Note that this pick may or may not also be included in other hypocenter
	 * pick data lists, but this pick will only link to a single hypocenter
	 *
	 * \return Return a shared_ptr to the CHypo referenced by this pick, or NULL
	 * if no hypo is referenced
	 */
	const std::shared_ptr<CHypo> getHypoReference() const;

	/**
	 * \brief Get the site for this pick
	 * \return Return a shared_ptr to a CSite object containing the site this
	 * pick was made at
	 */
	const std::shared_ptr<CSite> getSite() const;

	/**
	 * \brief Get the phase name for this pick
	 * \return Return a std::string containing the pick phase name
	 */
	const std::string& getPhaseName() const;

	/**
	 * \brief Get the ID of this pick
	 * \return Return a std::string containing the pick ID
	 */
	const std::string& getID() const;

	/**
	 * \brief Get the arrival time for this pick
	 * \return Return a double containing the pick arrival time
	 */
	double getTPick() const;

	/**
	 * \brief Set the arrival time for this pick
	 * \param tPick -  a double containing the pick arrival time
	 */
	void setTPick(double tPick);

	/**
	 * \brief Get the sorting time for this pick
	 * \return Returns an double containing the pick sort time in Gregorian
	 * seconds
	 */
	double getTSort() const;

	/**
	 * \brief Set the sorting time for this pick
	 * \param newTSort - a double containing the pick sort time in Gregorian
	 * seconds
	 */
	void setTSort(double newTSort);

	/**
	 * \brief Get the insertion time for this pick
	 * \return Returns a double containing the pick insertion time into Glass
	 * in Gregorian seconds
	 */
	double getTInsertion() const;

	/**
	 * \brief Get the first assoc time for this pick
	 * \return Returns a double containing the time in Gregorian seconds when
	 * the pick was first associated with an event.
	 */
	double getTFirstAssociation() const;

	/**
	 * \brief Get the nucleation time for this pick
	 * \return Returns a double containing the time in Gregorian seconds when
	 * this pick was used as keystone for nucleation
	 */
	double getTNucleation() const;

	/**
	 * \brief Get the classified phase for this pick
	 * \return Return a std::string containing the pick phase
	 */
	const std::string& getClassifiedPhase() const;

	/**
	 * \brief Get the classified phase probability for this pick
	 * \return Returns a double containing the pick classified phase probability 
	 */
	double getClassifiedPhaseProbability() const;

	/**
	 * \brief Get the classified distance for this pick
	 * \return Returns a double containing the pick classified distance 
	 */
	double getClassifiedDistance() const;

	/**
	 * \brief Get the classified distance probability for this pick
	 * \return Returns a double containing the pick classified distance 
	 * probability 
	 */
	double getClassifiedDistanceProbability() const;

	/**
	 * \brief Get the classified azimuth for this pick
	 * \return Returns a double containing the pick classified azimuth 
	 */
	double getClassifiedAzimuth() const;

	/**
	 * \brief Get the classified azimuth probability for this pick
	 * \return Returns a double containing the pick classified azimuth 
	 * probability 
	 */
	double getClassifiedAzimuthProbability() const;

	/**
	 * \brief Get the classified depth for this pick
	 * \return Returns a double containing the pick classified depth 
	 */
	double getClassifiedDepth() const;

	/**
	 * \brief Get the classified depth probability for this pick
	 * \return Returns a double containing the pick classified depth 
	 * probability 
	 */
	double getClassifiedDepthProbability() const;

	/**
	 * \brief Get the classified magnitude for this pick
	 * \return Returns a double containing the pick classified magnitude 
	 */
	double getClassifiedMagnitude() const;

	/**
	 * \brief Get the classified magnitude probability for this pick
	 * \return Returns a double containing the pick classified magnitude 
	 * probability 
	 */
	double getClassifiedMagnitudeProbability() const;

 protected:
	/**
	 * \brief Remove hypo reference to this pick
	 *
	 * Remove any existing weak_ptr reference to a hypo from this pick,
	 * breaking the graph database link between this pick and a hypocenter.
	 *
	 * Note that this pick may or may not still be included in other hypocenter
	 * pick data lists.
	 */
	void clearHypoReference();

	/**
	 * \brief Set the first-assoc time for this pick to Now
	 */
	void setTFirstAssociation();

	/**
	 * \brief Set the nucleation time for this pick to Now
	 */
	void setTNucleation();

 private:
	/**
	 * \brief A std::weak_ptr to a CSite object
	 * representing the link between this pick and the site it was
	 * picked at. A weak_ptr is used here instead of a shared_ptr to prevent
	 * a cyclical reference between CPick and CSite. The weak_ptr is here
	 * instead of in site due to performance reasons.
	 */
	std::weak_ptr<CSite> m_wpSite;

	/**
	 * \brief A std::weak_ptr to a CHypo object
	 * representing the links between this pick and associated hypocenter.
	 * A weak_ptr is used here instead of a shared_ptr to prevent a cyclical
	 * reference between CPick and CHypo.
	 */
	std::weak_ptr<CHypo> m_wpHypo;

	/**
	 * \brief A std::string containing the phase name of this pick
	 */
	std::string m_sPhaseName;

	/**
	 * \brief A std::string containing the string unique id of this pick
	 */
	std::string m_sID;

	/**
	 * \brief A double value containing the back azimuth of the pick
	 */
	std::atomic<double> m_dBackAzimuth;

	/**
	 * \brief A double value containing the slowness of the pick
	 */
	std::atomic<double> m_dSlowness;

	/**
	 * \brief A double value containing the arrival time of the pick
	 */
	std::atomic<double> m_tPick;

	/**
	 * \brief A string containing the classified phase for the pick
	 */
	std::string m_sClassifiedPhase;

	/**
	 * \brief A double value containing the probability of the classified 
	 * phase
	 */
	std::atomic<double> m_dClassifiedPhaseProbability;

	/**
	 * \brief A double value containing the classified distance for the pick
	 */
	std::atomic<double> m_dClassifiedDistance;

	/**
	 * \brief A double value containing the probability of the classified 
	 * distance
	 */
	std::atomic<double> m_dClassifiedDistanceProbability;

	/**
	 * \brief A double value containing the classified azimuth for the pick
	 */
	std::atomic<double> m_dClassifiedAzimuth;

	/**
	 * \brief A double value containing the probability of the classified 
	 * azimuth
	 */
	std::atomic<double> m_dClassifiedAzimuthProbability;

	/**
	 * \brief A double value containing the classified depth for the pick
	 */
	std::atomic<double> m_dClassifiedDepth;

	/**
	 * \brief A double value containing the probability of the classified 
	 * depth
	 */
	std::atomic<double> m_dClassifiedDepthProbability;

	/**
	 * \brief A double value containing the classified magnitude for the pick
	 */
	std::atomic<double> m_dClassifiedMagnitude;

	/**
	 * \brief A double value containing the probability of the classified 
	 * magnitude
	 */
	std::atomic<double> m_dClassifiedMagnitudeProbability;

	/**
	 * \brief A std::shared_ptr to a json object
	 * representing the original pick input, used in accessing information
	 * not relevant to glass that are needed for generating outputs.
	 */
	std::shared_ptr<json::Object> m_JSONPick;

	/**
	 * \brief A double value containing this pick's sort time in Gregorian
	 * seconds, this is a cached copy of tPick that is  guaranteed to not change
	 * during the lifetime of the pick in a PickList's internal multiset,
	 * ensuring that sort order won't change, even when tPick changes because of
	 * an update. Resorting is accomplished by removing the pick from the
	 * internal multiset (NOT the PickList), updating tSort to equal the current
	 * tPick, and then reinserting the pick into the internal multiset.
	 * /see CPickList.
	 */
	std::atomic<double> m_tSort;

	/**
	 * \brief A double value containing the time this pick was created in Glass3.
	 */
	std::atomic<double> m_tInsertion;

	/**
	 * \brief A double value containing the time this pick was first associated
	 * with a Hypo.
	 */
	std::atomic<double> m_tFirstAssociation;

	/**
	 * \brief A double value containing the time this pick was nucleated.  Could
	 * be 0 if nucleation wasn't done with this pick.
	 */
	std::atomic<double> m_tNucleation;

	/**
	 * \brief A recursive_mutex to control threading access to CPick.
	 * NOTE: recursive mutexes are frowned upon, so maybe redesign around it
	 * see: http://www.codingstandard.com/rule/18-3-3-do-not-use-stdrecursive_mutex/
	 * However a recursive_mutex allows us to maintain the original class
	 * design as delivered by the contractor.
	 */
	mutable std::recursive_mutex m_PickMutex;

	// constants
	/**
	 * \brief The number of anneal passes to run when nucleating
	 */
	static const unsigned int k_nNucleateAnnealPasses = 3;

	/**
	 * \brief The number of anneal iterations to run when nucleating
	 */
	static const unsigned int k_nNucleateNumberOfAnnealIterations = 20000;

	/**
	 * \brief The initial anneal step size to use when nucleating
	 */
	static constexpr double k_dNucleateInitialAnnealTimeStepSize = 10.0;

	/**
	 * \brief The final anneal step size to use when nucleating
	 */
	static constexpr double k_dNucleateFinalAnnealTimeStepSize = 0.1;
};
}  // namespace glasscore
#endif  // PICK_H

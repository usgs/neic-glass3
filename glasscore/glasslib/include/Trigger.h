/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef TRIGGER_H
#define TRIGGER_H

#include <geo.h>
#include <vector>
#include <memory>
#include <string>
#include <utility>
#include <mutex>
#include <tuple>
#include <atomic>
#include "Link.h"

namespace glasscore {

// forward declarations
class CPick;
class CWeb;

/**
 * \brief glasscore detection trigger class
 *
 * The CTrigger class represents a single detection trigger from the
 * detection graph database.  A CTrigger consists of the location (latitude,
 * longitude, and depth) of the triggering node, the spatial resolution of the
 * triggering node, and a list of picks that made the trigger
 *
 * CTrigger uses smart pointers (std::shared_ptr).
 */
class CTrigger {
 public:
	/**
	 * \brief CTrigger constructor
	 */
	CTrigger();

	/**
	 * \brief CTrigger advanced constructor
	 *
	 * Construct a trigger using the provided data
	 *
	 * \param lat - A double value containing the latitude to use
	 * for this trigger in degrees
	 * \param lon - A double value containing the longitude to use
	 * for this trigger in degrees
	 * \param z - A double value containing the depth to use
	 * for this trigger in kilometers
	 * \param ot - A double value containing the time to use
	 * for this trigger in seconds
	 * \param resolution - A double value containing the inter-node resolution
	 * in kilometer
	 * \param sum - A double value containing the bayesian sum for this trigger
	 * \param count - An integer value containing the site count for this
	 * trigger
	 * \param picks - A std::vector<std::shared_ptr<CPick> containing the picks
	 * for this trigger
	 * \param web - A pointer to the creating node's CWeb
	 */
	CTrigger(double lat, double lon, double z, double ot, double resolution,
				double sum, int count,
				std::vector<std::shared_ptr<CPick>> picks, CWeb *web);

	/**
	 * \brief CTrigger destructor
	 */
	~CTrigger();

	/**
	 * \brief CTrigger initialization function
	 *
	 * Initialize a trigger using the provided data
	 *
	 * \param lat - A double value containing the latitude to use
	 * for this trigger in degrees
	 * \param lon - A double value containing the longitude to use
	 * for this trigger in degrees
	 * \param z - A double value containing the depth to use
	 * for this trigger in kilometers
	 * \param ot - A double value containing the time to use
	 * for this trigger in seconds
	 * \param resolution - A double value containing the inter-node resolution
	 * in kilometer
	 * \param sum - A double value containing the bayesian sum for this trigger
	 * \param count - An integer value containing the site count for this
	 * trigger
	 * \param picks - A std::vector<std::shared_ptr<CPick> containing the picks
	 * for this trigger
	 * \param web - A pointer to the creating node's web
	 */
	bool initialize(double lat, double lon, double z, double ot,
					double resolution, double sum, int count,
					std::vector<std::shared_ptr<CPick>> picks, CWeb *web);

	/**
	 * \brief CTrigger clear function
	 */
	void clear();

	/**
	 * \brief Get the latitude for this trigger
	 * \return Return a double containing the latitude in degrees
	 */
	double getLatitude() const;

	/**
	 * \brief Get the longitude for this trigger
	 * \return Return a double containing the longitude in degrees
	 */
	double getLongitude() const;

	/**
	 * \brief Get the depth for this trigger
	 * \return Return a double containing the depth in kilometers
	 */
	double getDepth() const;

	/**
	 * \brief Get the combined trigger location (latitude, longitude, depth) as
	 * a CGeo object
	 * \return Returns a glass3::util::Geo object containing the combined location.
	 */
	glass3::util::Geo getGeo() const;

	/**
	 * \brief Get the origin time for this trigger
	 * \return Returns a double containing the trigger origin time in julian seconds
	 */
	double getTOrigin() const;

	/**
	 * \brief Gets the node resolution of the web that nucleated this trigger
	 * \return Returns a double value containing the node resolution of the web
	 * that nucleated this trigger in kilometers
	 */
	double getWebResolution() const;

	/**
	 * \brief Gets the current bayes stack value for this trigger
	 * \return Returns a double containing the current bayes stack value
	 */
	double getBayesValue() const;

	/**
	 * \brief Gets the number of picks supporting this trigger
	 * \return Returns and integer containing the number of picks supporting
	 * this trigger
	 */
	int getPickCount() const;

	/**
	 * \brief Gets a pointer to the web that generated this trigger, used to get
	 * travel times
	 * \return Returns a pointer to the CWeb that generated this trigger
	 */
	const CWeb* getWeb() const;

	/**
	 * \brief Get the list of picks supporting this trigger
	 * \return Returns a vector of shared_ptr's to the CPick objects supporting
	 * this trigger
	 */
	const std::vector<std::shared_ptr<CPick>> getVPick() const;

 private:
	/**
	 * \brief A double value containing latitude of this trigger in degrees.
	 */
	std::atomic<double> m_dLatitude;

	/**
	 * \brief A double value containing longitude of this trigger in degrees.
	 */
	std::atomic<double> m_dLongitude;

	/**
	 * \brief A double value containing the depth of this trigger in kilometers.
	 */
	std::atomic<double> m_dDepth;

	/**
	 * \brief A double value with the origin time of this trigger in seconds
	 */
	std::atomic<double> m_tOrigin;

	/**
	 * \brief A double value containing the spatial resolution
	 * (between nodes) in kilometers.
	 */
	std::atomic<double> m_dWebResolution;

	/**
	 * \brief A double value that accumulates the Bayesian
	 * sum of this trigger
	 */
	std::atomic<double> m_dBayesValue;

	/**
	 * \brief A integer value that tallies the number of sites
	 * that are included in this trigger
	 */
	std::atomic<int> m_iPickCount;

	/**
	 * \brief A std::vector of std::shared_ptr's to CPick objects
	 * used in creating this trigger
	 */
	std::vector<std::shared_ptr<CPick>> m_vPick;

	/**
	 * \brief A pointer to the node CWeb class, used to get travel times
	 */
	CWeb * m_pWeb;

	/**
	 * \brief A recursive mutex to control threading access to this trigger.
	 */
	mutable std::recursive_mutex m_TriggerMutex;
};
}  // namespace glasscore
#endif  // TRIGGER_H

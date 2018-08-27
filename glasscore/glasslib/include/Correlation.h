/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef CORRELATION_H
#define CORRELATION_H

#include <json.h>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>

namespace glasscore {

// forward declarations
class CSite;
class CSiteList;
class CHypo;

/**
 * \brief glasscore correlation class
 *
 * The CCorrelation class is the class that encapsulates everything necessary
 * to represent a waveform arrival correlation, including arrival time, phase id,
 * and an unique identifier.  The CCorrelation class is also a node in the
 * detection graph database.
 *
 * CCorrelation contains functions to support creation of a new event based
 * on the correlation.
 *
 * CCorrelation maintains a graph database link between it and the the site (station)
 * the correlation was made at.
 *
 * CCorrelation also maintains a vector of CHypo objects represent the graph database
 * links between  this correlation and various hypocenters.  A single correlation may be
 * linked to multiple hypocenters
 *
 * CCorrelation uses smart pointers (std::shared_ptr).
 */
class CCorrelation {
 public:
	/**
	 * \brief CCorrelation constructor
	 */
	CCorrelation();

	/**
	 * \brief CCorrelation alternate constructor
	 *
	 * Constructs a CCorellation using the provided values
	 *
	 * \param correlationSite - A shared pointer to a CSite object that the
	 * correlation wasmade at
	 * \param correlationTime - A double containing the correlation arrival time
	 * \param correlationId - An integer containing the glass correlation id
	 * (index) to use.
	 * \param correlationIdString - A std::string containing the external
	 * correlation id.
	 * \param phase - A std::string containing the phase name
	 * \param orgTime - A double containing the julian time in seconds to use
	 * \param orgLat - A double containing the geocentric latitude in degrees to
	 * use
	 * \param orgLon - A double containing the geocentric longitude in degrees
	 * to use
	 * \param orgZ - A double containing the geocentric depth in kilometers to
	 * use
	 * \param corrVal - A double containing the correlation value
	 */
	CCorrelation(std::shared_ptr<CSite> correlationSite, double correlationTime,
					int correlationId, std::string correlationIdString,
					std::string phase, double orgTime, double orgLat,
					double orgLon, double orgZ, double corrVal);

	/**
	 * \brief CCorrelation alternate constructor
	 *
	 * Constructs a CCorrelation class from the provided json object and id, using
	 * a CGlass pointer to convert times and lookup stations.
	 *
	 * \param correlation - A pointer to a json::Object to construct the correlation from
	 * \param correlationId - An integer containing the correlation id to use.
	 * \param pSiteList - A pointer to the CSiteList class
	 */
	CCorrelation(std::shared_ptr<json::Object> correlation, int correlationId,
					CSiteList *pSiteList);

	/**
	 * \brief CCorrelation destructor
	 */
	~CCorrelation();

	/**
	 * \brief CCorrelation clear function
	 */
	void clear();

	/**
	 * \brief CCorrelation initialization function
	 *
	 * Initializes correlation class to provided values.
	 *
	 * \param correlationSite - A shared pointer to a CSite object that the
	 * correlation wasmade at
	 * \param correlationTime - A double containing the correlation arrival time
	 * \param correlationId - An integer containing the glass correlation id
	 * (index) to use.
	 * \param correlationIdString - A std::string containing the external
	 * correlation id.
	 * \param phase - A std::string containing the phase name
	 * \param orgTime - A double containing the julian time in seconds to use
	 * \param orgLat - A double containing the geocentric latitude in degrees to
	 * use
	 * \param orgLon - A double containing the geocentric longitude in degrees
	 * to use
	 * \param orgZ - A double containing the geocentric depth in kilometers to
	 * use
	 * \param corrVal - A double containing the correlation value
	 * \return Returns true if successful, false otherwise.
	 */
	bool initialize(std::shared_ptr<CSite> correlationSite,
					double correlationTime, int correlationId,
					std::string correlationIdString, std::string phase,
					double orgTime, double orgLat, double orgLon, double orgZ,
					double corrVal);

	/**
	 * \brief Add hypo reference to this correlation
	 *
	 * Adds a shared_ptr reference to the given hypo to this correlation,
	 * representing a graph database link between this correlation and the
	 * hypocenters.
	 *
	 * Note that this correlation may or may not also be linked
	 * to other hypocenters
	 *
	 * \param hyp - A std::shared_ptr to an object containing the hypocenter
	 * to link.
	 * \param force - A boolean flag indicating whether to force the association,
	 * defaults to false.
	 */
	void addHypo(std::shared_ptr<CHypo> hyp, bool force =
							false);

	/**
	 * \brief Remove hypo specific reference to this correlation
	 *
	 * Remove a shared_ptr reference from the given hypo to this correlation,
	 * breaking the graph database link between this correlation and the
	 * hypocenter.
	 *
	 * Note that this correlation may or may not be still linked
	 * to other hypocenters
	 *
	 * \param hyp - A std::shared_ptr to an object containing the hypocenter
	 * to unlink.
	 */
	void removeHypo(std::shared_ptr<CHypo> hyp);

	/**
	 * \brief Remove hypo specific reference to this correlation
	 *
	 * Remove a shared_ptr reference from the given hypo id to this correlation,
	 * breaking the graph database link between this correlation and the
	 * hypocenter.
	 *
	 * Note that this correlation may or may not be still linked
	 * to other hypocenters
	 *
	 * \param pid - A std::string identifying the the hypocenter to unlink.
	 */
	void removeHypo(std::string pid);

	/**
	 * \brief Remove hypo reference to this correlation
	 *
	 * Remove any shared_ptr reference from this correlation, breaking the graph
	 * database link between this correlation and any hypocenter.
	 */
	void clearHypo();

	/**
	 * \brief Correlation value getter
	 * \return the correlation value
	 */
	double getCorrelation() const;

	/**
	 * \brief Latitude getter
	 * \return the latitude
	 */
	double getLatitude() const;

	/**
	 * \brief Longitude getter
	 * \return the longitude
	 */
	double getLongitude() const;

	/**
	 * \brief Depth getter
	 * \return the depth
	 */
	double getDepth() const;

	/**
	 * \brief Origin time getter
	 * \return the origin time
	 */
	double getTOrigin() const;

	/**
	 * \brief Correlation id getter
	 * \return the correlation id
	 */
	int getCorrelationID() const;

	/**
	 * \brief Json correlation getter
	 * \return the json correlation
	 */
	const std::shared_ptr<json::Object>& getJSONCorrelation() const;

	/**
	 * \brief Hypo getter
	 * \return the hypo
	 */
	const std::shared_ptr<CHypo> getHypo() const;

	/**
	 * \brief Pid getter
	 * \return the pid
	 */
	const std::string getHypoID() const;

	/**
	 * \brief Site getter
	 * \return the site
	 */
	const std::shared_ptr<CSite>& getSite() const;

	/**
	 * \brief Phase getter
	 * \return the phase
	 */
	const std::string& getPhaseName() const;

	/**
	 * \brief Pid getter
	 * \return the pid
	 */
	const std::string& getID() const;

	/**
	 * \brief Correlation time getter
	 * \return the correlation time
	 */
	double getTCorrelation() const;

	/**
	 * \brief Creation time getter
	 * \return the creation time
	 */
	double getTCreate() const;

 private:
	/**
	 * \brief A std::shared_ptr to a CSite object
	 * representing the link between this correlation and the site it was
	 * correlated at
	 */
	std::shared_ptr<CSite> m_pSite;

	/**
	 * \brief A std::weak_ptr to a CHypo object
	 * representing the links between this correlation and associated hypocenter
	 */
	std::weak_ptr<CHypo> m_wpHypo;

	/**
	 * \brief A std::string containing the phase name of this correlation
	 */
	std::string m_sPhaseName;

	/**
	 * \brief A std::string containing the string unique id of this correlation
	 */
	std::string m_sID;

	/**
	 * \brief An integer value containing the numeric id of the correlation
	 */
	std::atomic<int> m_iCorrelationID;

	/**
	 * \brief A double value containing the arrival time of the correlation
	 */
	std::atomic<double> m_tCorrelation;

	/**
	 * \brief A double value containing this correlation's origin time in julian
	 * seconds
	 */
	std::atomic<double> m_tOrigin;

	/**
	 * \brief A double value containing this correlation's latitude in degrees
	 */
	std::atomic<double> m_dLatitude;

	/**
	 * \brief A double value containing this correlation's longitude in degrees
	 */
	std::atomic<double> m_dLongitude;

	/**
	 * \brief A double value containing this correlation's depth
	 * in kilometers.
	 */
	std::atomic<double> m_dDepth;

	/**
	 * \brief A double value containing this correlation's correlation value
	 */
	std::atomic<double> m_dCorrelation;

	/**
	 * \brief A double value containing the creation time of the correlation in
	 * glass
	 */
	std::atomic<double> m_tCreate;

	/**
	 * \brief A std::shared_ptr to a json object
	 * representing the original correlation input, used in accessing
	 * information not relevant to glass that are needed for generating outputs.
	 */
	std::shared_ptr<json::Object> m_JSONCorrelation;

	/**
	 * \brief A recursive_mutex to control threading access to CCorrelation.
	 * NOTE: recursive mutexes are frowned upon, so maybe redesign around it
	 * see: http://www.codingstandard.com/rule/18-3-3-do-not-use-stdrecursive_mutex/
	 * However a recursive_mutex allows us to maintain the original class
	 * design as delivered by the contractor.
	 */
	mutable std::recursive_mutex m_CorrelationMutex;
};
}  // namespace glasscore
#endif  // CORRELATION_H

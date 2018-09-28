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
 * CCorrelation maintains a graph database link between it and the the site
 * (station) the correlation was made at.
 *
 * CCorrelation also maintains a weak_ptr to a CHypo object representing a graph
 * database link between this correlation and a hypocenters.  A single
 * correlation may be included in multiple hypocenters, but a correlation will
 * only include a single hypocenter
 *
 * CCorrelation uses smart pointers (std::shared_ptr).
 */
class CCorrelation {
 public:
	/**
	 * \brief CCorrelation default constructor
	 *
	 * The default constructor for the CCorrelation class.
	 * Initializes members to default values.
	 */
	CCorrelation();

	/**
	 * \brief CCorrelation advanced constructor
	 *
	 * An advanced constructor for the CCorrelation class. This function
	 * initializes members to the provided values.
	 *
	 * \param correlationSite - A shared pointer to a CSite object that the
	 * correlation wasmade at
	 * \param correlationTime - A double containing the correlation arrival time
	 * \param correlationIdString - A std::string containing the external
	 * correlation id.
	 * \param phase - A std::string containing the phase name
	 * \param orgTime - A double containing the Gregorian time in seconds to use
	 * \param orgLat - A double containing the geocentric latitude in degrees to
	 * use
	 * \param orgLon - A double containing the geocentric longitude in degrees
	 * to use
	 * \param orgZ - A double containing the geocentric depth in kilometers to
	 * use
	 * \param corrVal - A double containing the correlation value
	 */
	CCorrelation(std::shared_ptr<CSite> correlationSite, double correlationTime,
					std::string correlationIdString, std::string phase,
					double orgTime, double orgLat, double orgLon, double orgZ,
					double corrVal);

	/**
	 * \brief CCorrelation advanced constructor
	 *
	 * An advanced constructor for the CCorrelation class. This function
	 * initializing members to the values parsed from the provided json object,
	 * and using the provided pointer to a CSiteList class to lookup the
	 * correlation station.
	 *
	 * \param correlation - A shared pointer to a json::Object to containing the
	 * data to construct the correlation from
	 * \param pSiteList - A pointer to the CSiteList class to use when looking
	 * up the correlation station
	 */
	CCorrelation(std::shared_ptr<json::Object> correlation,
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
	 * \param correlationIdString - A std::string containing the external
	 * correlation id.
	 * \param phase - A std::string containing the phase name
	 * \param orgTime - A double containing the Gregorian time in seconds to use
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
					double correlationTime, std::string correlationIdString,
					std::string phase, double orgTime, double orgLat,
					double orgLon, double orgZ, double corrVal);

	/**
	 * \brief Add a hypo reference to this correlation
	 *
	 * Adds a weak_ptr reference to the given hypo to this correlation,
	 * representing a graph database link between this correlation and a
	 * hypocenter. If the correlation is already linked to a hypocenter,
	 * the new link will be ignored unless force is set to true.
	 *
	 * Note that this correlation may or may not also be included in other
	 * hypocenter correlation data lists, but this correlation will only link to
	 * a single hypocenter
	 *
	 * \param hyp - A std::shared_ptr to an object containing the hypocenter
	 * to link.
	 * \param force - A boolean flag indicating whether to force the association,
	 * defaults to false.
	 */
	void addHypoReference(std::shared_ptr<CHypo> hyp, bool force = false);

	/**
	 * \brief Remove hypo specific reference to this correlation
	 *
	 * Remove a weak_ptr reference to the given hypo from this correlation,
	 * breaking the graph database link between this correlation and the
	 * hypocenter.
	 *
	 * Note that this correlation may or may not still be included in other
	 * hypocenter correlation data lists.
	 *
	 * \param hyp - A std::shared_ptr to an object containing the hypocenter
	 * to unlink.
	 */
	void removeHypoReference(std::shared_ptr<CHypo> hyp);

	/**
	 * \brief Remove hypo specific reference to this correlation by id
	 *
	 * Remove a weak_ptr reference to the given hypo id from this correlation,
	 * breaking the graph database link between this correlation and the
	 * hypocenter.
	 *
	 * Note that this correlation may or may not still be included in other
	 * hypocenter correlation data lists.
	 *
	 * \param pid - A std::string identifying the the hypocenter to unlink.
	 */
	void removeHypoReference(std::string pid);

	/**
	 * \brief Remove hypo reference to this correlation
	 *
	 * Remove any existing weak_ptr reference to a hypo from this correlation,
	 * breaking the graph database link between this correlation and a hypocenter.
	 *
	 * Note that this correlation may or may not still be included in other
	 * hypocenter correlation data lists.
	 */
	void clearHypoReference();

	/**
	 * \brief Get the correlation value for this correlation
	 * \return Return a double containing the correlation value
	 */
	double getCorrelation() const;

	/**
	 * \brief Get the correlation source latitude for this correlation
	 * \return Return a double containing the correlation source latitude in
	 * degrees
	 */
	double getLatitude() const;

	/**
	 * \brief Get the correlation source longitude for this correlation
	 * \return Return a double containing the correlation source longitude in
	 * degrees
	 */
	double getLongitude() const;

	/**
	 * \brief Get the correlation source depth for this correlation
	 * \return Return a double containing the correlation source depth in
	 * kilometers
	 */
	double getDepth() const;

	/**
	 * \brief Get the correlation source origin time for this correlation
	 * \return Return a double containing the correlation source origin time in
	 * Gregorian seconds
	 */
	double getTOrigin() const;

	/**
	 * \brief Get input JSON correlation message
	 * \return Return a shared_ptr to a json::Object containing the correlation
	 * message
	 */
	const std::shared_ptr<json::Object>& getJSONCorrelation() const;

	/**
	 * \brief Get the current hypo reference to this correlation
	 *
	 * Note that this correlation may or may not also be included in other
	 * hypocenter correlation data lists, but this correlation will only link to
	 * a single hypocenter
	 *
	 * \return Return a shared_ptr to the CHypo referenced by this correlation,
	 * or NULL if no hypo is referenced
	 */
	const std::shared_ptr<CHypo> getHypoReference() const;

	/**
	 * \brief Get the site for this correlation
	 * \return Return a shared_ptr to a CSite object containing the site this
	 * correlation was made at
	 */
	const std::shared_ptr<CSite> getSite() const;

	/**
	 * \brief Get the phase name for this correlation
	 * \return Return a std::string containing the correlation phase name
	 */
	const std::string& getPhaseName() const;

	/**
	 * \brief Get the ID of this correlation
	 * \return Return a std::string containing the correlation ID
	 */
	const std::string& getID() const;

	/**
	 * \brief Get the arrival time for this correlation
	 * \return Return a double containing the correlation arrival time
	 */
	double getTCorrelation() const;

	/**
	 * \brief Get the creation (insertion) time of this correlation
	 * \return Return a double containing the correlation creation time
	 */
	double getTCreate() const;

 private:
	/**
	 * \brief A std::weak_ptr to a CSite object
	 * representing the link between this correlation and the site it was
	 * correlated at
	 */
	std::weak_ptr<CSite> m_wpSite;

	/**
	 * \brief A std::weak_ptr to a CHypo object
	 * representing the links between this correlation and associated hypocenter
	 * A weak_ptr is used here instead of a shared_ptr to prevent a cyclical
	 * reference between CPick and CHypo.
	 */
	std::weak_ptr<CHypo> m_wpHypo;

	/**
	 * \brief A std::string containing the phase name of this correlation
	 */
	std::string m_sPhaseName;

	/**
	 * \brief A std::string containing the unique string id of this correlation
	 */
	std::string m_sID;

	/**
	 * \brief A double value containing the arrival time of the correlation
	 */
	std::atomic<double> m_tCorrelation;

	/**
	 * \brief A double value containing this correlation's origin time in Gregorian
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
	 * \brief A double value containing the creation (insertion) time of the
	 * correlation in glass in Gregorian seconds
	 */
	std::atomic<double> m_tCreate;

	/**
	 * \brief A std::shared_ptr to a json object representing the original
	 * correlation input message, used in accessing information not relevant to
	 * glass that are needed for generating outputs.
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

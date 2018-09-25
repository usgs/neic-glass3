/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef HYPO_H
#define HYPO_H

#include <json.h>
#include <geo.h>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include "TTT.h"

namespace glasscore {

// forward declarations
class CPick;
class CCorrelation;
class CTrigger;
class CSiteList;

/**
 * \brief glasscore hypocenter class
 *
 * The CHypo class is the class that encapsulates everything necessary
 * to represent an earthquake hypocenter.
 *
 * CHypo also maintains vectors of shared_ptr's to CPick and CCorrelation
 * objects that make up the data that supports the hypocenter
 *
 * CHypo contains functions to support association, disassociation, location,
 * removal, and various statistical calculations, as well as generating
 * output data in various formats.
 *
 * The CHypo associate() and prune() functions essentially make up
 * the glasscore data association engine, along with the various statistical
 * calculations.
 *
 * The glasscore location algorithm consists of the CHypo anneal(), localize(),
 * annealingLocateBayes(), annealingLocateResidual(), calculateBayes(), and
 * calculateAbsResidualSum() functions, along with the various statistical
 * calculations.
 *
 * CHypo uses smart pointers (std::shared_ptr).
 */
class CHypo {
 public:
	/**
	 * \brief CHypo constructor
	 *
	 * The constructor for the CHypo class.
	 * Sets allocated objects to null.
	 * Initializes members to default values.
	 */
	CHypo();

	/**
	 * \brief CHypo advanced constructor
	 *
	 * An advanced constructor for the CHypo class. This function initializes
	 * members to the provided values found in the json detection message.
	 *
	 * \param detection - A shared pointer to a json::Object to containing the
	 * data to construct the hypo from
	 * \param thresh - A double containing the threshold value for this hypo
	 * \param cut - An integer containing the Bayesian stack threshold for this
	 * hypo
	 * \param firstTrav - A traveltime::CTravelTime containing the first travel
	 * time used in creating this hypo
	 * \param secondTrav - A traveltime::CTravelTime containing the second travel
	 * time used in creating this hypo
	 * \param ttt - A traveltime::CTTT to be used for association for this hypo
	 * \param resolution - A double value containing the web resolution used
	 * \param aziTaper = A double value containing the azimuth taper to be used,
	 * defaults to 360
	 * \param maxDepth = A double value containing the maximum allowed depth,
	 * defaults to 800
	 * \param pSiteList - A pointer to the CSiteList class to use when looking
	 * up the pick station
	 * \return Returns true if successful, false otherwise.
	 */
	CHypo(std::shared_ptr<json::Object> detection, double thresh, int cut,
			std::shared_ptr<traveltime::CTravelTime> firstTrav,
			std::shared_ptr<traveltime::CTravelTime> secondTrav,
			std::shared_ptr<traveltime::CTTT> ttt, double resolution = 100,
			double aziTaper = 360.0, double maxDepth = 800.0,
			CSiteList *pSiteList = NULL);

	/**
	 * \brief CHypo advanced constructor
	 *
	 * An advanced constructor for the CHypo class. This function initializes
	 * members to the provided values.
	 *
	 * \param lat - A double containing the geocentric latitude in degrees to
	 * use
	 * \param lon - A double containing the geocentric longitude in degrees to
	 * use
	 * \param z - A double containing the geocentric depth in kilometers to use
	 * \param time - A double containing the julian time in seconds to use
	 * \param pid - A std::string containing the id of this hypo
	 * \param web - A std::string containing the name of the web that nucleated
	 * this hypo
	 * \param bayes - A double containing the bayesian value for this hypo.
	 * \param thresh - A double containing the threshold value for this hypo
	 * \param cut - An integer containing the Bayesian stack threshold for this
	 * hypo
	 * \param firstTrav - A traveltime::CTravelTime containing the first travel
	 * time used in creating this hypo
	 * \param secondTrav - A traveltime::CTravelTime containing the second travel
	 * time used in creating this hypo
	 * \param ttt - A traveltime::CTTT to be used for association for this hypo
	 * \param resolution - A double value containing the web resolution used
	 * \param aziTaper = A double value containing the azimuth taper to be used,
	 * defaults to 360
	 * \param maxDepth = A double value containing the maximum allowed depth,
	 * defaults to 800
	 * \return Returns true if successful, false otherwise.
	 */
	CHypo(double lat, double lon, double z, double time, std::string pid,
			std::string web, double bayes, double thresh, int cut,
			std::shared_ptr<traveltime::CTravelTime> firstTrav,
			std::shared_ptr<traveltime::CTravelTime> secondTrav,
			std::shared_ptr<traveltime::CTTT> ttt, double resolution = 100,
			double aziTaper = 360.0, double maxDepth = 800.0);

	/**
	 * \brief CHypo advanced constructor
	 *
	 * An advanced constructor for the CHypo class. This function initializing
	 * members to the values contained in the provided CTrigger object, used
	 * when a new hypo is nucleated.
	 *
	 * \param trigger - A CTrigger object containing the nucleation trigger to
	 * construct this hypo from.
	 * \param ttt - A traveltime::CTTT to be used for association for this hypo
	 */
	explicit CHypo(std::shared_ptr<CTrigger> trigger,
					std::shared_ptr<traveltime::CTTT> ttt);

	/**
	 * \brief CHypo advanced constructor
	 *
	 * An advanced constructor for the CHypo class. This function initializing
	 * members to the values contained in the provided CCorrelation object, used
	 * when a creating a new hypo based on a correlation message.
	 *
	 * \param corr - A shared pointer to a CNode object containing the
	 * correlation to construct this hypo from.
	 * \param firstTrav - A traveltime::CTravelTime containing the first travel
	 * time used in creating this hypo
	 * \param secondTrav - A traveltime::CTravelTime containing the second travel
	 * time used in creating this hypo
	 * \param ttt - A traveltime::CTTT to be used for association for this hypo
	 */
	explicit CHypo(std::shared_ptr<CCorrelation> corr,
					std::shared_ptr<traveltime::CTravelTime> firstTrav,
					std::shared_ptr<traveltime::CTravelTime> secondTrav,
					std::shared_ptr<traveltime::CTTT> ttt);

	/**
	 * \brief CHypo destructor
	 *
	 * The destructor for the CHypo class.
	 * Cleans up all memory allocated objects.
	 */
	~CHypo();

	/**
	 * \brief CHypo clear function
	 */
	void clear();

	/**
	 * \brief CHypo initialization function
	 *
	 * Initializes hypo class to provided values.
	 *
	 * \param lat - A double containing the geocentric latitude in degrees to
	 * use
	 * \param lon - A double containing the geocentric longitude in degrees to
	 * use
	 * \param z - A double containing the geocentric depth in kilometers to use
	 * \param time - A double containing the julian time in seconds to use
	 * \param pid - A std::string containing the id of this hypo
	 * \param web - A std::string containing the name of the web that nucleated
	 * this hypo
	 * \param bayes - A double containing the bayesian value for this hypo.
	 * \param thresh - A double containing the threshold value for this hypo
	 * \param cut - An integer containing the Bayesian stack threshold for this
	 * hypo
	 * \param firstTrav - A traveltime::CTravelTime containing the first travel
	 * time used in creating this hypo
	 * \param secondTrav - A traveltime::CTravelTime containing the second travel
	 * time used in creating this hypo
	 * \param ttt - A traveltime::CTTT to be used for association for this hypo
	 * \param resolution - A double value containing the web resolution used
	 * \param aziTaper = A double value containing the azimuth taper to be used,
	 * defaults to 360
	 * \param maxDepth = A double value the maximum event depth for the locator,
	 * defaults to 800
	 * \return Returns true if successful, false otherwise.
	 */
	bool initialize(double lat, double lon, double z, double time,
					std::string pid, std::string web, double bayes,
					double thresh, int cut,
					std::shared_ptr<traveltime::CTravelTime> firstTrav,
					std::shared_ptr<traveltime::CTravelTime> secondTrav,
					std::shared_ptr<traveltime::CTTT> ttt, double resolution =
							100,
					double aziTaper = 360.0, double maxDepth = 800.0);

	/**
	 * \brief Add pick reference to this hypo
	 *
	 * Adds a shared_ptr reference to the given pick to the list of supporting
	 * pick references for this hypo, representing a graph database link between
	 * this hypocenter and the provided pick.  This link also represents a phase
	 * association.
	 *
	 * Note that this pick may or may not also be referenced by other hypocenters
	 *
	 * \param pck - A std::shared_ptr to the CPick object to add.
	 */
	void addPickReference(std::shared_ptr<CPick> pck);

	/**
	 * \brief Remove pick reference from this hypo
	 *
	 * Remove a shared_ptr reference to the given pick from the list of supporting
	 * pick references for this hypo, breaking the graph database link between
	 * this hypocenter and the provided pick. The breaking of this link also
	 * represents a phase disassociation.
	 *
	 * Note that this pick may or may not be still referenced by other hypocenters
	 *
	 * \param pck - A std::shared_ptr to the CPick object to remove.
	 */
	void removePickReference(std::shared_ptr<CPick> pck);

	/**
	 * \brief Check if pick is referenced by this hypo
	 *
	 * Check to see if a shared_ptr reference from the given pick to this hypo
	 * exists
	 *
	 * Note that this pick may or may not be also referenced by other hypocenters
	 *
	 * \param pck - A std::shared_ptr to the CPick object to check.
	 * \return returns true if a reference exists to the given pick, false
	 * otherwise
	 */
	bool hasPickReference(std::shared_ptr<CPick> pck);

	/**
	 * \brief Clear all pick references for this hypo
	 *
	 * Clears the list of supporting shared_ptr pick references for this hypo
	 *
	 * Note picks may or may not be still referenced by other hypocenters
	 */
	void clearPickReferences();

	/**
	 * \brief Add correlation reference to this hypo
	 *
	 * Adds a shared_ptr reference to the given correlation to the list of
	 * supporting correlation references for this hypo, representing a graph
	 * database link between this hypocenter and the provided correlation.
	 * This link also represents a correlation association.
	 *
	 * Note that this correlation may or may not also be referenced by other
	 * hypocenters
	 *
	 * \param corr - A std::shared_ptr to the CCorrelation object to
	 * add.
	 */
	void addCorrelationReference(std::shared_ptr<CCorrelation> corr);

	/**
	 * \brief Remove correlation reference from this hypo
	 *
	 * Remove a shared_ptr reference to the given correlation from the list of
	 * supporting correlation references for this hypo, breaking the graph
	 * database link between this hypocenter and the provided correlation. The
	 * breaking of this link also represents a correlation disassociation.
	 *
	 * Note that this correlation may or may not still be referenced by other
	 * hypocenters
	 *
	 * \param corr - A std::shared_ptr to the CCorrelation object to remove.
	 */
	void removeCorrelationReference(std::shared_ptr<CCorrelation> corr);

	/**
	 * \brief  Check if correlation is referenced by this hypo
	 *
	 * Check to see if a shared_ptr reference from the given correlation to this
	 * hypo exists
	 *
	 * Note that this correlation may or may not be also referenced by other
	 * hypocenters
	 *
	 * \param corr - A std::shared_ptr to the CCorrelation object to check.
	 * \return returns true if a reference exists to the given correlation,
	 * false otherwise
	 */
	bool hasCorrelationReference(std::shared_ptr<CCorrelation> corr);

	/**
	 * \brief Clear all correlation references for this hypo
	 *
	 * Clears the list of supporting shared_ptr correlation references for this
	 * hypo
	 *
	 * Note correlations may or may not be still referenced by other hypocenters
	 */
	void clearCorrelationReferences();

	/**
	 * \brief Generate Hypo message
	 *
	 * Generate a json object representing this hypocenter in the "Hypo" format
	 *
	 * \return Returns the generated json object in the "Hypo" format.
	 */
	std::shared_ptr<json::Object> generateHypoMessage();

	/**
	 * \brief Generate Event message
	 *
	 * Generate a json object representing a summary of this hypocenter in the
	 * "Event" format
	 *
	 * \return Returns the generated json object in the "Event" format.
	 */
	std::shared_ptr<json::Object> generateEventMessage();

	/**
	 * \brief Generate cancel message
	 *
	 * Generate a json object representing a cancellation of this hypocenter in
	 * the "Cancel" format
	 *
	 * \return Returns the generated json object in the "Cancel" format.
	 */
	std::shared_ptr<json::Object> generateCancelMessage();

	/**
	 * \brief Generate expire message
	 *
	 * Generate a json object representing a expiration of this hypocenter in
	 * the "Expire" format
	 *
	 * If this hypocenter was previously reported, a copy of it is included in
	 * this message via the generateHypoMessage() function
	 *
	 * \return Returns the generated json object in the "Expire" format.
	 */
	std::shared_ptr<json::Object> generateExpireMessage();

	/**
	 * \brief Check to see if pick could be associated
	 *
	 * Check to see if a given pick could be associated to this CHypo
	 *
	 * \param pick - A std::shared_ptr to the CPick object to
	 * check.
	 * \param sigma - A double value containing the sigma to use
	 * \param sdassoc - A double value containing the standard
	 * deviation assocaiation limit to use
	 * \return Returns true if the pick can be associated, false otherwise
	 */
	bool canAssociate(std::shared_ptr<CPick> pick, double sigma,
						double sdassoc);

	/**
	 * \brief Calculates the residual of a pick to this hypo
	 *
	 * Calculates the residual of the given supporting data to this hypo
	 *
	 * \param pick - The pick to calculate a residual for
	 * \return Returns a double value containing the residual of the given
	 * pick
	 */
	double calculateResidual(std::shared_ptr<CPick> pick);

	/**
	 * \brief Check to see if correlation could be associated
	 *
	 * Check to see if a given correlation could be associated to this
	 * CHypo
	 *
	 * \param corr - A std::shared_ptr to the CCorrelation object to
	 * check.
	 * \param tWindow - A double value containing the time window to use
	 * \param xWindow - A double value containing the distance window
	 * \return Returns true if the correlation can be associated, false otherwise
	 */
	bool canAssociate(std::shared_ptr<CCorrelation> corr, double tWindow,
						double xWindow);

	/* \brief Calculate pick affinity
	 *
	 * Calculate the association affinity between the given
	 * supporting pick and the current hypocenter based on a number of factors
	 * including the identified phase, distance to picked station,
	 * observation error, and other hypocentral properties
	 *
	 * \param pck - A std::shared_ptr to the pick to consider.
	 * \return Returns a double value containing the pick affinity
	 */
	double calculateAffinity(std::shared_ptr<CPick> pck);

	/* \brief Calculate correlation affinity
	 *
	 * Calculate the association affinity between the given
	 * supporting correlation and the current hypocenter based on a number of
	 * factors including distance to correlation station,
	 * time to correlation, and other properties
	 *
	 * \param corr - A std::shared_ptr to the correlation to consider.
	 * \return Returns a double value containing the correlation affinity
	 */
	double calculateAffinity(std::shared_ptr<CCorrelation> corr);

	/* \brief Remove data that no longer fit association criteria
	 *
	 * Calculate the association affinity between it's associated
	 * supporting data and the current hypocenter based on a number of factors
	 * including the identified phase, distance to picked station,
	 * observation error, and other hypocentral properties
	 *
	 * \return Returns true if any data removed
	 */
	bool pruneData();

	/**
	 * \brief Evaluate hypocenter viability
	 *
	 * Evaluate whether the hypocenter is viable, first by checking to
	 * see if the current number of supporting data exceeds the configured
	 * threshold, second checking if the current bayes value exceeds
	 * the configured threshold, and finally making a depth/gap check to
	 * ensure the hypocenter is not a "whispy".
	 *
	 * \return Returns true if the hypocenter is not viable, false otherwise
	 */
	bool cancelCheck();

	/**
	 * \brief Evaluate hypocenter report suitability
	 *
	 * Evaluate whether the hypocenter is suitable to be reported, utilizing the
	 * reporting data and stack thresholds (instead of the nucleation thresholds)
	 *
	 * \return Returns true if the hypocenter can be reported, false otherwise
	 */
	bool reportCheck();

	/**
	 * \brief Calculate supporting data statistical values
	 *
	 * Calculate various statistical values for this hypo, including minimum
	 * distance, median distance, gap, kurtosis value, and association distance
	 * cutoff as part of the anneal() and localize()
	 */
	void calculateStatistics();

	/**
	 * \brief Fast baysian fit synthetic annealing location algorithm used by
	 * nucleation
	 *
	 * Rapid synthetic annealing algoritym used by nucleation to calculate an
	 * initial starting location.
	 *
	 * Also computes supporting data statistics by calling calculateStatistics()
	 *
	 * \param nIter - An integer containing the number of iterations to perform,
	 * defaults to 250
	 * \param dStart - A double value containing the starting distance iteration
	 * step size in kilometers, default 100 km
	 * \param dStop - A double value containing the ending distance iteration
	 * step size in kilometers, default 1 km
	 * \param tStart - A double value containing the starting time iteration
	 * step size in seconds, default 5 seconds
	 * \param tStop - A double value containing the ending time iteration
	 * step size in seconds, default 0.5 seconds
	 * \return Returns a double value containing the final baysian fit.
	 */
	double anneal(int nIter = 250, double dStart = 100.0, double dStop = 1.0,
					double tStart = 5., double tStop = .5);

	/**
	 * \brief Location calculation function
	 *
	 * This function calculates the current location of this hypo given the
	 * supporting data using either a maximum baysian fit (annealingLocateBayes)
	 * or minimum residual (annealingLocateResidual) depending on the
	 * configuration
	 *
	 * Also computes supporting data statistics by calling calculateStatistics()
	 *
	 * \return Returns a double value containing the final baysian fit.
	 */
	double localize();

	/**
	 * \brief Baysian Fit synthetic annealing location algorithm
	 *
	 * Locator which uses synthetic annealing to compute the geographic point
	 * of the maximum bayesian fit given the supporting data.
	 *
	 * \param nIter - An integer value containing the number of iterations
	 * \param dStart - A double value containing the distance starting value
	 * \param dStop - A double value containing the distance stopping value
	 * \param tStart - A double value containing the time starting value in
	 * gregorian seconds
	 * \param tStop - A double value containing the time stopping value in
	 * gregorian seconds
	 * \param nucleate - An boolean flag that sets if this is a nucleation which
	 * limits the phase used.
	 */
	void annealingLocateBayes(int nIter, double dStart, double dStop,
								double tStart, double tStop, bool nucleate =
										false);

	/**
	 * \brief Residual synthetic annealing location algorithm
	 *
	 * Locator which uses synthetic annealing to compute the geographic point
	 * of the minimum of sum of absolute of residuals given the supporting data.
	 *
	 * \param nIter - An integer value containing the number of iterations
	 * \param dStart - A double value containing the distance starting value
	 * \param dStop - A double value containing the distance stopping value
	 * \param tStart - A double value containing the time starting value in
	 * gregorian seconds
	 * \param tStop - A double value containing the time stopping value in
	 * gregorian seconds
	 * \param nucleate - A boolean flag that sets if this is a nucleation,
	 * which limits the phases used.
	 */
	void annealingLocateResidual(int nIter, double dStart, double dStop,
									double tStart, double tStop, bool nucleate =
											false);

	/**
	 * \brief Calculate gap
	 *
	 * Calculates the azimuthal gap for a given location using the supporting
	 * data
	 *
	 * \param lat - latitude of test location
	 * \param lon - longitude of test location
	 * \param z - depth of test location
	 * \return Returns a double value containing the calculated gap
	 */
	double calculateGap(double lat, double lon, double z);

	/**
	 * \brief Calculate bayes
	 *
	 * Calculates the total bayseian stack value for a given location using
	 * the supporting data. Used in calculating locations by
	 * annealingLocateBayes()
	 *
	 * \param xlat - A double of the latitude to evaluate
	 * \param xlon - A double of the longitude to evaluate
	 * \param xZ - A double of the depth to evaluate
	 * \param oT - A double of the oT to evaluate
	 * \param nucleate - A boolean flag that sets if this is a nucleation,
	 * which limits the phases used.
	 * \return Returns a double value containing the total bayseian stack value
	 * for the given location.
	 */
	double calculateBayes(double xlat, double xlon, double xZ, double oT,
							bool nucleate);

	/**
	 * \brief Calculate absolute residual sum
	 *
	 * Calculates the sum of the absolute residuals of the supporting data for a
	 * given location. Used in calculating locations by annealingLocateResidual()
	 *
	 * \param xlat - A double of the latitude to evaluate
	 * \param xlon - A double of the longitude to evaluate
	 * \param xZ - A double of the depth to evaluate
	 * \param oT - A double of the oT in gregorian seconds
	 * \param nucleate - A boolean flag that sets if this is a nucleation,
	 * which limits the phases used.
	 * \return Returns a double value containing the absolute residual sum for
	 * the given location.
	 */
	double calculateAbsResidualSum(double xlat, double xlon, double xZ,
									double oT, bool nucleate);

	/**
	 * \brief Calculate residual for a phase
	 * Calculates the weighted residual (with S down weighted) for the current
	 * location given the phase, observed travel time, and calculated travel
	 * time
	 *
	 * \param sPhase - A string with the phase type
	 * \param tObs - The observed travel time in gregorian seconds
	 * \param tCal - The calculated travel time in gregorian seconds
	 * \return Returns a double value containing the weighted residual
	 */
	double calculateWeightedResidual(std::string sPhase, double tObs,
										double tCal);

	/**
	 * \brief Write files for plotting output
	 *
	 */
	void graphicsOutput();

	/**
	 * \brief Ensure all supporting data belong to this hypo
	 *
	 * Search through all supporting data (eg. Picks) in the given hypocenter's
	 * lists, using the affinity functions to determine whether the data best
	 *  fits this hypocenter or not.
	 *
	 * \param hypo - A shared_ptr to a CHypo to use when adding references to
	 * this hypo. This parameter is passed because issues occurred using
	 * this-> to reference data.
	 * \param allowStealing - A boolean flag indicating whether to allow
	 * resolveData to steal data, defaults to true
	 * \return Returns true if the hypocenter's pick list was changed,
	 * false otherwise.
	 */
	bool resolveData(std::shared_ptr<CHypo> hypo, bool allowStealing = true);

	/**
	 * \brief Supporting data link checking function
	 *
	 * Causes CHypo to print any data in in it's lists that are either improperly
	 * linked or do not belong to this CHypo.
	 */
	void trap();

	/**
	 * \brief Get the azimuth taper used on the bayseian stack value in order
	 * to compensate for a large azimuthal gap
	 * \return Returns a double value containing the taper to use
	 */
	double getAzimuthTaper() const;

	/**
	 * \brief Get the maximum allowed depth for this hypo
	 * \return Returns a double value containing the maximum depth in kilometers
	 */
	double getMaxDepth() const;

	/**
	 * \brief Get the latitude for this hypo
	 * \return Returns a double containing the hypo latitude in degrees
	 */
	double getLatitude() const;

	/**
	 * \brief Set the latitude for this hypo
	 * \param lat - a double containing the hypo latitude in degrees
	 */
	void setLatitude(double lat);

	/**
	 * \brief Get the longitude for this hypo
	 * \return Returns a double containing the hypo longitude in degrees
	 */
	double getLongitude() const;

	/**
	 * \brief Set the longitude for this hypo, accounting for the longitude
	 * wrap at +/-180
	 * \param lon - a double containing the hypo longitude in degrees
	 */
	void setLongitude(double lon);

	/**
	 * \brief Get the depth for this hypo
	 * \return Returns a double containing the hypo depth in kilometers
	 */
	double getDepth() const;

	/**
	 * \brief Sets the depth for this hypo
	 * \param z - a double containing the hypo depth in kilometers
	 */
	void setDepth(double z);

	/**
	 * \brief Get the combined hypo location (latitude, longitude, depth) as
	 * a CGeo object
	 * \return Returns a glass3::util::Geo object containing the combined location.
	 */
	glass3::util::Geo getGeo() const;

	/**
	 * \brief Get the origin time for this hypo
	 * \return Returns a double containing the hypo origin time in julian seconds
	 */
	double getTOrigin() const;

	/**
	 * \brief Sets the origin time for this hypo
	 * \param newTOrg - a double containing the hypo origin time in julian seconds
	 */
	void setTOrigin(double newTOrg);

	/**
	 * \brief Gets the current bayes stack value for this hypo
	 * \return Returns a double containing the current bayes stack value
	 */
	double getBayesValue() const;

	/**
	 * \brief Gets whether a correlation has been added to this hypo.
	 *
	 * Gets whether a correlation has been added to this hypo.  This flag is used
	 * in preserving hypos generated from correlations long enough for supporting
	 * data to be added.
	 *
	 * \return Returns a boolean flag indicating whether a correlation has been
	 * added to this hypo, true if one has, false otherwise
	 */
	bool getCorrelationAdded() const;

	/**
	 * \brief Sets whether a correlation has been added to this hypo.
	 *
	 * Sets whether a correlation has been added to this hypo.  This flag is used
	 * in preserving hypos generated from correlations long enough for supporting
	 * data to be added.
	 *
	 * \param corrAdded - a boolean flag indicating whether a correlation has
	 * been added to this hypo, true if one has, false otherwise
	 */
	void setCorrelationAdded(bool corrAdded);

	/**
	 * \brief Gets whether an event message was generated for this hypo
	 *
	 * \return Returns a boolean flag indicating whether an event message has
	 * been generated for this hypo, true if one has, false otherwise
	 */
	bool getEventGenerated() const;

	/**
	 * \brief Gets whether an hypo message was generated for this hypo
	 *
	 * \return Returns a boolean flag indicating whether a hypo message has
	 * been generated for this hypo, true if one has, false otherwise
	 */
	bool getHypoGenerated() const;

	/**
	 * \brief Gets whether this hypo is fixed
	 *
	 * \return Returns a boolean flag indicating whether this hypo is fixed,
	 * true if it is, false otherwise
	 */
	bool getFixed() const;

	/**
	 * \brief Sets whether this hypo is fixed
	 *
	 * \param fixed - a boolean flag indicating whether this hypo is fixed,
	 * true if it is, false otherwise
	 */
	void setFixed(bool fixed);

	/**
	 * \brief Gets the initial (nucleation) bayes stack value for this hypo
	 * \return Returns a double value containing the initial (nucleation) bayes
	 * stack value
	 */
	double getInitialBayesValue() const;

	/**
	 * \brief Gets the association distance cutoff used in canAssociate() and
	 * generateAffinitu()
	 * \return Returns a double value containing the current association distance
	 * cutoff
	 */
	double getAssociationDistanceCutoff() const;

	/**
	 * \brief Gets the distance cutoff factor used in calculating the
	 * Association Distance Cutoff
	 * \return Returns a double value containing the distance cutoff factor
	 */
	double getDistanceCutoffFactor() const;

	/**
	 * \brief Gets the threshold that represents the minimum count of data
	 * required to successfully nucleate or maintain (via passing cancelCheck()
	 * a hypocenter.
	 *
	 * If threshold is 10, then 8 picks + 1 correlation + 1 beam would be
	 * sufficient, but 9 picks would not be.
	 *
	 * \return Returns a double value containing the nucleation minimum stack
	 * threshold
	 */
	int getNucleationDataThreshold() const;

	/**
	 * \brief Sets the threshold that represents the minimum count of data
	 * required to successfully nucleate or maintain (via passing cancelCheck()
	 * a hypocenter.
	 *
	 * If threshold is 10, then 8 picks + 1 correlation + 1 beam would be
	 * sufficient, but 9 picks would not be.
	 *
	 * \param cut - a double value containing the nucleation minimum stack
	 * threshold
	 */
	void setNucleationDataThreshold(int cut);

	/**
	 * \brief Gets the nucleation stack minimum threshold used in determining
	 * hypo viability in cancelCheck()
	 * \return Returns an integer value containing the nucleation data minimum
	 * threshold
	 */
	double getNucleationStackThreshold() const;

	/**
	 * \brief Sets the nucleation stack minimum threshold used in determining
	 * hypo viability in cancelCheck()
	 * \param thresh - an integer value containing the nucleation stack minimum
	 * threshold
	 */
	void setNucleationStackThreshold(double thresh);

	/**
	 * \brief Gets the azimuthal gap as of the last call of calculateStatistics
	 * \return Returns a double value containing the azimuthal gap
	 */
	double getGap() const;

	/**
	 * \brief Gets the median data distance as of the last call of
	 * calculateStatistics()
	 * \return Returns a double value containing the median data distance
	 */
	double getMedianDistance() const;

	/**
	 * \brief Gets the minimum data distance as of the last call of
	 * calculateStatistics()
	 * \return Returns a double value containing the minimum data distance
	 */
	double getMinDistance() const;

	/**
	 * \brief Gets the mutex used to ensure that the hypo can be processed by
	 * only one thread at a time
	 * \return Returns a std::mutex
	 */
	std::mutex & getProcessingMutex();

	/**
	 * \brief Gets the node resolution of the web that nucleated this hypo
	 * \return Returns a double value containing the node resolution of the web
	 * that nucleated this hypo in kilometers
	 */
	double getWebResolution() const;

	/**
	 * \brief Gets the current distance standard deviation
	 * \return Returns a double value containing the current distance standard
	 * deviation
	 */
	double getDistanceSD() const;

	/**
	 * \brief Gets the current process count, used by HypoList to prevent
	 * continuous hypo reprocessing
	 * \return Returns an integer value containing the current process count
	 */
	int getProcessCount() const;

	/**
	 * \brief Sets the process count, used by HypoList to prevent continuous
	 * hypo reprocessing
	 * \return newCycle - an integer value containing the new process count
	 */
	int setProcessCount(int newCycle);

	/**
	 * \brief Gets the total process count
	 * \return Returns an integer value containing the total process count
	 */
	int getTotalProcessCount() const;

	/**
	 * \brief Increments the total process count
	 * \return Returns an integer value containing the new total process count
	 */
	int incrementTotalProcessCount();

	/**
	 * \brief Gets the identifier for this hypo
	 * \return Returns a std::string containing this hypo's identifier
	 */
	const std::string& getID() const;

	/**
	 * \brief Gets the name of the web that nucleated this hypo
	 * \return Returns a std::string containing the name of the web that
	 * nuclated this hypo
	 */
	const std::string& getWebName() const;

	/**
	 * \brief Get the current size of the supporting data pick vector
	 * \return Returns an integer containing current size of the supporting data
	 * pick vector
	 */
	int getPickDataSize() const;

	/**
	 * \brief Get a vector containing all the supporting pick data for this hypo
	 * \return Returns a std::vector containing all the supporting pick data
	 */
	std::vector<std::shared_ptr<CPick>> getPickData() const;

	/**
	 * \brief Get the current size of the supporting data correlation vector
	 * \return Returns an integer containing current size of the supporting data
	 * correlation vector
	 */
	int getCorrelationDataSize() const;

	/**
	 * \brief Get the time that this hypo was created
	 * \return Returns a double value containg the time this hypo was created in
	 * julian seconds
	 */
	double getTCreate() const;

	/**
	 * \brief Get the primary phase/travel time used in nucleating this hypo
	 * This object is kept in CHypo for performance (throughput) reasons, and is
	 * used in the calculateBayes() and calculateAbsResidualSum() functions
	 *
	 * \return Returns a shared_ptr to the primary CTravelTime used in
	 * nucleating this hypo
	 */
	std::shared_ptr<traveltime::CTravelTime> getNucleationTravelTime1() const;

	/**
	 * \brief Get the secondary phase/travel time used in nucleating this hypo
	 * This object is kept in CHypo for performance (throughput) reasons, and is
	 * used in the calculateBayes() and calculateAbsResidualSum() functions
	 *
	 * \return Returns a shared_ptr to the secondary CTravelTime used in
	 * nucleating this hypo
	 */
	std::shared_ptr<traveltime::CTravelTime> getNucleationTravelTime2() const;

	/**
	 * \brief Get the list of association phases / travel times for this hypo
	 * This object is kept in CHypo for performance (throughput) reasons, and is
	 * used in canAssociate() calculateResidual(), and annealingLocateResidual()
	 * functions
	 *
	 * \return Returns a shared_ptr to the association CTTT used in this hypo
	 */
	std::shared_ptr<traveltime::CTTT> getTravelTimeTables() const;

	/**
	 * \brief Gets the number of times that this hypo has been reported
	 * \return Returns an integer value containing the report count
	 */
	int getReportCount() const;

	/**
	 * \brief Gets whether the mutex accessed via getProcessingMutex() is locked
	 * \return Returns a boolean flag indicating whether the mutex is locked,
	 * true if it is, false otherwise
	 */
	bool isLockedForProcessing();

	/**
	 * \brief Get the sorting time for this hypo
	 * \return Returns an int64_t containing the hypo sort time in julian seconds
	 */
	int64_t getTSort() const;

	/**
	 * \brief Set the sorting time for this hypo
	 * \param newTSort - a double containing the hypo sort time in julian seconds
	 */
	void setTSort(double newTSort);

 private:
	/**
	 * \brief  A std::string with the name of the web used during the nucleation
	 * process
	 */
	std::string m_sWebName;

	/**
	 * \brief An integer containing the number of stations needed to maintain
	 * association during the nucleation process
	 */
	std::atomic<int> m_iNucleationDataThreshold;

	/**
	 * \brief A double containing the subnet specific Bayesian stack threshold
	 * used during the nucleation process
	 */
	std::atomic<double> m_dNucleationStackThreshold;

	/**
	 * \brief A double value containing the the taper to use on the bayseian
	 * stack value in order to compensate for a large azimuthal gap
	 */
	std::atomic<double> m_dAzimuthTaper;

	/**
	 * \brief maximum allowable event depth
	 */
	std::atomic<double> m_dMaxDepth;

	/**
	 * \brief An integer value containing this hypo's processing cycle count
	 */
	std::atomic<int> m_iProcessCount;

	/**
	 * \brief A double value containing this hypo's origin time in julian
	 * seconds
	 */
	std::atomic<double> m_tOrigin;

	/**
	 * \brief A double value containing this hypo's latitude in degrees
	 */
	std::atomic<double> m_dLatitude;

	/**
	 * \brief A double value containing this hypo's longitude in degrees
	 */
	std::atomic<double> m_dLongitude;

	/**
	 * \brief A double value containing this hypo's depth in kilometers
	 */
	std::atomic<double> m_dDepth;

	/**
	 * \brief A boolean indicating if an Event message was sent for this hypo.
	 */
	std::atomic<bool> m_bEventGenerated;

	/**
	 * \brief A boolean indicating if an Event message was generated for this
	 * hypo.
	 */
	std::atomic<bool> m_bHypoGenerated;

	/**
	 * \brief A double value containing this hypo's Bayes statistic
	 */
	std::atomic<double> m_dBayesValue;

	/**
	 * \brief A double value containing this hypo's initial Bayes statistic
	 */
	std::atomic<double> m_dInitialBayesValue;

	/**
	 * \brief A double value containing this hypo's minimum distance in degrees
	 * as of the last call of calculateStatistics
	 */
	std::atomic<double> m_dMinDistance;

	/**
	 * \brief A double value containing this hypo's median distance in degrees
	 * as of the last call of calculateStatistics
	 */
	std::atomic<double> m_dMedianDistance;

	/**
	 * \brief A double value containing this hypo's maximum azimuthal gap in
	 * degrees as of the last call of calculateStatistics
	 */
	std::atomic<double> m_dGap;

	/**
	 * \brief A double value containing this hypo's distance standard deviation
	 */
	std::atomic<double> m_dDistanceSD;

	/**
	 * \brief A double value the resolution of the triggering web
	 */
	std::atomic<double> m_dWebResolution;

	/**
	 * \brief A double value containing this hypo's association distance cutoff
	 * (2.0 * 80 percentile) in degrees
	 */
	std::atomic<double> m_dAssociationDistanceCutoff;

	/**
	 * \brief A std::string containing this hypo's unique identifier
	 */
	std::string m_sID;

	/**
	 * \brief A boolean indicating if this hypo is fixed (not allowed to change)
	 */
	std::atomic<bool> m_bFixed;

	/**
	 * \brief A boolean indicating if a correlation was recently added to this
	 *  hypo.
	 */
	std::atomic<double> m_bCorrelationAdded;

	/**
	 * \brief An integer containing the number of times this hypo has been
	 * processed.
	 */
	std::atomic<int> m_iTotalProcessCount;

	/**
	 * \brief An integer containing the number of times this hypo has been
	 * reported.
	 */
	std::atomic<int> m_iReportCount;

	/**
	 * \brief A double value containing this hypo's creation time in julian
	 * seconds
	 */
	std::atomic<double> m_tCreate;

	/**
	 * \brief An int64_t value containing this hypo's sort time in julian
	 * seconds, this is a cached copy of tOrigin as an integer that is
	 * guaranteed to not change during the lifetime of the Hypo in a HypoList's
	 * internal multiset, ensuring that sort order won't change, even when
	 * tOrigin changes because of a relocation. Resorting is accomplished by
	 * removing the hypo from the internal multiset (NOT the HypoList), updating
	 * tSort to equal the current tOrigin, and then reinserting the hypo into
	 * the internal multiset. /see HypoList.
	 */
	std::atomic<int64_t> m_tSort;

	/**
	 * \brief A vector of shared_ptr's to the pick data that supports this hypo.
	 *
	 * We use shared_ptr's instead of weak ptr's in this vector so that any
	 * supporting picks will not be deleted before the hypo is canceled or
	 * expired.
	 */
	std::vector<std::shared_ptr<CPick>> m_vPickData;

	/**
	 * \brief A vector of shared pointers to correlation data that support
	 * this hypo.
	 *
	 * We use shared_ptr's instead of weak ptr's in this vector so that any
	 * supporting correlations will not be deleted before the hypo is canceled or
	 * expired.
	 */
	std::vector<std::shared_ptr<CCorrelation>> m_vCorrelationData;

	/**
	 * \brief A pointer to a CTravelTime object containing
	 * travel times for the first phase used to nucleate this hypo. This
	 * object is kept in CHypo for performance (throughput) reasons, and is
	 * used in the calculateBayes() and calculateAbsResidualSum() functions
	 */
	std::shared_ptr<traveltime::CTravelTime> m_pNucleationTravelTime1;

	/**
	 * \brief A pointer to a CTravelTime object containing
	 * travel times for the second phase used to nucleate this hypo. This
	 * object is kept in CHypo for performance (throughput) reasons, and is
	 * used in the calculateBayes() and calculateAbsResidualSum() functions
	 */
	std::shared_ptr<traveltime::CTravelTime> m_pNucleationTravelTime2;

	/**
	 * \brief A pointer to a CTTT object containing the travel
	 * time phases and branches used for association and location.  This
	 * object is kept in CHypo for performance (throughput) reasons, and is
	 * used in canAssociate() calculateResidual(), and annealingLocateResidual()
	 * functions
	 */
	std::shared_ptr<traveltime::CTTT> m_pTravelTimeTables;

	/**
	 * \brief A recursive_mutex to control threading access to CHypo.
	 * NOTE: recursive mutexes are frowned upon, so maybe redesign around it
	 * see: http://www.codingstandard.com/rule/18-3-3-do-not-use-stdrecursive_mutex/
	 * However a recursive_mutex allows us to maintain the original class
	 * design as delivered by the contractor.
	 */
	mutable std::recursive_mutex m_HypoMutex;

	/**
	 * \brief A mutex to control processing access to CHypo.
	 */
	std::mutex m_ProcessingMutex;
};
}  // namespace glasscore
#endif  // HYPO_H

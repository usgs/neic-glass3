/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef GLASS_H
#define GLASS_H

#include <json.h>
#include <string>
#include <memory>
// #include "Terra.h"
// #include "Ray.h"
#include "TTT.h"
#include "TravelTime.h"

namespace glasscore {

// forward declarations
class CWebList;
class CSiteList;
class CPickList;
class CHypoList;
class CDetection;
class CCorrelationList;
struct IGlassSend;

/**
 * \brief glasscore interface class
 *
 * The CGlass class is the class that sets up and maintains the glass
 * association engine, and acts as the interface between the glasscore library
 * and any clients.
 *
 * CGlass initializes the traveltime library, allocates the site, pick, and
 * hypo lists, creates and maintains the detection web, and manages
 * communication between glasscore and clients via the dispatch function
 * (receiving) and an IGlassSend interface pointer variable (sending).
 *
 * CGlass also performs traveltime library testing during initialization,
 * time encoding/decoding as well as calculating the significance functions
 * and normal distributions as needed.
 *
 * All communication (configuration, input data, or output results ) to / from
 * CGlass is via deserialized json messages as pointers to supereasyjson
 * json::objects.
 */
class CGlass {
 public:
	/**
	 * \brief CGlass constructor
	 *
	 * The constructor for the CGlass class.
	 * Sets allocated lists and objects to null.
	 * Initializes members to default values.
	 */
	CGlass();

	/**
	 * \brief CGlass destructor
	 *
	 * The destructor for the CGlass class.
	 * Cleans up all memory allocated to lists and objects.
	 */
	~CGlass();

	/**
	 * \brief CGlass communication receiving function
	 *
	 * The function used by CGlass to receive communication
	 * (such as configuration or input data), from outside the
	 * glasscore library.
	 *
	 * CGlass will forward the communication on to the pick, site,
	 * or hypo lists, or the detection web if CGlass cannot
	 * use the communication.
	 *
	 * \param com - A pointer to a json::object containing the
	 * communication.
	 * \return Returns true if the communication was handled by CGlass,
	 * false otherwise
	 */
	bool dispatch(std::shared_ptr<json::Object> com);

	/**
	 * \brief CGlass communication sending function
	 *
	 * The function used by CGlass to send communication
	 * (such as output data), to outside the glasscore library
	 * using an IGlassSend interface pointer.
	 *
	 * \param com - A pointer to a json::object containing the
	 * communication.
	 * \return Returns true if the communication was sent via
	 * a valid IGlassSend interface pointer, false otherwise
	 */
	bool send(std::shared_ptr<json::Object> com);

	/**
	 * \brief CGlass initialization function
	 *
	 * The function used by CGlass to initialize the glasscore
	 * library.  This function loads the earth model, sets the
	 * association parameters, sets up  the ray path calculator,
	 * creates the detection web configures and tests the phase
	 * and branch travel times used for association, creates the
	 * pick, site, and hypo lists, and sets up the output format
	 *
	 * \param com - A pointer to a json::object containing the
	 * configuration to use in initialization.
	 * \return Returns true if the initialization was successful,
	 * false otherwise
	 */
	bool initialize(std::shared_ptr<json::Object> com);

	/**
	 * \brief CGlass clear function
	 *
	 */
	void clear();

	/**
	 * \brief CGlass significance function
	 *
	 * This function calculates the significance function for glasscore,
	 * which is the bell shaped curve with sig(0, x) pinned to 0.
	 *
	 * \param tdif - A double containing x value.
	 * \param sig - A double value containing the sigma,
	 * \return Returns a double value containing significance function result
	 */
	double sig(double tdif, double sig);

	/**
	 * \brief CGlass laplacian significance function (PDF)
	 *
	 * This function calculates a laplacian significance used in associator.
	 * This should have the affect of being L1 normish, instead of L2 normish.
	 * Unlike the other significance function, this returns the PDF value
	 * \param tdif - A double containing x value.
	 * \param sig - A double value containing the sigma,
	 * \return Returns a double value containing significance function result
	 */
	double sig_laplace_pdf(double tdif, double sig);

	/**
	 * \brief An IGlassSend interface pointer used to send communication
	 * (such as output data), to outside the glasscore library
	 */
	glasscore::IGlassSend *piSend;

	/**
	 * \brief check to see if each thread is still functional
	 *
	 * Checks each thread to see if it is still responsive.
	 */
	bool healthCheck();

	/**
	 * \brief Beam matching azimuth window getter
	 * \return the beam matching azimuth window in degrees
	 */
	double getBeamMatchingAzimuthWindow() const;

	/**
	 * \brief Beam matching distance window getter
	 * \return the beam matching distance window in degrees
	 */
	double getBeamMatchingDistanceWindow() const;

	/**
	 * \brief Correlation cancel age getter
	 * \return the correlation cancel age in seconds
	 */
	int getCorrelationCancelAge() const;

	/**
	 * \brief Correlation matching time window getter
	 * \return the correlation matching time window in seconds
	 */
	double getCorrelationMatchingTimeWindow() const;

	/**
	 * \brief Correlation matching distance window getter
	 * \return the correlation matching distance window in degrees
	 */
	double getCorrelationMatchingDistanceWindow() const;

	/**
	 * \brief Distance cutoff factor getter
	 * \return the distance cutoff factor
	 */
	double getDistanceCutoffFactor() const;

	/**
	 * \brief Average distance cutoff minimum getter
	 * \return the minimum distance cutoff in degrees
	 */
	double getMinDistanceCutoff() const;

	/**
	 * \brief Distance cutoff percentage getter
	 * \return the distance cutoff percentage
	 */
	double getDistanceCutoffPercentage() const;

	/**
	 * \brief Report threshold getter
	 * \return the reporting viability threshold
	 */
	double getReportingStackThreshold() const;

	/**
	 * \brief Nucleation threshold getter
	 * \return the nucleation viability threshold
	 */
	double getNucleationStackThreshold() const;

	/**
	 * \brief Exponential Affinity getter
	 * \return the exponential factor used for pick affinity
	 */
	double getPickAffinityExpFactor() const;

	/**
	 * \brief Graphics output flag getter
	 * \return a flag indicating whether to output graphics files
	 */
	bool getGraphicsOut() const;

	/**
	 * \brief Graphics output folder getter
	 * \return the folder to output graphics files to
	 */
	const std::string& getGraphicsOutFolder() const;

	/**
	 * \brief Graphics step getter
	 * \return the graphics step size in km
	 */
	double getGraphicsStepKm() const;

	/**
	 * \brief Graphics steps getter
	 * \return the number of graphic steps
	 */
	int getGraphicsSteps() const;

	/**
	 * \brief Cycle limit getter
	 * \return the limit of processing cycles
	 */
	int getCycleLimit() const;

	/**
	 * \brief Graphics minimize TT locator getter
	 * \return the flag indicating whether to use the minimizing tt locator
	 */
	bool getMinimizeTTLocator() const;

	/**
	 * \brief Maximum number of correlations getter
	 * \return the maximum number of correlations
	 */
	int getMaxNumCorrelations() const;

	/**
	 * \brief Default number of detection stations getter
	 * \return the default number of detections used in a node
	 */
	int getNumStationsPerNode() const;

	/**
	 * \brief Maximum number of hypocenters getter
	 * \return the maximum number of hypocenters
	 */
	int getMaxNumHypos() const;

	/**
	 * \brief Default number of picks for nucleation getter
	 * \return the default number of nucleations used in for a detection
	 */
	int getNucleationDataThreshold() const;

	/**
	 * \brief Maximum number of picks getter
	 * \return the maximum number of picks
	 */
	int getMaxNumPicks() const;

	/**
	 * \brief Report cutoff getter
	 * \return the reporting cutoff
	 */
	double getReportingDataThreshold() const;

	/**
	 * \brief Maximum number of picks with a site getter
	 * \return the maximum number of picks stored with a site
	 */
	int getMaxNumPicksPerSite() const;

	/**
	 * \brief Correlation list getter
	 * \return a pointer to the correlation list
	 */
	CCorrelationList*& getCorrelationList();

	/**
	 * \brief Detection getter
	 * \return a pointer to the detection processor
	 */
	CDetection*& getDetectionProcessor();

	/**
	 * \brief Hypocenter list getter
	 * \return a pointer to the hypocenter list
	 */
	CHypoList*& getHypoList();

	/**
	 * \brief Pick duplicate time window getter
	 * \return the pick duplication time window in seconds
	 */
	double getPickDuplicateTimeWindow() const;

	/**
	 * \brief Pick list getter
	 * \return a pointer to the pick list
	 */
	CPickList*& getPickList();

	/**
	 * \brief Site list getter
	 * \return a pointer to the site list
	 */
	CSiteList*& getSiteList();

	/**
	 * \brief Default travel time  getter
	 * \return the default nucleation travel time
	 */
	std::shared_ptr<traveltime::CTravelTime>& getDefaultNucleationTravelTime();

	/**
	 * \brief Travel time list getter
	 * \return the list of association travel times
	 */
	std::shared_ptr<traveltime::CTTT>& getAssociationTravelTimes();

	/**
	 * \brief Web list getter
	 * \return a pointer to the web list
	 */
	CWebList*& getWebList();

	/**
	 * \brief SD associate getter
	 * \return the standard deviation cutoff used for association
	 */
	double getAssociationSDCutoff() const;

	/**
	 * \brief SD prune getter
	 * \return the standard deviation cutoff used for pruning
	 */
	double getPruningSDCutoff() const;

	/**
	 * \brief Testing locator flag getter
	 * \return a flag indicating whether to output locator testing files
	 */
	bool getTestLocator() const;

	/**
	 * \brief Testing travel times flag getter
	 * \return a flag indicating whether to output travel times testing files
	 */
	bool getTestTravelTimes() const;

 private:
	/**
	 * \brief A double value containing the default number of picks that
	 * that need to be gathered to trigger the nucleation of an event.
	 * This value can be overridden in a detection grid (Web) if provided as
	 * part of a specific grid setup.
	 */
	int m_iNucleationDataThreshold;

	/**
	 * \brief A double value containing the default viability threshold needed
	 * to exceed for a nucleation to be successful.
	 * This value can be overridden in a detection grid (Web) if provided as
	 * part of a specific grid setup.
	 */
	double m_dNucleationStackThreshold;

	/**
	 * \brief A double value containing the default number of closest stations
	 * to use  when generating a node for a detection array.
	 * This value can be overridden in a detection grid (Web) if provided as
	 * part of a specific grid setup.
	 */
	int m_iNumStationsPerNode;

	/**
	 * \brief A double value containing the standard deviation cutoff used for
	 * associating a pick with a hypocenter.
	 */
	double m_dAssociationSDCutoff;

	/**
	 * \brief A double value containing the standard deviation cutoff used for
	 * pruning a pick from a hypocenter.
	 */
	double m_dPruningSDCutoff;

	/**
	 * \brief A double value containing the exponential factor used when
	 * calculating the affinity of a pick with a hypocenter.
	 */
	double m_dPickAffinityExpFactor;

	/**
	 * \brief A double value containing the factor used to calculate a hypo's
	 * distance cutoff
	 */
	double m_dDistanceCutoffFactor;

	/**
	 * \brief A double value containing the percentage used to calculate a
	 *  hypo's distance cutoff
	 */
	double m_dDistanceCutoffPercentage;

	/**
	 * \brief A double value containing the minimum distance cutoff in degrees
	 */
	double m_dMinDistanceCutoff;

	/**
	 * \brief A pointer to a CWeb object containing the detection web
	 */
	CWebList * m_pWebList;

	/**
	 * \brief A pointer to a CTravelTime object containing
	 *default travel time for nucleation
	 */
	std::shared_ptr<traveltime::CTravelTime> m_pDefaultNucleationTravelTime;

	/**
	 * \brief A pointer to a CTTT object containing the travel
	 * time phases and branches used by glasscore for association
	 */
	std::shared_ptr<traveltime::CTTT> m_pAssociationTravelTimes;

	/**
	 * \brief the std::mutex for traveltimes
	 */
	mutable std::mutex m_TTTMutex;

	/**
	 * \brief A pointer to a CSiteList object containing all the sites
	 * known to glasscore
	 */
	CSiteList * m_pSiteList;

	/**
	 * \brief A pointer to a CPickList object containing the last n picks sent
	 * into glasscore (as determined by nPickMax).  Picks passed into pPickList
	 * are also passed into the pWeb detection web
	 */
	CPickList * m_pPickList;

	/**
	 * \brief A pointer to a CPickList object containing the last n hypos sent
	 * into glasscore (as determined by nPickMax)
	 */
	CHypoList * m_pHypoList;

	/**
	 * \brief A pointer to a CCorrelationList object containing the last n
	 * correlations sent into glasscore
	 */
	CCorrelationList * m_pCorrelationList;

	/**
	 * \brief A pointer to a CDetection object used to process detections sent
	 * into glasscore
	 */
	CDetection * m_pDetectionProcessor;

	/**
	 * \brief An integer containing the maximum number of picks stored by
	 * pPickList
	 */
	int m_iMaxNumPicks;

	/**
	 * \brief An integer containing the maximum number of correlations stored by
	 * pCorrelationList
	 */
	int m_iMaxNumCorrelations;

	/**
	 * \brief An integer containing the maximum number of picks stored by
	 * the vector in a site
	 */
	int m_iMaxNumPicksPerSite;

	/**
	 * \brief An integer containing the maximum number of hypocenters stored by
	 * pHypoList
	 */
	int m_iMaxNumHypos;

	/**
	 * \brief Window in seconds to check for 'duplicate' picks at same station.
	 * If new pick is within window, it isn't added to pick list.
	 */
	double m_dPickDuplicateTimeWindow;

	/**
	 * \brief Time Window to check for matching correlations in seconds. Used
	 * for checking for duplicate correlations and associating correlations to
	 * hypos
	 */
	double m_dCorrelationMatchingTimeWindow;

	/**
	 * \brief Distance Window to check for matching correlations in degrees.
	 * Used for checking for duplicate correlations and associating correlations
	 * to hypos
	 */
	double m_dCorrelationMatchingDistanceWindow;

	/**
	 * \brief age of correlations before allowing cancel in seconds
	 */
	int m_iCorrelationCancelAge;

	/**
	 * \brief Azimuth Window to check for matching beams in degrees. Used for
	 * nucleating beams and associating beams to hypos
	 */
	double m_dBeamMatchingAzimuthWindow;

	/**
	 * \brief Distance Window to check for matching beams in degrees. Used for
	 * nucleating beams and associating beams to hypos
	 */
	double m_dBeamMatchingDistanceWindow;

	/**
	 * \brief Bool to decide when to print out travel-times.
	 */
	bool m_bTestTravelTimes;

	/**
	 * \brief Bool to decide when to print files for locator test
	 */
	bool m_bTestLocator;

	/**
	 * \brief Flag indicating whether to output info for graphics.
	 */
	bool m_bGraphicsOut;

	/**
	 * \brief Output locations info for graphics.
	 */
	std::string m_sGraphicsOutFolder;

	/**
	 * \brief For graphics, the step size for output.
	 */
	double m_dGraphicsStepKM;

	/**
	 * \brief For graphics, the number of steps from hypocenter.
	 */
	int m_iGraphicsSteps;

	/**
	 * \brief Maximum number of processing cycles a hypo can do without having
	 * new data associated
	 */
	int m_iCycleLimit;

	/**
	 * \brief boolean to use a locator which minimizes TT as opposed to
	 * maximizes significance functions
	 */
	bool m_bMinimizeTTLocator;

	/**
	 * \brief number of data required for reporting a hypo
	 */
	int m_iReportingDataThreshold;

	/**
	 * \brief A double value containing the default viability threshold needed
	 * to for reporting a hypo
	 */
	double m_dReportingStackThreshold;
};
}  // namespace glasscore
#endif  // GLASS_H

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
#include "Terra.h"
#include "Ray.h"
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
	bool dispatch(json::Object *com);

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
	bool send(json::Object *com);

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
	bool initialize(json::Object *com);

	/**
	 * \brief CGlass clear function
	 *
	 */
	void clear();

	/**
	 * \brief CGlass input synchronization confirmation function
	 *
	 * The function used by CGlass to confirm that glasscore has processed
	 * all previously sent input data.  Once glasscore has completed
	 * processing all input, a confirmation "pong" message is sent.
	 *
	 * \param com - A pointer to a json::object containing the
	 * confirmation query.
	 * \return Always returns true
	 */
	bool ping(json::Object *com);

	/**
	 * \brief CGlass earth model test function
	 *
	 * This function is used by CGlass to test the loading and
	 * functionality of the earth model file and classes
	 *
	 * \param com - A pointer to a json::object containing the
	 * the earth model configuration.
	 * \return Returns true if the tests are successful, false
	 * otherwise.
	 */
	// bool test(json::Object *com);
	/**
	 * \brief CGlass earth model travel time generator
	 *
	 * This function is used to generate .trv files that
	 * are used by CTrv to calculate travel times during
	 * the real-time event processing
	 *
	 * \param com - A pointer to a json::object containing the
	 * the descriptor for a branch of the travel time curves.
	 * \return Returns true if the tests are successful, false
	 * otherwise.
	 */
	// bool genTrv(json::Object *com);
	/**
	 * \brief CGlass travel time testing function
	 *
	 * This function is used by CGlass to test various travel
	 * time phases and branches at specified distances
	 */
	// void testTTT(json::Object *com);
	/**
	 * \brief CGlass significance function
	 *
	 * This function calculates the significance function for glasscore,
	 * which is the bell shaped curve with Sig(0, x) pinned to 0.
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
	 * \brief check to see if each thread is still functional
	 *
	 * Checks each thread to see if it is still responsive.
	 */
	bool statusCheck();

	// Local Attributes
	/**
	 * \brief A std::string used in debugging to keep track of the id of the
	 * last pick added to glass.
	 */
	std::string sTrack;

	/**
	 * \brief A boolean set to true to turn on process tracking using
	 * printf strings (saved in sTrack)
	 */
	bool bTrack;

	/**
	 * \brief A double value containing the default number of picks that
	 * that need to be gathered to trigger the nucleation of an event.
	 * This value can be overridden in a detection grid (Web) if provided as
	 * part of a specific grid setup.
	 */
	int nNucleate;

	/**
	 * \brief A double value containing the default number of closest stations
	 * to use  when generating a node for a detection array.
	 * This value can be overridden in a detection grid (Web) if provided as
	 * part of a specific grid setup.
	 */
	int nDetect;

	/**
	 * \brief A double value containing the default viability threshold needed
	 * to exceed for a nucleation to be successful.
	 * This value can be overridden in a detection grid (Web) if provided as
	 * part of a specific grid setup.
	 */
	double dThresh;

	/**
	 * \brief A double value containing the standard deviation cutoff used for
	 * associating a pick with a hypocenter.
	 */
	double sdAssociate;

	/**
	 * \brief A double value containing the standard deviation cutoff used for
	 * pruning a pick from a hypocenter.
	 */
	double sdPrune;

	/**
	 * \brief A double value containing the exponential factor used when
	 * calculating the affinity of a pick with a hypocenter.
	 */
	double expAffinity;

	/**
	 * \brief A double value containing the average station distance in degrees,
	 * used as the defining value for a taper compensate for station density in
	 * Hypo::weights()
	 */
	double avgDelta;

	/**
	 * \brief A double value containing the exponent of the gaussian weighting
	 * kernel in degrees.  It is used to compensate for station density in
	 * Hypo::weights()
	 */
	double avgSigma;

	/**
	 * \brief A double value containing the factor used to calculate a hypo's
	 * distance cutoff
	 */
	double dCutFactor;

	/**
	 * \brief A double value containing the percentage used to calculate a
	 *  hypo's distance cutoff
	 */
	double dCutPercentage;

	/**
	 * \brief A double value containing the minimum distance cutoff
	 */
	double dCutMin;

	/**
	 * \brief An IGlassSend interface pointer used to send communication
	 * (such as output data), to outside the glasscore library
	 */
	glasscore::IGlassSend *piSend;

	/**
	 * \brief A pointer to a CWeb object containing the detection web
	 */
	CWebList *pWebList;

	/**
	 * \brief A pointer to a CTravelTime object containing
	 *default travel time for nucleation
	 */
	std::shared_ptr<traveltime::CTravelTime> pTrvDefault;

	/**
	 * \brief A pointer to a CTTT object containing the travel
	 * time phases and branches used by glasscore for association
	 */
	traveltime::CTTT *pTTT;

	/**
	 * \brief the std::mutex for traveltimes
	 */
	std::mutex m_TTTMutex;

	/**
	 * \brief A pointer to a CSiteList object containing all the sites
	 * known to glasscore
	 */
	CSiteList *pSiteList;

	/**
	 * \brief A pointer to a CPickList object containing the last n picks sent
	 * into glasscore (as determined by nPickMax).  Picks passed into pPickList
	 * are also passed into the pWeb detection web
	 */
	CPickList *pPickList;

	/**
	 * \brief A pointer to a CPickList object containing the last n hypos sent
	 * into glasscore (as determined by nPickMax)
	 */
	CHypoList *pHypoList;

	/**
	 * \brief A pointer to a CCorrelationList object containing the last n
	 * correlations sent into glasscore
	 */
	CCorrelationList *pCorrelationList;

	/**
	 * \brief A pointer to a CDetection object  used to process detections sent
	 * into glasscore
	 */
	CDetection *pDetection;

	/**
	 * \brief An integer containing the maximum number of picks stored by
	 * pPickList
	 */
	int nPickMax;

	/**
	 * \brief An integer containing the maximum number of correlations stored by
	 * pCorrelationList
	 */
	int nCorrelationMax;

	/**
	 * \brief An integer containing the maximum number of picks stored by
	 * the vector in a site
	 */
	int nSitePickMax;

	/**
	 * \brief An integer containing the maximum number of hypocenters stored by
	 * pHypoList
	 */
	int nHypoMax;

	/**
	 * \brief An integer containing the count of the number of times
	 * Hypo::iterate is called. Used for debugging.
	 */
	int nIterate;

	/**
	 * \brief An integer containing the count of the number of times
	 * Hypo::localize is called. Used for debugging.
	 */
	int nLocate;

	/**
	 * \brief A string containing the most recent nucleated web. Used for
	 * debugging.
	 */
	std::string sWeb;

	/**
	 * \brief An integer containing the count of supporting stations for the most
	 * recently nucleated node. Used for debugging.
	 */
	int nCount;

	/**
	 * \brief A double containing the Bayesian sum for the most recently
	 * nucleated node. Used for debugging.
	 */
	double dSum;

	/**
	 * \brief Window to check for 'duplicate' picks at same station. If new pick is
	 * within window, it isn't added to pick list.
	 */
	double pickDuplicateWindow;

	/**
	 * \brief Time Window to check for matching correlations. Used for checking
	 * for duplicate correlations and associating correlations to hypos
	 */
	double correlationMatchingTWindow;

	/**
	 * \brief Distance Window to check for matching correlations. Used for
	 * checking for duplicate correlations and associating correlations to hypos
	 */
	double correlationMatchingXWindow;

	/**
	 * \brief Azimuth Window to check for matching beams. Used for
	 * nucleating beams and associating beams to hypos
	 */
	double beamMatchingAzimuthWindow;

	/**
	 * \brief Distance Window to check for matching beams. Used for
	 * nucleating beams and associating beams to hypos
	 */
	double beamMatchingDistanceWindow;

	/**
	 * \brief age of correlations before allowing cancel in seconds
	 */
	int correlationCancelAge;

	/**
	 * \brief Bool to decide when to print out travel-times.
	 */
	bool testTimes;

	/**
	 * \brief Bool to decide when to print files for locator test
	 */
	bool testLocator;

	/**
	 * \brief Output info for graphics.
	 */
	bool graphicsOut;

	/**
	 * \brief Output locations info for graphics.
	 */
	std::string graphicsOutFolder;

	/**
	 * \brief For graphics, the step size for output.
	 */
	double graphicsStepKM;

	/**
	 * \brief For graphics, the number of steps from hypocenter.
	 */
	int graphicsSteps;

	/**
	 * \brief Maximum number of processing cycles a hypo can do without having
	 * new data associated
	 */
	int iCycleLimit;

	/**
	 * \brief boolean to use a locator which minimizes TT as opposed to
	 * maximizes significance functions
	 */
	bool minimizeTTLocator;

	/**
	 * \brief number of data required for reporting a hypo
	 */
	double nReportCut;

	/**
	 * \brief A double value containing the default viability threshold needed
	 * to for reporting a hypo
	 */
	double dReportThresh;
};
}  // namespace glasscore
#endif  // GLASS_H

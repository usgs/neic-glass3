#include "Hypo.h"
#include <json.h>
#include <date.h>
#include <logger.h>
#include <taper.h>
#include <glassid.h>
#include <glassmath.h>
#include <geo.h>
#include <cmath>
#include <string>
#include <algorithm>
#include <memory>
#include <vector>
#include <mutex>
#include <fstream>
#include <limits>
#include "TTT.h"
#include "Site.h"
#include "PickList.h"
#include "HypoList.h"
#include "Pick.h"
#include "Correlation.h"
#include "Trigger.h"
#include "Web.h"
#include "Glass.h"
#include "SiteList.h"

namespace glasscore {

// constants
constexpr double CHypo::k_dResidualSigmaLengthSeconds;
constexpr double CHypo::k_dGapTaperDownBegin;
constexpr double CHypo::k_dGapTaperDownEnd;
constexpr double CHypo::k_dTimeToDistanceCorrectionFactor;
constexpr double CHypo::k_dVerticalToHorizontalDistanceCorrectionFactor;
constexpr double CHypo::k_dBayesFactorMaximumRange;
constexpr double CHypo::k_dBayesFactorExponent;
constexpr double CHypo::k_dBayesFactorStepSizeReduction;
constexpr double CHypo::k_dLocationChangeWebResolutionRatio;
constexpr double CHypo::k_dMinimumDepthChangeKMThreshold;
constexpr double CHypo::k_dMinimumDepthChangeRatioThreshold;
constexpr double CHypo::k_dInitialAnnealStepReducationFactor;
constexpr double CHypo::k_dFinalAnnealStepReducationFactor;
constexpr double CHypo::k_dLocationMinDistanceStepSize;
constexpr double CHypo::k_dLocationMinTimeStepSize;
constexpr double CHypo::k_dLocationSearchRadiusToTime;
constexpr double CHypo::k_dLocationTaperConstant;
constexpr double CHypo::k_dLocationMaxTaperThreshold;
const int CHypo::k_iLocationNPicksToSkipSmall;
const int CHypo::k_iLocationNPicksToSkipMedium;
const int CHypo::k_iLocationNPickThresholdSmall;
const int CHypo::k_iLocationNPickThresholdMedium;
const int CHypo::k_iLocationNPickThresholdLarge;
const int CHypo::k_iLocationNumIterationsSmall;
const int CHypo::k_iLocationNumIterationsMedium;
const int CHypo::k_iLocationNumIterationsLarge;
constexpr double CHypo::k_dSearchRadiusResolutionFactor;
constexpr double CHypo::k_dSearchRadiusTaperFactor;
constexpr double CHypo::k_dSearchRadiusFactor;

// ---------------------------------------------------------CHypo
CHypo::CHypo() {
	clear();
}

// ---------------------------------------------------------CHypo
CHypo::CHypo(double lat, double lon, double z, double time, std::string pid,
				std::string web, double bayes, double thresh, int cut,
				std::shared_ptr<traveltime::CTravelTime> firstTrav,
				std::shared_ptr<traveltime::CTravelTime> secondTrav,
				std::shared_ptr<traveltime::CTTT> ttt, double resolution,
				double aziTap, double maxDep) {
	if (!initialize(lat, lon, z, time, pid, web, bayes, thresh, cut, firstTrav,
					secondTrav, ttt, resolution, aziTap, maxDep)) {
		clear();
	}
}

// ---------------------------------------------------------CHypo
CHypo::CHypo(std::shared_ptr<json::Object> detection, double thresh, int cut,
				std::shared_ptr<traveltime::CTravelTime> firstTrav,
				std::shared_ptr<traveltime::CTravelTime> secondTrav,
				std::shared_ptr<traveltime::CTTT> ttt, double resolution,
				double aziTap, double maxDep, CSiteList *pSiteList) {
	// null check json
	if (detection == NULL) {
		glass3::util::Logger::log("error",
									"CHypo::CHypo: NULL json communication.");
		return;
	}

	// check type
	if (detection->HasKey("Type")
			&& ((*detection)["Type"].GetType() == json::ValueType::StringVal)) {
		std::string type = (*detection)["Type"].ToString();

		if (type != "Detection") {
			glass3::util::Logger::log(
					"warning",
					"CHypo::CHypo: Non-Detection message passed in.");
			return;
		}
	} else {
		glass3::util::Logger::log("error",
									"CHypo::CHypo: Missing required Type Key.");
		return;
	}

	// detection definition variables
	double time = 0;
	double lat = 0;
	double lon = 0;
	double z = 0;
	double bayes = 0;

	// Get information from hypocenter
	if (detection->HasKey("Hypocenter")
			&& ((*detection)["Hypocenter"].GetType()
					== json::ValueType::ObjectVal)) {
		json::Object hypocenter = (*detection)["Hypocenter"].ToObject();

		// get time from hypocenter
		if (hypocenter.HasKey("Time")
				&& (hypocenter["Time"].GetType() == json::ValueType::StringVal)) {
			// get time string
			std::string tiso = hypocenter["Time"].ToString();

			// convert time
			glass3::util::Date dt = glass3::util::Date();
			time = dt.decodeISO8601Time(tiso);
		} else {
			glass3::util::Logger::log(
					"error",
					"CHypo::CHypo: Missing required Hypocenter Time Key.");

			return;
		}

		// get latitude from hypocenter
		if (hypocenter.HasKey("Latitude")
				&& (hypocenter["Latitude"].GetType()
						== json::ValueType::DoubleVal)) {
			lat = hypocenter["Latitude"].ToDouble();

		} else {
			glass3::util::Logger::log(
					"error",
					"CHypo::CHypo: Missing required Hypocenter Latitude"
					" Key.");

			return;
		}

		// get longitude from hypocenter
		if (hypocenter.HasKey("Longitude")
				&& (hypocenter["Longitude"].GetType()
						== json::ValueType::DoubleVal)) {
			lon = hypocenter["Longitude"].ToDouble();
		} else {
			glass3::util::Logger::log(
					"error",
					"CHypo::CHypo: Missing required Hypocenter Longitude"
					" Key.");

			return;
		}

		// get depth from hypocenter
		if (hypocenter.HasKey("Depth")
				&& (hypocenter["Depth"].GetType() == json::ValueType::DoubleVal)) {
			z = hypocenter["Depth"].ToDouble();
		} else {
			glass3::util::Logger::log(
					"error", "CHypo::CHypo: Missing required Hypocenter Depth"
					" Key.");

			return;
		}
	} else {
		glass3::util::Logger::log(
				"error", "CHypo::CHypo: Missing required Hypocenter Key.");

		return;
	}

	// get bayes
	if (detection->HasKey("Bayes")
			&& ((*detection)["Bayes"].GetType() == json::ValueType::DoubleVal)) {
		bayes = (*detection)["Bayes"].ToDouble();

	} else {
		glass3::util::Logger::log(
				"error", "CHypo::CHypo: Missing required Hypocenter Latitude"
				" Key.");

		return;
	}

	if (!initialize(lat, lon, z, time, glass3::util::GlassID::getID(),
					"Detection", bayes, thresh, cut, firstTrav, secondTrav, ttt,
					resolution, aziTap, maxDep)) {
		clear();
	}

	// if we have a sitelist
	if (pSiteList != NULL) {
		// if we have supporting data
		if (detection->HasKey("Data")
				&& ((*detection)["Data"].GetType() == json::ValueType::ArrayVal)) {
			json::Array data = (*detection)["Data"].ToArray();

			// for cach data we dound
			for (int i = 0; i < data.size(); i++) {
				std::shared_ptr<json::Object> aData = std::make_shared
						< json::Object > (json::Object(data[i]));

				// check for type
				std::string type = "";
				if (aData->HasKey("Type")
						&& ((*aData)["Type"].GetType()
								== json::ValueType::StringVal)) {
					type = (*aData)["Type"].ToString();
				} else {
					continue;
				}

				// convert by type
				if (type == "Pick") {
					CPick * newPick = new CPick(aData, pSiteList);

					// create new shared pointer to this pick
					std::shared_ptr<CPick> pck(newPick);

					// add to hypo
					addPickReference(pck);
				} else if (type == "Correlation") {
					CCorrelation * newCorr = new CCorrelation(aData, pSiteList);

					// create new shared pointer to this correlation
					std::shared_ptr<CCorrelation> cor(newCorr);

					// add to hypo
					addCorrelationReference(cor);
				}  // convert by type
			}  // for each data
		}  // if we have data
	}  // if we have a sitelist
}

// ---------------------------------------------------------CHypo
CHypo::CHypo(std::shared_ptr<CTrigger> trigger,
				std::shared_ptr<traveltime::CTTT> ttt) {
	// null checks
	if (trigger == NULL) {
		glass3::util::Logger::log("error", "CHypo::CHypo: NULL node.");

		clear();
		return;
	}

	if (trigger->getWeb() == NULL) {
		glass3::util::Logger::log("error",
									"CHypo::CHypo: NULL trigger->getWeb().");

		clear();
		return;
	}

	if (!initialize(trigger->getLatitude(), trigger->getLongitude(),
					trigger->getDepth(), trigger->getTOrigin(),
					glass3::util::GlassID::getID(),
					trigger->getWeb()->getName(), trigger->getBayesValue(),
					trigger->getWeb()->getNucleationStackThreshold(),
					trigger->getWeb()->getNucleationDataCountThreshold(),
					trigger->getWeb()->getNucleationTravelTime1(),
					trigger->getWeb()->getNucleationTravelTime2(), ttt,
					trigger->getWebResolution(),
					trigger->getWeb()->getAzimuthTaper(),
					trigger->getWeb()->getMaxDepth())) {
		clear();
	}
}

// ---------------------------------------------------------CHypo
CHypo::CHypo(std::shared_ptr<CCorrelation> corr,
				std::shared_ptr<traveltime::CTravelTime> firstTrav,
				std::shared_ptr<traveltime::CTravelTime> secondTrav,
				std::shared_ptr<traveltime::CTTT> ttt) {
	m_pTravelTimeTables = NULL;
	m_pNucleationTravelTime1 = NULL;
	m_pNucleationTravelTime2 = NULL;

	// null checks
	if (corr == NULL) {
		glass3::util::Logger::log("error", "CHypo::CHypo: NULL correlation.");

		clear();
		return;
	}

	if (!initialize(corr->getLatitude(), corr->getLongitude(), corr->getDepth(),
					corr->getTOrigin(), glass3::util::GlassID::getID(),
					"Correlation", 0.0, 0.0, 0.0, firstTrav, secondTrav, ttt,
					0.0)) {
		clear();
	}
}

// ---------------------------------------------------------~CHypo
CHypo::~CHypo() {
}

// ------------------------------------------------------addCorrelationReference
void CHypo::addCorrelationReference(std::shared_ptr<CCorrelation> corr) {
	// null check
	if (corr == NULL) {
		glass3::util::Logger::log("warning",
									"CHypo::addCorrelation: NULL correlation.");
		return;
	}

	// lock mutex for this scope
	std::lock_guard < std::recursive_mutex > guard(m_HypoMutex);

	// for each correlation in the vector
	for (auto q : m_vCorrelationData) {
		// see if we have this same correlation
		// NOTE: this only checks by ID, need to improve this to
		// an ID/Source check
		if (q->getID() == corr->getID()) {
			char sLog[glass3::util::Logger::k_nMaxLogEntrySize];
			snprintf(sLog, sizeof(sLog),
						"CHypo::addCorrelation: ** Duplicate correlation %s",
						corr->getSite()->getSCNL().c_str());
			glass3::util::Logger::log(sLog);

			return;
		}
	}

	// add the pick to the vector.
	m_vCorrelationData.push_back(corr);
	m_bCorrelationAdded = true;
}

// ---------------------------------------------------------addPickReference
bool CHypo::addPickReference(std::shared_ptr<CPick> pck) {
	// null check
	if (pck == NULL) {
		glass3::util::Logger::log("warning",
									"CHypo::addPickReference: NULL pck.");
		return (false);
	}

	// lock mutex for this scope
	std::lock_guard < std::recursive_mutex > guard(m_HypoMutex);

	// for each pick in the vector
	for (auto aPick : m_vPickData) {
		// see if we have this same pick
		if (aPick->getID() == pck->getID()) {
			// Don't add this duplicate pick
			return(false);
		}
	}

	// add the pick to the vector.
	m_vPickData.push_back(pck);
	return(true);
}

// ---------------------------------------------------------calculateAffinity
double CHypo::calculateAffinity(std::shared_ptr<CPick> pck) {
	// null checks
	if (pck == NULL) {
		return (0.0);
	}

	// lock mutex for this scope
	std::lock_guard < std::recursive_mutex > guard(m_HypoMutex);

	// get the site from the pick
	std::shared_ptr<CSite> site = pck->getSite();

	// get various global parameters from the glass pointer
	// get the standard deviation allowed for association
	double sdassoc = CGlass::getAssociationSDCutoff();

	// get the affinity factor
	double expaff = CGlass::getPickAffinityExpFactor();

	// check to see if this pick can  associate with this hypo using
	// the given association standard deviation
	if (!canAssociate(pck, k_dResidualSigmaLengthSeconds, sdassoc)) {
		// the pick did not associate
		return (0.0);
	}

	// get pick residual
	// double tRes = calculateResidual(pck);

	// get absolute residual
	// if (tRes < 0.0) {
	// 	tRes = -tRes;
	// }

	// WLY - I removed this because it was leading to small
	// events trading picks and canceling....
	// stack accounts for residual anyways
	// compute a weight factor based on residual
	// weight goes from 0.25 at a residual of sdassoc to 1.
	// glass3::util::Taper resWeight;
	// resWeight = glass3::util::Taper(-1., -1., sdassoc/2., sdassoc);
	// double resWeightFactor = (resWeight.calculateValue(tRes) * 0.75) + 0.25;

	// now compute the gap factor using a taper
	glass3::util::Taper gap;
	gap = glass3::util::Taper(0.0, 0.0, k_dGapTaperDownBegin,
								k_dGapTaperDownEnd);
	double gapfac = gap.calculateValue(m_dGap);

	// compute the affinity of this pick to this hypo by multiplying
	// the gap factor to the hypocenter's current baysian statistic to
	// the affinity power factor
	double aff = gapfac * pow(m_dBayesValue, expaff);

	// return the affinity
	return (aff);
}

// ---------------------------------------------------------calculateAffinity
double CHypo::calculateAffinity(std::shared_ptr<CCorrelation> corr) {
	// lock mutex for this scope
	std::lock_guard < std::recursive_mutex > guard(m_HypoMutex);

	// NOTE: I'm just combining time/distance into a made up affinity
	// wiser heads than mine may come up with a more robust approach JMP
	// null checks
	if (corr == NULL) {
		return (0.0);
	}

	// get various global parameters from the glass pointer
	double tWindow = CGlass::getCorrelationMatchingTimeWindow();
	double xWindow = CGlass::getCorrelationMatchingDistanceWindow();

	// check to see if this correlation can associate with this hypo
	if (!canAssociate(corr, tWindow, xWindow)) {
		// the correlation did not associate
		return (0.0);
	}

	// compute time factor, multiply by 10 to make time factor
	// have equal weight to the distance factor
	double tFactor = std::abs(m_tOrigin - corr->getTCorrelation())
			* k_dTimeToDistanceCorrectionFactor;

	// hypo is in geographic coordinates
	glass3::util::Geo geo1;
	geo1.setGeographic(m_dLatitude, m_dLongitude,
						glass3::util::Geo::k_EarthRadiusKm - m_dDepth);

	// correlation is in geographic coordinates
	glass3::util::Geo geo2;
	geo2.setGeographic(corr->getLatitude(), corr->getLongitude(),
						glass3::util::Geo::k_EarthRadiusKm - corr->getDepth());

	// compute distance factor
	double xFactor = glass3::util::GlassMath::k_RadiansToDegrees
			* geo1.delta(&geo2);

	// compute the affinity of this correlation to this hypo by taking the
	// inverse of multiplying the time factor to the distance filter
	// note the idea is a higher number the closer the correlation is
	// to the hypo
	double aff = 1 / (tFactor * xFactor);

	// return the affinity
	return (aff);
}

// ---------------------------------------------------------anneal
double CHypo::anneal(int nIter, double dStart, double dStop, double tStart,
						double tStop) {
	// lock mutex for this scope
	std::lock_guard < std::recursive_mutex > guard(m_HypoMutex);

	// This is essentially a faster algorithmic implementation of iterate
	// *** First, locate ***
	if (CGlass::getMinimizeTTLocator() == false) {
		annealingLocateBayes(nIter, dStart, dStop, tStart, tStop, true);
	} else {
		annealingLocateResidual(nIter, dStart, dStop, tStart, tStop, true);
	}
	// *** Second, based on the new location/depth/time, remove ill fitting
	// picks ***

	// compute current stats after location
	calculateStatistics();

	// create pick delete vector
	std::vector < std::shared_ptr < CPick >> vRemovePicks;

	// set the traveltime for the current hypo
	if (m_pNucleationTravelTime1 != NULL) {
		m_pNucleationTravelTime1->setTTOrigin(m_dLatitude, m_dLongitude,
												m_dDepth);
	}
	if (m_pNucleationTravelTime2 != NULL) {
		m_pNucleationTravelTime2->setTTOrigin(m_dLatitude, m_dLongitude,
												m_dDepth);
	}

	// get number of picks
	int npick = m_vPickData.size();

	// for each pick in the pick vector
	for (int ipick = 0; ipick < npick; ipick++) {
		// get the pick
		auto pick = m_vPickData[ipick];

		// calculate the travel times
		double tCal1 = -1;
		if (m_pNucleationTravelTime1 != NULL) {
			tCal1 = m_pNucleationTravelTime1->T(&pick->getSite()->getGeo());
		}
		double tCal2 = -1;
		if (m_pNucleationTravelTime2 != NULL) {
			tCal2 = m_pNucleationTravelTime2->T(&pick->getSite()->getGeo());
		}

		// calculate absolute residuals
		double tRes1 = 0;
		if (tCal1 > 0) {
			tRes1 = std::abs(pick->getTPick() - tCal1 - m_tOrigin);
		}
		double tRes2 = 0;
		if (tCal2 > 0) {
			tRes2 = std::abs(pick->getTPick() - tCal2 - m_tOrigin);
		}

		// get best (smallest) residual
		double tResBest = 0;
		if ((tRes1 != 0) && (tRes1 < tRes2)) {
			tResBest = tRes1;
		}
		if ((tRes2 != 0) && (tRes1 > tRes2)) {
			tResBest = tRes2;
		} else {
			tResBest = tRes1;
		}

		// make sure the pick residual is within the residual limits
		double sdassoc = CGlass::getAssociationSDCutoff();
		if (tResBest > sdassoc) {
			vRemovePicks.push_back(pick);
		}
	}

	// for each pick to remove
	for (auto pick : vRemovePicks) {
		// remove the pick hypo link
		std::shared_ptr<CHypo> pickHyp = pick->getHypoReference();
		if (pickHyp != NULL) {
			if (pickHyp->getID() == getID()) {
				pick->removeHypoReference(pickHyp->getID());
			}
		}

		// remove the pick from this hypo
		removePickReference(pick);
	}

	glass3::util::Logger::log("debug", "CHypo::anneal, Event: " + m_sID
		+ " bayes: " + std::to_string(m_dBayesValue)
		+ " removed: " + std::to_string(vRemovePicks.size()));

	// return the final bayesian value
	return (m_dBayesValue);
}

// ---------------------------------------------------------annealingLocateBayes
void CHypo::annealingLocateBayes(int nIter, double dStart, double dStop,
									double tStart, double tStop,
									bool nucleate) {
	// lock mutex for this scope
	std::lock_guard < std::recursive_mutex > guard(m_HypoMutex);

	// don't locate if the location is fixed
	if (m_bFixed) {
		return;
	}

	// taper to lower calculateValue if large azimuthal gap
	glass3::util::Taper taperGap;
	taperGap = glass3::util::Taper(0.0, 0.0, m_dAzimuthTaper,
									k_dGapTaperDownEnd);

	// these hold the values of the initial, current, and best stack location
	double valStart = 0;
	double valBest = 0;
	// calculate the value of the stack at the current location
	valStart = calculateBayes(m_dLatitude, m_dLongitude, m_dDepth, m_tOrigin,
								nucleate)
			* taperGap.calculateValue(
					calculateGap(m_dLatitude, m_dLongitude, m_dDepth));

	char sLog[glass3::util::Logger::k_nMaxLogEntrySize];

	// if testing locator, setup output file
	std::ofstream outfile;
	if (CGlass::getTestLocator()) {
		std::string filename = "./locatorTest/" + m_sID + ".txt";
		outfile.open(filename, std::ios::out | std::ios::app);
		outfile << std::to_string(m_dLatitude) << " "
				<< std::to_string(m_dLongitude) << " "
				<< std::to_string(m_dDepth) << " " << std::to_string(m_tOrigin)
				<< " " << std::to_string(m_vPickData.size()) << " "
				<< std::to_string(valStart) << " 0 0 0 \n";
	}

	// set the starting location as the current best stack value
	valBest = valStart;

	// Save total movement
	double ddx = 0.0;
	double ddy = 0.0;
	double ddz = 0.0;
	double ddt = 0.0;

	// create taper using the number of iterations to define the
	// end point. As we iterate through trial locations this makes
	// the search space decrease.
	glass3::util::Taper taper;
	taper = glass3::util::Taper(-k_dLocationTaperConstant,
								-k_dLocationTaperConstant,
								-k_dLocationTaperConstant,
								nIter + k_dLocationTaperConstant);

	// for the number of requested iterations
	for (int iter = 0; iter < nIter; iter++) {
		// compute the current step distance from the current iteration and
		// starting and stopping values, use the taper to make the step distance
		// slowly decrease
		double dkm = (dStart - dStop)
				* taper.calculateValue(static_cast<double>(iter)) + dStop;
		double dOt = (tStart - tStop)
				* taper.calculateValue(static_cast<double>(iter)) + tStop;

		// init x, y, and z gaussian step distances
		// the km for dx and dy is double so the epicentral search space is
		// larger than the depth search space, because we live on a sphere
		double dx = glass3::util::GlassMath::gauss(
				0.0, dkm * k_dVerticalToHorizontalDistanceCorrectionFactor);
		double dy = glass3::util::GlassMath::gauss(
				0.0, dkm * k_dVerticalToHorizontalDistanceCorrectionFactor);
		double dz = glass3::util::GlassMath::gauss(0.0, dkm);
		double dt = glass3::util::GlassMath::gauss(0.0, dOt);

		// compute current location using the hypo location and the x and y
		// Gaussian step distances
		double xlon = m_dLongitude
				+ cos(glass3::util::GlassMath::k_DegreesToRadians * m_dLatitude)
						* dx / glass3::util::Geo::k_DegreesToKm;
		double xlat = m_dLatitude + dy / glass3::util::Geo::k_DegreesToKm;

		// compute current depth using the hypo depth and the z Gaussian step
		// distance
		double xz = m_dDepth + dz;

		// don't let depth go below 1 km
		if (xz < 1.0) {
			xz = 1.0;
		}

		// don't let depth exceed maximum
		if (xz > m_dMaxDepth) {
			xz = m_dDepth;
		}

		// compute current origin time
		double oT = m_tOrigin + dt;

		// get the stack value for this hypocenter
		double bayes = calculateBayes(xlat, xlon, xz, oT, nucleate)
				* taperGap.calculateValue(calculateGap(xlat, xlon, xz));

		// if testing locator print iteration
		if (CGlass::getTestLocator()) {
			outfile << std::to_string(xlat) << " " << std::to_string(xlon)
					<< " " << std::to_string(xz) << " " << std::to_string(oT)
					<< " " << std::to_string(m_vPickData.size()) << " "
					<< std::to_string(bayes) << " " << std::to_string(dkm * 2)
					<< " " << std::to_string(dkm) << " " << std::to_string(dOt)
					<< "\n";
		}

		// is this stacked bayesian value (calculateValue) better than the
		// previous best  (valBest)
		if (bayes > valBest) {
			// then this is the new best value
			valBest = bayes;

			// check to see if this is a "BIG" move.  If so, update audit
			// information.
			if ((sqrt(
					(xlat - this->m_hapsAudit.dLatPrev)
							* (xlat - this->m_hapsAudit.dLatPrev)
							+ (xlon - this->m_hapsAudit.dLonPrev)
									* cos(glass3::util::GlassMath::k_DegreesToRadians
											* xlat)
									* (xlon - this->m_hapsAudit.dLonPrev)
									* cos(glass3::util::GlassMath::k_DegreesToRadians
											* xlat))
					* glass3::util::Geo::k_DegreesToKm
					> m_dWebResolution * k_dLocationChangeWebResolutionRatio)
					|| (fabs(this->m_hapsAudit.dDepthPrev - xz)
							> k_dMinimumDepthChangeKMThreshold
							&& fabs(m_dDepth - xz)
									> this->m_hapsAudit.dDepthPrev
											* k_dMinimumDepthChangeRatioThreshold)) {
				// this represents a LARGE movement (currently
				// m_dWebResolution / 2).  Update auditing information
				this->m_hapsAudit.dtLastBigMove = glass3::util::Date::now();
				this->m_hapsAudit.dtOrigin = oT;
				this->m_hapsAudit.dMaxStackBeforeMove = this->m_hapsAudit
						.dMaxStackSinceMove;
				this->m_hapsAudit.nMaxPhasesBeforeMove = this->m_hapsAudit
						.nMaxPhasesSinceMove;
				this->m_hapsAudit.dLatPrev = m_dLatitude;
				this->m_hapsAudit.dLonPrev = m_dLongitude;
				this->m_hapsAudit.dDepthPrev = m_dDepth;
			}

			this->m_hapsAudit.dMaxStackSinceMove = valBest;
			double dCurrDataCount = m_vPickData.size();
			if (dCurrDataCount > this->m_hapsAudit.nMaxPhasesSinceMove) {
				this->m_hapsAudit.nMaxPhasesSinceMove = dCurrDataCount;
			}

			// set the hypo location/depth/time from the new best
			// locaton/depth/time
			setLatitude(xlat);
			setLongitude(xlon);
			setDepth(xz);
			setTOrigin(oT);

			// save this perturbation to the overall change
			ddx += dx;
			ddy += dy;
			ddz += dz;
			ddt += dt;
		}
	}

	// set dBayes to current value
	m_dBayesValue = valBest;
	if (nucleate == true) {
		m_dInitialBayesValue = valBest;
	}

	/*
	 snprintf(
	 sLog, sizeof(sLog),
	 "CHypo::annealingLocate: total movement (%.4f,%.4f,%.4f,%.4f)"
	 " (%.4f,%.4f,%.4f,%.4f) sPid:%s; new bayes value:%.4f; old bayes"
	 " value:%.4f",
	 getLatitude(), getLongitude(), getDepth(), getTOrigin(), ddx, ddy,
	 ddz, ddt, m_sID.c_str(), valBest, valStart);
	 glass3::util::Logger::log(sLog);
	 */

	if (CGlass::getGraphicsOut() == true) {
		graphicsOutput();
	}

	// if testing the locator close the file
	if (CGlass::getTestLocator()) {
		outfile.close();
	}

	return;
}

// ------------------------------------------------------annealingLocateResidual
void CHypo::annealingLocateResidual(int nIter, double dStart, double dStop,
									double tStart, double tStop,
									bool nucleate) {
	// lock mutex for this scope
	std::lock_guard < std::recursive_mutex > guard(m_HypoMutex);

	if (m_pTravelTimeTables == NULL) {
		glass3::util::Logger::log(
				"error", "CHypo::annealingLocateResidual: NULL pTTT.");
		return;
	}

	if (m_bFixed) {
		return;
	}
	char sLog[glass3::util::Logger::k_nMaxLogEntrySize];

	double delta;
	double sigma;

	m_pTravelTimeTables->setTTOrigin(m_dLatitude, m_dLongitude, m_dDepth);

	double valStart = calculateAbsResidualSum(m_dLatitude, m_dLongitude,
												m_dDepth, m_tOrigin, nucleate);
	m_dBayesValue = calculateBayes(m_dLatitude, m_dLongitude, m_dDepth,
									m_tOrigin, nucleate);
	snprintf(sLog, sizeof(sLog), "CHypo::annealingLocate: old bayes value %.4f",
				getBayesValue());
	glass3::util::Logger::log(sLog);

	snprintf(
			sLog, sizeof(sLog),
			"CHypo::annealingLocateResidual: old sum abs residual value %.4f",
			valStart);
	glass3::util::Logger::log(sLog);

	double valBest = valStart;

	// Save total movement
	double ddx = 0.0;
	double ddy = 0.0;
	double ddz = 0.0;
	double ddt = 0.0;

	// create taper using the number of iterations to define the
	// end point
	glass3::util::Taper taper;
	taper = glass3::util::Taper(-k_dLocationTaperConstant,
								-k_dLocationTaperConstant,
								-k_dLocationTaperConstant,
								nIter + k_dLocationTaperConstant);

	// for the number of requested iterations
	for (int iter = 0; iter < nIter; iter++) {
		// compute the current step distance from the current iteration and
		// starting and stopping values, use the taper to make the step distance
		// slowly decrease
		double dkm = dStart * taper.calculateValue(static_cast<double>(iter))
				+ dStop;
		double dOt = tStart * taper.calculateValue(static_cast<double>(iter))
				+ tStop;

		// init x, y, and z gaussian step distances
		double dx = glass3::util::GlassMath::gauss(
				0.0, dkm * k_dVerticalToHorizontalDistanceCorrectionFactor);
		double dy = glass3::util::GlassMath::gauss(
				0.0, dkm * k_dVerticalToHorizontalDistanceCorrectionFactor);
		double dz = glass3::util::GlassMath::gauss(0.0, dkm);
		double dt = glass3::util::GlassMath::gauss(0.0, dOt);

		// compute current location using the hypo location and the x and y
		// Gaussian step distances
		double xlon = m_dLongitude
				+ cos(glass3::util::GlassMath::k_DegreesToRadians * m_dLatitude)
						* dx / glass3::util::Geo::k_DegreesToKm;
		double xlat = m_dLatitude + dy / glass3::util::Geo::k_DegreesToKm;

		// compute current depth using the hypo depth and the z Gaussian step
		// distance
		double xz = m_dDepth + dz;

		// don't let depth go below 1 km
		if (xz < 1.0) {
			xz = 1.0;
		}

		// don't let depth exceed maximum
		if (xz > m_dMaxDepth) {
			xz = m_dDepth;
		}

		// compute current origin time
		double oT = m_tOrigin + dt;

		m_pTravelTimeTables->setTTOrigin(xlat, xlon, xz);
		double calculateValue = calculateAbsResidualSum(xlat, xlon, xz, oT,
														nucleate);
		// geo.setGeographic(dLat, dLon, glass3::util::Geo::k_EarthRadiusKm - dZ);

		// is this stacked bayesian value better than the previous one
		if (calculateValue < valBest) {
			// this is the new minimized residual
			valBest = calculateValue;

			// set the hypo location/depth/time from the new best
			// locaton/depth/time
			// check to see if this is a "BIG" move.  If so, update audit
			// information.
			if ((sqrt(
					(xlat - m_dLatitude) * (xlat - m_dLatitude)
							+ (xlon - m_dLongitude)
									* cos(glass3::util::GlassMath::k_DegreesToRadians
											* xlat) * (xlon - m_dLongitude)
									* cos(glass3::util::GlassMath::k_DegreesToRadians
											* xlat))
					* glass3::util::Geo::k_DegreesToKm
					> m_dWebResolution * k_dLocationChangeWebResolutionRatio)
					|| (fabs(m_dDepth - xz) > k_dMinimumDepthChangeKMThreshold
							&& fabs(m_dDepth - xz)
									> this->m_hapsAudit.dDepthPrev
											* k_dMinimumDepthChangeRatioThreshold)) {
				// this represents a LARGE movement (currently m_dWebResolution
				// / 2).  Update auditing information
				this->m_hapsAudit.dtLastBigMove = glass3::util::Date::now();
				this->m_hapsAudit.dtOrigin = oT;
				this->m_hapsAudit.dMaxStackBeforeMove = this->m_hapsAudit
						.dMaxStackSinceMove;
				this->m_hapsAudit.nMaxPhasesBeforeMove = this->m_hapsAudit
						.nMaxPhasesSinceMove;
				this->m_hapsAudit.dMaxStackSinceMove = valBest;
				this->m_hapsAudit.nMaxPhasesSinceMove = m_vPickData.size();
				this->m_hapsAudit.dLatPrev = m_dLatitude;
				this->m_hapsAudit.dLonPrev = m_dLongitude;
				this->m_hapsAudit.dDepthPrev = m_dDepth;
			}

			// set the hypo location/depth/time from the new best
			// locaton/depth/time
			setLatitude(xlat);
			setLongitude(xlon);
			setDepth(xz);
			setTOrigin(oT);
			ddx += dx;
			ddy += dy;
			ddz += dz;
			ddt += dt;
		}
	}

	m_dBayesValue = calculateBayes(m_dLatitude, m_dLongitude, m_dDepth,
									m_tOrigin, nucleate);
	snprintf(sLog, sizeof(sLog), "CHypo::annealingLocate: old bayes value %.4f",
				getBayesValue());
	glass3::util::Logger::log(sLog);
	snprintf(sLog, sizeof(sLog),
				"CHypo::annealingLocate: total movement (%.4f,%.4f,%.4f,%.4f)"
				" (%.4f,%.4f,%.4f,%.4f)",
				getLatitude(), getLongitude(), getDepth(), getTOrigin(), ddx,
				ddy, ddz, ddt);
	glass3::util::Logger::log(sLog);

	snprintf(sLog, sizeof(sLog),
				"CHypo::annealingLocate: new sum abs residual %.4f", valBest);
	glass3::util::Logger::log(sLog);

	if (CGlass::getGraphicsOut() == true) {
		graphicsOutput();
	}

	return;
}

// ---------------------------------------------------------canAssociate
bool CHypo::canAssociate(std::shared_ptr<CPick> pick, double sigma,
							double sdassoc, bool p_only, bool debug) {
	// lock mutex for this scope
	std::lock_guard < std::recursive_mutex > guard(m_HypoMutex);

	// check to see if this is a valid hypo, a hypo must always have an id
	if (m_sID == "") {
		return (false);
	}

	// null check
	if (pick == NULL) {
		glass3::util::Logger::log("error", "CHypo::canAssociate: NULL pick.");
		return (false);
	}

	if (m_pTravelTimeTables == NULL) {
		glass3::util::Logger::log("error", "CHypo::canAssociate: NULL pTTT.");
		return (false);
	}

	double dAzimuthRange = CGlass::getBeamMatchingAzimuthWindow();
	// double dDistanceRange = pGlass->getBeamMatchingDistanceWindow();

	// get site
	std::shared_ptr<CSite> site = pick->getSite();

	// set up a geographic object for this hypo
	glass3::util::Geo hypoGeo;
	hypoGeo.setGeographic(m_dLatitude, m_dLongitude,
							glass3::util::Geo::k_EarthRadiusKm - m_dDepth);

	// check backazimuth if present
	if (pick->getBackAzimuth() != std::numeric_limits<double>::quiet_NaN()) {
		// compute azimuth from the site to the node
		double siteAzimuth = site->getGeo().azimuth(&hypoGeo);

		// check to see if pick's backazimuth is within the
		// valid range
		if (glass3::util::GlassMath::angleDifference(pick->getBackAzimuth(),
														siteAzimuth)
				> dAzimuthRange) {
			// it is not, do not associate
			if (debug) {
				glass3::util::Logger::log("debug", "CHypo::canAssociate: Pick: "
							+ pick->getID() + " ("
							+ pick->getSite()->getSCNL()
							+ ") back azimuth "
							+ std::to_string(pick->getBackAzimuth())
							+ " is beyond the valid range "
							+ std::to_string(dAzimuthRange)
							+ " of " + std::to_string(siteAzimuth)
							+ ", cannot associate with "
							+ getID());
			}
			return (false);
		}
	}

	// check classification azimuth if present
	if (CGlass::getPickAzimuthClassificationThreshold() > 0) {
		if ((std::isnan(pick->getClassifiedAzimuthProbability()) != true)
				&& (pick->getClassifiedAzimuthProbability()
						> CGlass::getPickAzimuthClassificationThreshold())) {
			// set up a geo for azimuth calculations
			double siteAzimuth = hypoGeo.azimuth(&(site->getGeo()))
					* glass3::util::GlassMath::k_RadiansToDegrees;
			// check to see if pick's azimuth is within the
			// valid range
			if (glass3::util::GlassMath::angleDifference(
					pick->getClassifiedAzimuth(), siteAzimuth)
					> CGlass::getPickAzimuthClassificationUncertainty()) {
				// it is not, do not associate
				if (debug) {
					glass3::util::Logger::log("debug", "CHypo::canAssociate: Pick: "
								+ pick->getID() + " ("
								+ pick->getSite()->getSCNL()
								+ ") classified azimuth "
								+ std::to_string(pick->getBackAzimuth())
								+ " is beyond the valid range "
								+ std::to_string(dAzimuthRange)
								+ " of " + std::to_string(siteAzimuth)
								+ ", cannot associate with "
								+ getID());
				}
				return (false);
			}
		}
	}

	// check slowness if present
	// Need modify travel time libraries to support getting distance
	// from slowness, and it's of limited value compared to the back
	// azimuth check
	/* if (pick->dSlowness != std::numeric_limits<double>::quiet_NaN()) {
	 // compute observed distance from slowness (1/velocity)
	 // and tObs (distance = velocity * time)
	 double obsDistance = (1 / pick->dSlowness) * tObs;

	 // check to see if the observed distance is within the
	 // valid range
	 if ((obsDistance < (siteDistance - dDistanceRange))
	 || (obsDistance > (siteDistance + dDistanceRange))) {
	 // it is not, do not associate
	 return (false);
	 }
	 } */

	// compute distance in degrees
	double siteDistance = calculateDistanceToPick(pick);

	// check if distance is beyond cutoff
	if (siteDistance > m_dAssociationDistanceCutoff) {
		if (debug) {
			glass3::util::Logger::log("debug", "CHypo::canAssociate: Pick: "
						+ pick->getID() + " ("
						+ pick->getSite()->getSCNL()
						+ ") site distance "
						+ std::to_string(siteDistance)
						+ " is beyond the cutoff "
						+ std::to_string(m_dAssociationDistanceCutoff)
						+ ", cannot associate with "
						+ getID());
		}
		// it is, don't associated
		return (false);
	}

	// check classification distance if present
	if (CGlass::getPickDistanceClassificationThreshold() > 0) {
		if ((std::isnan(pick->getClassifiedDistanceProbability()) != true)
				&& (pick->getClassifiedDistanceProbability()
						> CGlass::getPickDistanceClassificationThreshold())) {
			// check to see if pick's distance is within the
			// valid range
			if (siteDistance
					< CGlass::getDistanceClassLowerBound(
							pick->getClassifiedDistance())
					|| siteDistance
							> CGlass::getDistanceClassUpperBound(
									pick->getClassifiedDistance())) {
				if (debug) {
					glass3::util::Logger::log("debug", "CHypo::canAssociate: Pick: "
								+ pick->getID() + " ("
								+ pick->getSite()->getSCNL()
								+ ") site distance "
								+ std::to_string(siteDistance)
								+ " is beyond the classified range of "
								+ std::to_string(CGlass::getDistanceClassLowerBound(
									pick->getClassifiedDistance()))
								+ " to "
								+ std::to_string(CGlass::getDistanceClassUpperBound(
									pick->getClassifiedDistance()))
								+ ", cannot associate with "
								+ getID());
				}
				// it is not, do not associate
				return (false);
			}
		}
	}

	// get residual, get useForLocations, get phase
	bool useForLocations = true;
	std::string phaseName = "??";
	double tRes = calculateResidual(pick, &useForLocations, &phaseName, p_only);

    // give up if there's no valid residual
	if (std::isnan(tRes) == true) {
		if (debug) {
			glass3::util::Logger::log("debug", "CHypo::canAssociate: Pick: "
							+ pick->getID() + " ("
							+ pick->getSite()->getSCNL() + ")"
							+ " does not have a valid residual, p_only is "
							+ (p_only ? "true" : "false")
							+ ", cannot associate with "
							+ getID());
		}
		return (false);
	}

	// compute absolute residual
	if (tRes < 0.0) {
		tRes = -tRes;
	}

	// compute pick standard deviation from residual
	// and sigma
	double stdev = tRes / sigma;

	// char sLog[glass3::util::Logger::k_nMaxLogEntrySize];

	// figure out the association cutoff
	// start off with the default cutoff
	double cutoff = sdassoc;

	// is this phase used for locations
	if (useForLocations == false) {
		// does this event have enough teleseismic data
		if (m_iTeleseismicPhaseCount > CGlass::getTeleseismicPhaseCountThreshold()) {
			// if this phase is not used for locations, and if this
			// hypo has teleseismic data, allow a much wider cutoff
			// range so we sweep up this phase so it doesn't cause
			// trouble
			cutoff = sdassoc * CGlass::getNonLocatingPhaseCutoffFactor();
		} else {
			// If this phase is not used for locations and if this hypo does
			// not have teleseismic data, do not associate this phase at all.
			// NOTE: if we ever have a non-teleseismic phase that is not used
			// for locations, this will cause issues some sort of way to
			// keep track of whether a phase is teleseismic will need to
			// be added to traveltime
			// return (false);
		}
	}

	// allow a wider association window for Lg
	// NOTE MAKE CONFIGURABLE
	if (phaseName == "Lg") {
		// only for shallow events
		if(getDepth() < 35.0) {
			// really big quake
			if((getBayesValue() / getNucleationStackThreshold()) > 20) {
				// Ideally this would be (-sdassoc, 4.5 * pick_dist_Deg)
				cutoff = 2.27 * siteDistance;
			} else if (((getBayesValue() / getNucleationStackThreshold()) > 10)
					  && (siteDistance < 10)) {
				// less big quake
				// Ideally this would be (-sdassoc, 4.5 * pick_dist_Deg)
				cutoff = 2.27 * siteDistance;
			} else if (siteDistance < 5) {
				// close in data
				// Ideally this would be (-sdassoc, 4.5 * pick_dist_Deg)
				cutoff = 2.27 * siteDistance;
			}
		}
	}
	/*
	 else if ((getBayesValue() / getNucleationStackThreshold()) > 6) {
		// big quake, but not Lg
		if((getBayesValue() / getNucleationStackThreshold()) > 20) {
			// *really* big quake
			// allow all phases 3x the normal association leeway
			// - gives WLY heartburn
			cutoff = 3.0 * sdassoc;
		} else {
			// allow all phases 2x the normal association leeway
			// - gives WLY heartburn
			cutoff = 2.0 * sdassoc;
		}
	}
	*/

	// check if pick standard deviation is greater than cutoff
	if (stdev > cutoff) {
		if (debug) {
			glass3::util::Logger::log("debug", "CHypo::canAssociate: Pick: "
							+ pick->getID() + " ("
							+ pick->getSite()->getSCNL() + ")"
							+ " standard deviation "
							+ std::to_string(stdev)
							+ " for phase "
							+ phaseName
							+ " is beyond the cutoff "
							+ std::to_string(cutoff)
							+ " (base sdassoc "
							+ std::to_string(sdassoc)
							+ "), cannot associate with "
							+ getID());
		}

		// it is, don't associate
		return (false);
	}

	// trimming criteria are met, associate
	return (true);
}

// ---------------------------------------------------------canAssociate
bool CHypo::canAssociate(std::shared_ptr<CCorrelation> corr, double tWindow,
							double xWindow) {
	// lock mutex for this scope
	std::lock_guard < std::recursive_mutex > guard(m_HypoMutex);

	// check to see if this is a valid hypo, a hypo must always have an id
	if (m_sID == "") {
		return (false);
	}

	// NOTE: this is a simple time/distance check for association
	// wiser heads than mine may come up with a more robust approach JMP
	// null check
	if (corr == NULL) {
		glass3::util::Logger::log("error",
									"CHypo::associate: NULL correlation.");
		return (false);
	}

	char sLog[glass3::util::Logger::k_nMaxLogEntrySize];

	double tDist = 0;
	double xDist = 0;

	// check if time difference is within window
	tDist = std::abs(m_tOrigin - corr->getTCorrelation());
	if (tDist < tWindow) {
		glass3::util::Geo geo1;
		geo1.setGeographic(m_dLatitude, m_dLongitude,
							glass3::util::Geo::k_EarthRadiusKm - m_dDepth);
		glass3::util::Geo geo2;
		geo2.setGeographic(
				corr->getLatitude(), corr->getLongitude(),
				glass3::util::Geo::k_EarthRadiusKm - corr->getDepth());
		xDist = glass3::util::GlassMath::k_RadiansToDegrees * geo1.delta(&geo2);

		// check if distance difference is within window
		if (xDist < xWindow) {
			snprintf(
					sLog,
					sizeof(sLog),
					"CHypo::associate: C-ASSOC Hypo:%s Time:%s Station:%s"
					" Corr:%s tDist:%.2f<tWindow:%.2f"
					" xDist:%.2f>xWindow:%.2f)",
					m_sID.c_str(),
					glass3::util::Date::encodeDateTime(corr->getTCorrelation())
							.c_str(),
					corr->getSite()->getSCNL().c_str(), corr->getID().c_str(),
					tDist, tWindow, xDist, xWindow);
			glass3::util::Logger::log(sLog);

			return (true);
		}
	}

	if (xDist == 0) {
		snprintf(
				sLog,
				sizeof(sLog),
				"CHypo::associate: C-NOASSOC Hypo:%s Time:%s Station:%s Corr:%s"
				" tDist:%.2f>tWindow:%.2f",
				m_sID.c_str(),
				glass3::util::Date::encodeDateTime(corr->getTCorrelation())
						.c_str(),
				corr->getSite()->getSCNL().c_str(), corr->getID().c_str(),
				tDist, tWindow);
	} else {
		snprintf(
				sLog,
				sizeof(sLog),
				"CHypo::associate: C-NOASSOC Hypo:%s Time:%s Station:%s Corr:%s"
				" tDist:%.2f<tWindow:%.2f xDist:%.2f>xWindow:%.2f)",
				m_sID.c_str(),
				glass3::util::Date::encodeDateTime(corr->getTCorrelation())
						.c_str(),
				corr->getSite()->getSCNL().c_str(), corr->getID().c_str(),
				tDist, tWindow, xDist, xWindow);
	}
	glass3::util::Logger::log(sLog);

	// it is, don't associate
	return (false);
}

// -------------------------------------------------------generateCancelMessage
std::shared_ptr<json::Object> CHypo::generateCancelMessage() {
	// lock mutex for this scope
	std::lock_guard < std::recursive_mutex > guard(m_HypoMutex);

	std::shared_ptr<json::Object> cancel = std::make_shared < json::Object
			> (json::Object());

	// fill in cancel command from current hypocenter
	(*cancel)["Cmd"] = "Cancel";
	(*cancel)["Pid"] = m_sID;

	// log it
	glass3::util::Logger::log(
			"debug",
			"CHypo::event: Created cancel message:" + json::Serialize(*cancel));

	return (cancel);
}

// ---------------------------------------------------------cancelCheck
bool CHypo::cancelCheck() {
	// lock mutex for this scope
	std::lock_guard < std::recursive_mutex > guard(m_HypoMutex);

	// can't cancel fixed hypos
	// NOTE: What implication does this have for "seed hypos" like twitter
	// detections
	if (m_bFixed) {
		return (false);
	}

	char sLog[glass3::util::Logger::k_nMaxLogEntrySize * 2];
	char sHypo[glass3::util::Logger::k_nMaxLogEntrySize];

	// calculate a new bayes value to use for these checks
	double bayes = calculateBayes(m_dLatitude, m_dLongitude, m_dDepth,
									m_tOrigin, false);

	glass3::util::Date dt = glass3::util::Date(m_tOrigin);
	snprintf(sHypo, sizeof(sHypo), "CHypo::cancel: %s tOrg:%s; dLat:%9.4f; "
				"dLon:%10.4f; dZ:%6.1f; bayes:%.2f; nPick:%d; nCorr:%d",
				m_sID.c_str(), dt.dateTime().c_str(), getLatitude(),
				getLongitude(), getDepth(), bayes,
				static_cast<int>(m_vPickData.size()),
				static_cast<int>(m_vCorrelationData.size()));

	// check correlations, we want a hypo created from a correlation
	// to stick around awhile to have a chance to associate picks.
	if (m_vCorrelationData.size() > 0) {
		// get the current time
		double now = glass3::util::Date::now();
		int cancelAge = CGlass::getCorrelationCancelAge();

		// check correlations
		int expireCount = 0;
		for (auto cor : m_vCorrelationData) {
			// count correlation as expired if it's creation time is older than
			// the cancel age
			if ((cor->getTCreate() + cancelAge) < now) {
				snprintf(sLog, sizeof(sLog),
							"CHypo::cancel: Correlation:%s created: %f "
							"limit:%d now:%f",
							m_sID.c_str(), cor->getTCreate(), cancelAge, now);
				glass3::util::Logger::log(sLog);
				expireCount++;
			}
		}

		// only prevent cancellation if all correlations haven't expired
		if (expireCount == m_vCorrelationData.size()) {
			snprintf(
					sLog, sizeof(sLog),
					"CHypo::cancel: $$Could cancel %s due to associated "
					"correlations (%d) older than %d seconds",
					m_sID.c_str(),
					(static_cast<int>(m_vCorrelationData.size()) - expireCount),
					cancelAge);
			glass3::util::Logger::log(sLog);
		} else {
			snprintf(
					sLog, sizeof(sLog),
					"CHypo::cancel: $$Will not cancel %s due to associated "
					"correlations (%d) younger than %d seconds",
					m_sID.c_str(),
					(static_cast<int>(m_vCorrelationData.size()) - expireCount),
					cancelAge);
			glass3::util::Logger::log(sLog);

			// Hypo is still viable, for now...
			return (false);
		}
	}

	// check to see if we still have enough picks for this hypo to
	// survive.
	// check to see if there is enough supporting data for this hypocenter
	// NOTE, in Node, ncut is used as a threshold for the number of
	// *stations* here it's used for the number of *picks*, not sure
	// what implication this has
	int ncut = m_iNucleationDataThreshold;
	int nPicks = static_cast<int>(m_vPickData.size());
	int nCorrelations = static_cast<int>(m_vCorrelationData.size());
	if ((nPicks + nCorrelations) < ncut) {
		// there isn't
		snprintf(sLog, sizeof(sLog), "CHypo::cancel: Insufficient data "
					"((%d + %d) < %d) Hypo: %s",
					nPicks, nCorrelations, ncut, sHypo);
		glass3::util::Logger::log(sLog);

		// this hypo can be canceled
		return (true);
	}

	// check to see if we still have a high enough bayes value for this
	// hypo to survive.
	double thresh = m_dNucleationStackThreshold;

	if (bayes < thresh) {
		// failure
		snprintf(sLog, sizeof(sLog),
					"CHypo::cancel: Below threshold (%.1f < %.1f) Hypo: %s",
					bayes, thresh, sHypo);
		glass3::util::Logger::log(sLog);

		// this hypo can be canceled
		return (true);
	}

	// Whispy (event fragment) check (does the quake have a gap greater than the
	// azimuth threshold while being deeper than the depth threshold)
	if ((m_dDepth > CGlass::getEventFragmentDepthThreshold())
			&& (m_dGap > CGlass::getEventFragmentAzimuthThreshold())) {
		// failure
		snprintf(
				sLog,
				sizeof(sLog),
				"CHypo::cancel: Event Fragment check (%.1f>%.1f, %.1f>%.1f) Hypo: %s",
				getDepth(), CGlass::getEventFragmentDepthThreshold(), getGap(),
				CGlass::getEventFragmentAzimuthThreshold(), sHypo);
		glass3::util::Logger::log(sLog);

		// this hypo can be canceled
		return (true);
	}
	// Hypo is still viable
	return (false);
}

// ---------------------------------------------------------clear
void CHypo::clear() {
	// lock mutex for this scope
	std::lock_guard < std::recursive_mutex > guard(m_HypoMutex);

	setLatitude(0.0);
	setLongitude(0.0);
	setDepth(0.0);
	setTOrigin(0.0);
	// note that we intentionally do not clear tSort...
	m_sID = "";
	m_iNucleationDataThreshold = 0;
	m_dNucleationStackThreshold = 0.0;
	m_dBayesValue = 0.0;
	m_dInitialBayesValue = 0.0;
	m_iProcessCount = 0;
	m_sWebName = "";
	m_dMedianDistance = 0;
	m_dMinDistance = 0;
	m_dGap = 0;
	m_iTeleseismicPhaseCount = 0;
	m_dDistanceSD = 0;
	m_dWebResolution = 0;

	m_dAssociationDistanceCutoff = 0;

	m_bFixed = false;
	m_bEventGenerated = false;
	m_bHypoGenerated = false;

	m_pTravelTimeTables.reset();
	m_pNucleationTravelTime1.reset();
	m_pNucleationTravelTime2.reset();

	m_bCorrelationAdded = false;

	clearPickReferences();
	clearCorrelationReferences();

	m_iTotalProcessCount = 0;
	m_iReportCount = 0;
}

// ---------------------------------------------------clearCorrelationReferences
void CHypo::clearCorrelationReferences() {
	// lock the hypo since we're iterating through it's lists
	std::lock_guard < std::recursive_mutex > hypoGuard(m_HypoMutex);

	// go through all the corrs linked to this hypo
	for (auto corr : m_vCorrelationData) {
		if (corr == NULL) {
			continue;
		}

		// remove the hypo from the corr
		// note only removes hypo if corr
		// is linked
		corr->removeHypoReference(m_sID);
	}

	// remove all correlation links to this hypo
	m_vCorrelationData.clear();
}

// ---------------------------------------------------------clearPickReferences
void CHypo::clearPickReferences() {
	// lock the hypo since we're iterating through it's lists
	std::lock_guard < std::recursive_mutex > hypoGuard(m_HypoMutex);

	// go through all the picks linked to this hypo
	for (auto pck : m_vPickData) {
		if (pck == NULL) {
			continue;
		}

		// remove the hypo from the pick
		// note only removes hypo if pick
		// is linked
		pck->removeHypoReference(m_sID);
	}

	// remove all pick links to this hypo
	m_vPickData.clear();
}

// ---------------------------------------------------------generateEventMessage
std::shared_ptr<json::Object> CHypo::generateEventMessage() {
	// lock mutex for this scope
	std::lock_guard < std::recursive_mutex > guard(m_HypoMutex);

	if (m_bEventGenerated == false) {
		m_hapsAudit.dtFirstEventMessage = glass3::util::Date::now();
	}

	m_bEventGenerated = true;
	m_iReportCount++;
	std::shared_ptr<json::Object> event = std::make_shared < json::Object
			> (json::Object());

	// fill in Event command from current hypocenter
	(*event)["Cmd"] = "Event";
	(*event)["Pid"] = m_sID;
	(*event)["CreateTime"] = glass3::util::Date::encodeISO8601Time(m_tCreate);
	(*event)["ReportTime"] = glass3::util::Date::encodeISO8601Time(
			glass3::util::Date::now());
	(*event)["Version"] = getReportCount();

	// basic hypo information
	(*event)["Latitude"] = getLatitude();
	(*event)["Longitude"] = getLongitude();
	(*event)["Depth"] = getDepth();
	(*event)["Time"] = glass3::util::Date::encodeISO8601Time(m_tOrigin);
	(*event)["Bayes"] = getBayesValue();
	(*event)["Ndata"] = static_cast<int>(m_vPickData.size())
			+ static_cast<int>(m_vCorrelationData.size());

	// log it
	glass3::util::Logger::log(
			"debug",
			"CHypo::event: Created event message:" + json::Serialize(*event));

	return (event);
}

// -------------------------------------------------------generateExpireMessage
std::shared_ptr<json::Object> CHypo::generateExpireMessage() {
	std::shared_ptr<json::Object> expire = std::make_shared < json::Object
			> (json::Object());
	(*expire)["Cmd"] = "Expire";
	(*expire)["Pid"] = m_sID;

	// add a copy of the expiring hypo to the message
	// if we CAN report
	if (reportCheck() == true) {
		std::shared_ptr<json::Object> expireHypo = generateHypoMessage();
		(*expire)["Hypo"] = (*expireHypo);
	}

	// log it
	glass3::util::Logger::log(
			"debug",
			"CHypo::event: Created expire message:" + json::Serialize(*expire));

	return (expire);
}

// ---------------------------------------------------------calculateGap
double CHypo::calculateGap(double lat, double lon, double z) {
	// lock mutex for this scope
	std::lock_guard < std::recursive_mutex > guard(m_HypoMutex);

	// set up a geographic object for this hypo
	glass3::util::Geo geo;
	geo.setGeographic(lat, lon, glass3::util::Geo::k_EarthRadiusKm - z);

	// create and populate vectors containing the
	// pick distances and azimuths
	std::vector<double> azm;

	for (auto pick : m_vPickData) {
		// get the site
		std::shared_ptr<CSite> site = pick->getSite();

		// compute the azimuth
		double azimuth = geo.azimuth(&site->getGeo())
				/ glass3::util::GlassMath::k_DegreesToRadians;

		// add to azimuth vector
		azm.push_back(azimuth);
	}

	int nazm = azm.size();

	if (nazm <= 1) {
		return 360.;
	}

	// sort the azimuths
	sort(azm.begin(), azm.end());

	// add the first (smallest) azimuth to the end by adding 360
	azm.push_back(azm.front() + 360.0);

	// compute gap
	double tempGap = 0.0;

	for (int i = 0; i < nazm; i++) {
		double gap = azm[i + 1] - azm[i];
		if (gap > tempGap) {
			tempGap = gap;
		}
	}

	return tempGap;
}

// ---------------------------------------------getTravelTimeForPhase
double CHypo::getTravelTimeForPhase(std::shared_ptr<CPick> pick,
									std::string phaseName) {
	if (pick == NULL) {
		return(traveltime::CTravelTime::k_dTravelTimeInvalid);
	}
	if (phaseName == "") {
		return(traveltime::CTravelTime::k_dTravelTimeInvalid);
	}

	// lock mutex for this scope
	std::lock_guard <std::recursive_mutex> guard(m_HypoMutex);

	// setup traveltime interface for this hypo
	m_pTravelTimeTables->setTTOrigin(m_dLatitude, m_dLongitude, m_dDepth);

	// set up a geographic object for this hypo
	glass3::util::Geo hypoGeo;
	hypoGeo.setGeographic(m_dLatitude, m_dLongitude,
							glass3::util::Geo::k_EarthRadiusKm - m_dDepth);

	// get site
	std::shared_ptr<CSite> site = pick->getSite();

	// compute site distance in degrees
	double siteDistance = calculateDistanceToPick(pick);

	// get the traveltime for this phase depth and distance
	double tCal = m_pTravelTimeTables->Td(siteDistance, phaseName, m_dDepth);

	return (tCal);
}

// ---------------------------------------------calculateDistanceToPick
double CHypo::calculateDistanceToPick(std::shared_ptr<CPick> pick) {
	if (pick == NULL) {
		return(std::numeric_limits<double>::quiet_NaN());
	}

	// lock mutex for this scope
	std::lock_guard <std::recursive_mutex> guard(m_HypoMutex);

	// set up a geographic object for this hypo
	glass3::util::Geo hypoGeo;
	hypoGeo.setGeographic(m_dLatitude, m_dLongitude,
							glass3::util::Geo::k_EarthRadiusKm - m_dDepth);

	// get site
	std::shared_ptr<CSite> site = pick->getSite();

	// compute site distance in degrees
	double siteDistance = glass3::util::GlassMath::k_RadiansToDegrees
				* hypoGeo.delta(&site->getGeo());

	return(siteDistance);
}

// --------------------------------------------------------calculateResidual
double CHypo::calculateResidual(std::shared_ptr<CPick> pick,
		bool * useForLocations, std::string * phaseName, bool p_only) {
	if (pick == NULL) {
		return(std::numeric_limits<double>::quiet_NaN());
	}

	// lock mutex for this scope
	std::lock_guard < std::recursive_mutex > guard(m_HypoMutex);

	// setup traveltime interface for this hypo
	m_pTravelTimeTables->setTTOrigin(m_dLatitude, m_dLongitude, m_dDepth);

	// get site
	std::shared_ptr<CSite> site = pick->getSite();

	// compute observed traveltime
	double tObs = pick->getTPick() - m_tOrigin;

	// init expected travel time
	double tCal = traveltime::CTravelTime::k_dTravelTimeInvalid;

	std::string phase = "??";

	// check pick classification
	// are we configured to check pick phase classification
	if (CGlass::getPickPhaseClassificationThreshold() > 0) {
		// check to see if the phase classification is valid and above
		// our threshold
		if ((std::isnan(pick->getClassifiedPhaseProbability()) != true)
				&& (pick->getClassifiedPhaseProbability()
						> CGlass::getPickPhaseClassificationThreshold())
				&& (pick->getClassifiedPhase()
						!= traveltime::CTravelTime::k_dPhaseInvalid)) {
			// valid phase classification,
			// compute expected travel time based on the pick site location and
			// the classified pick phase
			tCal = m_pTravelTimeTables->T(&site->getGeo(),
											pick->getClassifiedPhase());
			phase = pick->getClassifiedPhase();
		} else {
			// no valid phase classification,
			// compute expected travel time based on the pick site location and
			// the observed travel time
			if (p_only == false) {
				tCal = m_pTravelTimeTables->T(&site->getGeo(), tObs);
				phase = m_pTravelTimeTables->m_sPhase;
			} else {
				tCal = m_pTravelTimeTables->T(&site->getGeo(), "P");
				phase = "P";
			}
		}
	} else {
		// not checking phase classification
		// compute expected travel time based on the pick site location and
		// the observed travel time
		if (p_only == false) {
			tCal = m_pTravelTimeTables->T(&site->getGeo(), tObs);
			phase = m_pTravelTimeTables->m_sPhase;
		} else {
			tCal = m_pTravelTimeTables->T(&site->getGeo(), "P");
			phase = "P";
		}
	}

	// Check if pick has an invalid travel time,
	if (tCal <= traveltime::CTravelTime::k_dTravelTimeInvalid) {
		// it does, don't associated
		if (useForLocations != NULL) {
			*useForLocations = false;
		}
		return (std::numeric_limits<double>::quiet_NaN());
	}

	// get the use flag if the location is valid
	if (useForLocations != NULL) {
		*useForLocations = m_pTravelTimeTables->m_bUseForLocations;
	}

	if (phaseName != NULL) {
		*phaseName = phase;
	}

	// compute residual from observed and calculated travel times
	double tRes = tObs - tCal;

	// set up geo for distance calculations
	/* glass3::util::Geo geo;
	geo.setGeographic(m_dLatitude, m_dLongitude,
		glass3::util::Geo::k_EarthRadiusKm - m_dDepth);

	double dist = geo.delta(&site->getGeo())
				/ glass3::util::GlassMath::k_DegreesToRadians;

	// log it
	glass3::util::Logger::log(
			"debug",
			"CHypo::calculateResidual: Calculated residual: " + std::to_string(tRes)
			+ "; Phase: " + phase + "; for Pick: " + pick->getID()
			+ "; tObs: " + std::to_string(tObs) + "; dist: " + std::to_string(dist)
			+ " and Hypo: " + getID());
	*/

	return (tRes);
}

// --------------------------------------------------------getAzimuthTaper
double CHypo::getAzimuthTaper() const {
	return (m_dAzimuthTaper);
}

// --------------------------------------------------------getMaxDepth
double CHypo::getMaxDepth() const {
	return (m_dMaxDepth);
}

// --------------------------------------------------------getBayesValue
double CHypo::getBayesValue() const {
	return (m_dBayesValue);
}

// ---------------------------------------------------calculateCurrentBayes
double CHypo::calculateCurrentBayes() {
	// note false = don't use nucleation phases
	return(calculateBayes(m_dLatitude, m_dLongitude, m_dDepth,
											m_tOrigin, false));
}

// ---------------------------------------------------calculateBayes
double CHypo::calculateBayes(double xlat, double xlon, double xZ, double oT,
								bool nucleate) {
	// check to see if this is a valid hypo, a hypo must always have an id
	if (m_sID == "") {
		return (0);
	}

	// lock mutex for this scope
	std::lock_guard < std::recursive_mutex > guard(m_HypoMutex);

	if ((!m_pNucleationTravelTime1) && (!m_pNucleationTravelTime2)) {
		glass3::util::Logger::log(
				"error", "CHypo::calculateBayes: NULL pTrv1 and pTrv2.");
		return (0);
	}

	if (!m_pTravelTimeTables) {
		glass3::util::Logger::log("error", "CHypo::calculateBayes: NULL pTTT.");
		return (0);
	}

	glass3::util::Geo geo;
	double value = 0.;
	double tcal;
	char sLog[glass3::util::Logger::k_nMaxLogEntrySize];

	// define a taper for sigma, makes close in readings have higher weight
	// ranges from 0.75-3.0 from 0-2 degrees, than 3.0 after that (see loop)
	glass3::util::Taper tap;
	tap = glass3::util::Taper(-0.0001, 2.0, 999.0, 999.0);

	// geo is used for calculating distances to stations for determining sigma
	geo.setGeographic(xlat, xlon, glass3::util::Geo::k_EarthRadiusKm - xZ);

	// This sets the travel-time look up location
	if (m_pNucleationTravelTime1) {
		m_pNucleationTravelTime1->setTTOrigin(geo);
	}
	if (m_pNucleationTravelTime2) {
		m_pNucleationTravelTime2->setTTOrigin(geo);
	}

	m_pTravelTimeTables->setTTOrigin(geo);

	// The number of picks associated with the hypocenter
	int npick = m_vPickData.size();

	// Loop through each pick and find the residual, calculate
	// the significance, and add to the stacks.
	// Currently only P, S, and nucleation phases added to stack.
	for (int ipick = 0; ipick < npick; ipick++) {
		auto pick = m_vPickData[ipick];
		double resi = std::numeric_limits<double>::quiet_NaN();

		// calculate residual
		double tobs = pick->getTPick() - oT;
		std::shared_ptr<CSite> site = pick->getSite();
		glass3::util::Geo siteGeo = site->getGeo();

		// only use nucleation phases if on nucleation branch
		if (nucleate == true) {
			if ((m_pNucleationTravelTime1) && (m_pNucleationTravelTime2)) {
				// we have both nucleation phases
				// first nucleation phase
				// calculate the residual using the phase name
				double tcal1 = m_pNucleationTravelTime1->T(&siteGeo);
				double resi1 = calculateWeightedResidual(
						m_pNucleationTravelTime1->m_sPhase, tobs, tcal1);

				// second nucleation phase
				// calculate the residual using the phase name
				double tcal2 = m_pNucleationTravelTime2->T(&siteGeo);
				double resi2 = calculateWeightedResidual(
						m_pNucleationTravelTime2->m_sPhase, tobs, tcal2);

				// use the smallest residual
				if (abs(resi1) < abs(resi2)) {
					tcal = tcal1;
					resi = resi1;
				} else {
					tcal = tcal2;
					resi = resi2;
				}
			} else if ((m_pNucleationTravelTime1)
					&& (!m_pNucleationTravelTime2)) {
				// we have just the first nucleation phase
				tcal = m_pNucleationTravelTime1->T(&siteGeo);
				resi = calculateWeightedResidual(
						m_pNucleationTravelTime1->m_sPhase, tobs, tcal);
			} else if ((!m_pNucleationTravelTime1)
					&& (m_pNucleationTravelTime2)) {
				// we have just the second nucleation phase
				tcal = m_pNucleationTravelTime2->T(&siteGeo);
				resi = calculateWeightedResidual(
						m_pNucleationTravelTime2->m_sPhase, tobs, tcal);
			}
		} else {
			// use all available association phases
			// take whichever phase has the smallest residual
			tcal = m_pTravelTimeTables->T(&siteGeo, tobs);

			// check if we're allowed to use this phase in locations
			if (m_pTravelTimeTables->m_bUseForLocations == false) {
				// if not, skip it
				continue;
			}

			// calculate the residual using the phase name
			resi = calculateWeightedResidual(m_pTravelTimeTables->m_sPhase,
												tobs, tcal);
		}

		// make sure residual is valid
		if (resi == std::numeric_limits<double>::quiet_NaN()) {
			glass3::util::Logger::log("warning",
										"CHypo::getBayes: invalid residual.");
			continue;
		}

		// calculate distance to station to get sigma
		double delta = glass3::util::GlassMath::k_RadiansToDegrees
				* geo.delta(&site->getGeo());
		double sigma = (tap.calculateValue(delta) * 2.25) + 0.75;

		// calculate and add to the stack
		value += glass3::util::GlassMath::sig(resi, sigma);
	}
	return value;
}

// --------------------------------------------------------getInitialBayesValue
double CHypo::getInitialBayesValue() const {
	return (m_dInitialBayesValue);
}

// --------------------------------------------------------getCorrelationAdded
bool CHypo::getCorrelationAdded() const {
	return (m_bCorrelationAdded);
}

// --------------------------------------------------getNucleationDataThreshold
int CHypo::getNucleationDataThreshold() const {
	return (m_iNucleationDataThreshold);
}

// ------------------------------------------------getAssociationDistanceCutoff
double CHypo::getAssociationDistanceCutoff() const {
	return (m_dAssociationDistanceCutoff);
}

// ------------------------------------------------------------getProcessCount
int CHypo::getProcessCount() const {
	return (m_iProcessCount);
}

// ------------------------------------------------------------getEventSent
bool CHypo::getEventGenerated() const {
	return (m_bEventGenerated);
}

// ------------------------------------------------------------getHypoGenerated
bool CHypo::getHypoGenerated() const {
	return (m_bHypoGenerated);
}

// ------------------------------------------------------------getFixed
bool CHypo::getFixed() const {
	return (m_bFixed);
}

// ------------------------------------------------------------getGap
double CHypo::getGap() const {
	return (m_dGap);
}

// ----------------------------------------------------getTeleseismicPhaseCount
int CHypo::getTeleseismicPhaseCount() const {
	return (m_iTeleseismicPhaseCount);
}

// ------------------------------------------------------------getGeo
glass3::util::Geo CHypo::getGeo() const {
	std::lock_guard < std::recursive_mutex > hypoGuard(m_HypoMutex);
	glass3::util::Geo geoHypo;
	geoHypo.setGeographic(m_dLatitude, m_dLongitude,
							glass3::util::Geo::k_EarthRadiusKm - m_dDepth);
	return (geoHypo);
}

// ------------------------------------------------------------getLatitude
double CHypo::getLatitude() const {
	return (m_dLatitude);
}

// ------------------------------------------------------------getLongitude
double CHypo::getLongitude() const {
	return (m_dLongitude);
}

// ------------------------------------------------------------getMedianDistance
double CHypo::getMedianDistance() const {
	return (m_dMedianDistance);
}

// ------------------------------------------------------------getMinDistance
double CHypo::getMinDistance() const {
	return (m_dMinDistance);
}

// -----------------------------------------------------------getProcessingMutex
std::mutex & CHypo::getProcessingMutex() {
	return (m_ProcessingMutex);
}

// ------------------------------------------------------------getID
const std::string& CHypo::getID() const {
	return (m_sID);
}

// ---------------------------------------------------------getTotalProcessCount
int CHypo::getTotalProcessCount() const {
	return (m_iTotalProcessCount);
}

// ---------------------------------------------------------getReportCount
int CHypo::getReportCount() const {
	return (m_iReportCount);
}

// ---------------------------------------------------------getWebResolution
double CHypo::getWebResolution() const {
	return (m_dWebResolution);
}

// ---------------------------------------------------------getDistanceSD
double CHypo::getDistanceSD() const {
	return (m_dDistanceSD);
}

// ---------------------------------------------------------getTCreate
double CHypo::getTCreate() const {
	return (m_tCreate);
}

// --------------------------------------------------getNucleationStackThreshold
double CHypo::getNucleationStackThreshold() const {
	return (m_dNucleationStackThreshold);
}

// --------------------------------------------------getNucleationTravelTime1
std::shared_ptr<traveltime::CTravelTime> CHypo::getNucleationTravelTime1() const {  // NOLINT
	std::lock_guard < std::recursive_mutex > hypoGuard(m_HypoMutex);
	return (m_pNucleationTravelTime1);
}

// --------------------------------------------------getNucleationTravelTime2
std::shared_ptr<traveltime::CTravelTime> CHypo::getNucleationTravelTime2() const {  // NOLINT
	std::lock_guard < std::recursive_mutex > hypoGuard(m_HypoMutex);
	return (m_pNucleationTravelTime2);
}

// --------------------------------------------------getTravelTimeTables
std::shared_ptr<traveltime::CTTT> CHypo::getTravelTimeTables() const {
	std::lock_guard < std::recursive_mutex > hypoGuard(m_HypoMutex);
	return (m_pTravelTimeTables);
}

// --------------------------------------------------getPickDataSize
int CHypo::getPickDataSize() const {
	std::lock_guard < std::recursive_mutex > hypoGuard(m_HypoMutex);
	return (m_vPickData.size());
}

// --------------------------------------------------getPickData
std::vector<std::shared_ptr<CPick>> CHypo::getPickData() const {
	std::lock_guard < std::recursive_mutex > hypoGuard(m_HypoMutex);
	return (m_vPickData);
}

// --------------------------------------------------getCorrelationDataSize
int CHypo::getCorrelationDataSize() const {
	std::lock_guard < std::recursive_mutex > hypoGuard(m_HypoMutex);
	return (m_vCorrelationData.size());
}

// --------------------------------------------------getWebName
const std::string& CHypo::getWebName() const {
	return (m_sWebName);
}

// ---------------------------------------------------calculateAbsResidualSum
double CHypo::calculateAbsResidualSum(double xlat, double xlon, double xZ,
										double oT, bool nucleate) {
	if (m_pTravelTimeTables == NULL) {
		glass3::util::Logger::log("error",
									"CHypo::getSumAbsResidual: NULL pTTT.");
		return (0);
	}

	// lock mutex for this scope
	std::lock_guard < std::recursive_mutex > guard(m_HypoMutex);

	double sigma;
	double value = 0.;
	char sLog[glass3::util::Logger::k_nMaxLogEntrySize];

	// This sets the travel-time look up location
	if (m_pNucleationTravelTime1 != NULL) {
		m_pNucleationTravelTime1->setTTOrigin(xlat, xlat, xZ);
	}
	if (m_pNucleationTravelTime2 != NULL) {
		m_pNucleationTravelTime2->setTTOrigin(xlat, xlat, xZ);
	}

	m_pTravelTimeTables->setTTOrigin(xlat, xlon, xZ);

	// The number of picks associated with the hypocenter
	int npick = m_vPickData.size();

	// Loop through each pick and find the residual, calculate
	// the resiudal, and sum.
	// Currently only P, S, and nucleation phases added to stack.
	// If residual is greater than 10, make it 10.
	for (int ipick = 0; ipick < npick; ipick++) {
		auto pick = m_vPickData[ipick];
		double resi = 99999999;

		// calculate residual
		double tobs = pick->getTPick() - oT;
		double tcal;
		std::shared_ptr<CSite> site = pick->getSite();

		// only use nucleation phase if on nucleation branch
		if ((nucleate == true) && (m_pNucleationTravelTime2 == NULL)) {
			tcal = m_pNucleationTravelTime1->T(&site->getGeo());
			resi = tobs - tcal;
		} else if ((nucleate == true) && (m_pNucleationTravelTime1 == NULL)) {
			tcal = m_pNucleationTravelTime2->T(&site->getGeo());
			resi = tobs - tcal;
		} else {
			// take whichever has the smallest residual, P or S
			tcal = m_pTravelTimeTables->T(&site->getGeo(), tobs);
			if (m_pTravelTimeTables->m_sPhase == "P"
					|| m_pTravelTimeTables->m_sPhase == "S") {
				resi = tobs - tcal;
			}
		}
		resi = std::abs(resi);
		if (resi > 10.) {
			resi = 10.;
		}
		value += resi;
	}
	return value;
}

// --------------------------------------------------getTOrigin
double CHypo::getTOrigin() const {
	return (m_tOrigin);
}

// --------------------------------------------------getTSort
int64_t CHypo::getTSort() const {
	return (m_tSort);
}

// ----------------------------------------------------calculateWeightedResidual
double CHypo::calculateWeightedResidual(std::string sPhase, double tObs,
										double tCal) {
	if (tCal < 0) {
		return (std::numeric_limits<double>::quiet_NaN());
	}

	if (sPhase == "P") {
		return (tObs - tCal);
	} else if (sPhase == "S") {
		// Effectively halving the weight of S
		// this value was selected by testing specific
		// events with issues
		// NOTE: Hard Coded
		return ((tObs - tCal) * 2.0);
	} else {
		// Down weighting all other phases
		// Value was chosen so that other phases would
		// still contribute (reducing instabilities)
		// but remain insignificant
		// NOTE: Hard Coded
		return ((tObs - tCal) * 10.0);
	}
}

// --------------------------------------------------getDepth
double CHypo::getDepth() const {
	return (m_dDepth);
}

// ---------------------------------------------------graphicsOutput
void CHypo::graphicsOutput() {
	if (m_pTravelTimeTables == NULL) {
		glass3::util::Logger::log("error", "CHypo::graphicsOutput: NULL pTTT.");
		return;
	}
	// lock mutex for this scope
	std::lock_guard < std::recursive_mutex > guard(m_HypoMutex);

	// create and open file
	std::ofstream outfile;
	std::string filename = CGlass::getGraphicsOutFolder() + m_sID + ".txt";
	outfile.open(filename, std::ios::out);

	// header
	outfile << "hypocenter: " << std::to_string(m_dLatitude) << " "
			<< std::to_string(m_dLongitude) << " " << std::to_string(m_dDepth)
			<< " " << std::to_string(m_tOrigin) << "\n";

	double stack = 0;
	double tcal = 0;
	double delta = 0;
	double sigma = 0;
	glass3::util::Geo geo;

	int npick = m_vPickData.size();
	// for each step in the y axis
	for (int y = -1 * CGlass::getGraphicsSteps();
			y <= CGlass::getGraphicsSteps(); y++) {
		// compute lat
		double xlat = m_dLatitude
				+ (y * CGlass::getGraphicsStepKm())
						/ glass3::util::Geo::k_DegreesToKm;

		// for each step in the x axis
		for (int x = -1 * CGlass::getGraphicsSteps();
				x <= CGlass::getGraphicsSteps(); x++) {
			// compute lon
			double xlon = m_dLongitude
					+ cos(glass3::util::GlassMath::k_DegreesToRadians * xlat)
							* (x * CGlass::getGraphicsStepKm())
							/ glass3::util::Geo::k_DegreesToKm;

			// set up traveltimes
			m_pTravelTimeTables->setTTOrigin(xlat, xlon, m_dDepth);
			stack = 0;

			// for each pick
			for (int ipick = 0; ipick < npick; ipick++) {
				auto pick = m_vPickData[ipick];
				double tobs = pick->getTPick() - m_tOrigin;
				std::shared_ptr<CSite> site = pick->getSite();
				tcal = m_pTravelTimeTables->T(&site->getGeo(), tobs);
				delta = glass3::util::GlassMath::k_RadiansToDegrees
						* geo.delta(&site->getGeo());

				// This should be a function.
				if (delta < 1.5) {
					sigma = .75;
				} else if ((delta >= 1.5) && (delta < 30.0)) {
					sigma = 1.5;
				} else {
					sigma = 3;
				}

				// compute stack value
				stack += glass3::util::GlassMath::sig_laplace_pdf(tobs - tcal,
																	sigma);

				// write to fiel
				outfile << std::to_string(xlat) << " " << std::to_string(xlon)
						<< " " << std::to_string(stack) << "\n";
			}
		}
	}

	outfile.close();
}

// ------------------------------------------------------hasCorrelationReference
bool CHypo::hasCorrelationReference(std::shared_ptr<CCorrelation> corr) {
	// null check
	if (corr == NULL) {
		glass3::util::Logger::log("warning",
									"CHypo::hasCorrelation: NULL correlation.");
		return (false);
	}

	// lock mutex for this scope
	std::lock_guard < std::recursive_mutex > guard(m_HypoMutex);

	// for each corr in the vector
	for (const auto &q : m_vCorrelationData) {
		// is this corr a match?
		if (q->getID() == corr->getID()) {
			return (true);
		}
	}

	return (false);
}

// ---------------------------------------------------------hasPickReference
bool CHypo::hasPickReference(std::shared_ptr<CPick> pck) {
	// null check
	if (pck == NULL) {
		glass3::util::Logger::log("warning", "CHypo::hasPick: NULL pck.");
		return (false);
	}

	// lock mutex for this scope
	std::lock_guard < std::recursive_mutex > guard(m_HypoMutex);

	// for each pick in the vector
	for (const auto &q : m_vPickData) {
		// is this pick a match?
		if (q->getID() == pck->getID()) {
			return (true);
		}
	}

	return (false);
}

// ---------------------------------------------------------generateHypoMessage
std::shared_ptr<json::Object> CHypo::generateHypoMessage() {
	// null check
	if (m_pTravelTimeTables == NULL) {
		glass3::util::Logger::log("warning",
									"CHypo::generateHypoMessage: NULL pTTT.");
		return (NULL);
	}

	// make sure this event hasn't been canceled
	if (cancelCheck() == true) {
		glass3::util::Logger::log(
				"debug",
				"CHypo::generateHypoMessage: hypo: " + m_sID
						+ " has been canceled.");
		// return a cancel message
		return (generateCancelMessage());
	}

	// make sure this event is still reportable
	if (reportCheck() == false) {
		glass3::util::Logger::log(
				"debug",
				"CHypo::generateHypoMessage: hypo: " + m_sID
						+ " is not reportable.");
		// return a cancel message
		return (generateCancelMessage());
	}

	// create json object
	std::shared_ptr<json::Object> hypo = std::make_shared < json::Object
			> (json::Object());

	glass3::util::Logger::log(
			"debug",
			"CHypo::generateHypoMessage: generating hypo message for sPid:"
					+ m_sID + " sWebName:" + m_sWebName);

	// lock mutex for this scope
	std::lock_guard < std::recursive_mutex > guard(m_HypoMutex);

	// NOTE: Need to think about this format, currently it *almost*
	// creates a detection formats json, but doesn't use the library
	// maybe this should be it's own "glass" format? or just use
	// the quake format?

	// basic info
	(*hypo)["Cmd"] = "Hypo";
	(*hypo)["Type"] = "Hypo";
	(*hypo)["ID"] = m_sID;
	(*hypo)["WebName"] = getWebName();

	// source
	// NOTE: THIS NEEDS TO BE NOT HARDCODED EVENTUALLY
	// or remove it? why should glasscore care about source?
	json::Object src;
	src["AgencyID"] = "US";
	src["Author"] = "glass";
	(*hypo)["Source"] = src;

	// time
	(*hypo)["T"] = glass3::util::Date::encodeDateTime(m_tOrigin);
	(*hypo)["Time"] = glass3::util::Date::encodeISO8601Time(m_tOrigin);

	// location
	(*hypo)["Latitude"] = getLatitude();
	(*hypo)["Longitude"] = getLongitude();
	(*hypo)["Depth"] = getDepth();

	// supplementary info
	(*hypo)["MinimumDistance"] = getMinDistance();
	(*hypo)["Gap"] = getGap();
	(*hypo)["Bayes"] = getBayesValue();
	(*hypo)["InitialBayes"] = getInitialBayesValue();
	(*hypo)["Web"] = m_sWebName;

	// generate data array for this hypo
	// set up traveltime object
	m_pTravelTimeTables->setTTOrigin(m_dLatitude, m_dLongitude, m_dDepth);

	// set up geo for distance calculations
	glass3::util::Geo geo;
	geo.setGeographic(m_dLatitude, m_dLongitude,
						glass3::util::Geo::k_EarthRadiusKm - m_dDepth);

	// array to hold data
	json::Array data;

	// for each pick
	for (auto pick : m_vPickData) {
		// get basic pick values
		std::shared_ptr<CSite> site = pick->getSite();
		double tobs = pick->getTPick() - m_tOrigin;
		double tcal = m_pTravelTimeTables->T(&site->getGeo(), tobs);
		double tres = tobs - tcal;
		// should this be changed?
		double sig = glass3::util::GlassMath::sig(tres, 1.0);

		double dist = calculateDistanceToPick(pick);

		// glass3::util::Logger::log(
		// "debug",
		// "CHypo::generateHypoMessage Checking pick: " +
		// m_pTravelTimeTables->m_sPhase + "; travtime: " +
		// std::to_string(tobs) +
		// "; distance: " + std::to_string(dist) +
		// "; residual: " + std::to_string(tres) +
		// "; publishable: " + std::to_string(m_pTravelTimeTables->m_bPublishable));

		// check if we're allowed to publish this pick based
		// on whether the travel time phase is publishable
		// m_bPublishable is set in the above m_pTravelTimeTables->T()
		// call
		if (m_pTravelTimeTables->m_bPublishable == false) {
			continue;
		}

		// if we have it, use the shared pointer
		json::Object pickObj;
		std::shared_ptr<json::Object> jPick = pick->getJSONPick();
		if (jPick) {
			// start with a copy of json pick
			// which has the site, source, time, etc
			pickObj = json::Object(*jPick.get());

			// add the lat/lon/elev to site
			if (pickObj.HasKey("Site")) {
				// get the site object
				json::Object siteobj = pickObj["Site"].ToObject();
				siteobj["Latitude"] = site->getRawLatitude();
				siteobj["Longitude"] = site->getRawLongitude();
				siteobj["Elevation"] = site->getRawElevation();
				pickObj["Site"] = siteobj;
			}

			// add the association info
			json::Object assocobj;
			assocobj["Phase"] = m_pTravelTimeTables->m_sPhase;
			assocobj["Distance"] = calculateDistanceToPick(pick);
			assocobj["Azimuth"] = geo.azimuth(&site->getGeo())
					/ glass3::util::GlassMath::k_DegreesToRadians;
			assocobj["Residual"] = tres;
			assocobj["Sigma"] = sig;
			pickObj["AssociationInfo"] = assocobj;
		} else {
			// we don't have a jpick, so fill in what we know
			pickObj["Site"] = site->getSCNL();
			pickObj["Pid"] = pick->getID();
			pickObj["T"] = glass3::util::Date::encodeDateTime(pick->getTPick());
			pickObj["Time"] = glass3::util::Date::encodeISO8601Time(
					pick->getTPick());
			pickObj["Distance"] = calculateDistanceToPick(pick);
			pickObj["Azimuth"] = geo.azimuth(&site->getGeo())
					/ glass3::util::GlassMath::k_DegreesToRadians;
			pickObj["Residual"] = tres;
		}

		// add new pick to list
		data.push_back(pickObj);
	}

	// for each correlation
	for (auto correlation : m_vCorrelationData) {
		// get basic pick values
		std::shared_ptr<CSite> site = correlation->getSite();
		double tobs = correlation->getTCorrelation() - m_tOrigin;
		double tcal = m_pTravelTimeTables->T(&site->getGeo(), tobs);
		double tres = tobs - tcal;
		// should this be changed?
		double sig = glass3::util::GlassMath::sig(tres, 1.0);

		// if we have it, use the shared pointer
		json::Object correlationObj;
		std::shared_ptr<json::Object> jCorrelation = correlation
				->getJSONCorrelation();
		if (jCorrelation) {
			// start with a copy of json pick
			// which has the site, source, time, etc
			correlationObj = json::Object(*jCorrelation.get());

			// add the lat/lon/elev to site
			if (correlationObj.HasKey("Site")) {
				// get the site object
				json::Object siteobj = correlationObj["Site"].ToObject();
				siteobj["Latitude"] = site->getRawLatitude();
				siteobj["Longitude"] = site->getRawLongitude();
				siteobj["Elevation"] = site->getRawElevation();
				correlationObj["Site"] = siteobj;
			}

			// add the association info
			json::Object assocobj;
			assocobj["Phase"] = m_pTravelTimeTables->m_sPhase;
			assocobj["Distance"] = geo.delta(&site->getGeo())
					/ glass3::util::GlassMath::k_DegreesToRadians;
			assocobj["Azimuth"] = geo.azimuth(&site->getGeo())
					/ glass3::util::GlassMath::k_DegreesToRadians;
			assocobj["Residual"] = tres;
			assocobj["Sigma"] = sig;
			correlationObj["AssociationInfo"] = assocobj;
		} else {
			// we don't have a jCorrelation, so fill in what we know
			correlationObj["Site"] = site->getSCNL();
			correlationObj["Pid"] = correlation->getID();
			correlationObj["Time"] = glass3::util::Date::encodeISO8601Time(
					correlation->getTCorrelation());
			correlationObj["Latitude"] = correlation->getLatitude();
			correlationObj["Longitude"] = correlation->getLongitude();
			correlationObj["Depth"] = correlation->getDepth();
			correlationObj["Distance"] = geo.delta(&site->getGeo())
					/ glass3::util::GlassMath::k_DegreesToRadians;
			correlationObj["Azimuth"] = geo.azimuth(&site->getGeo())
					/ glass3::util::GlassMath::k_DegreesToRadians;
			correlationObj["Residual"] = tres;
			correlationObj["Correlation"] = correlation->getCorrelation();
		}

		// add new pick to list
		data.push_back(correlationObj);
	}

	// add data array to object
	(*hypo)["Data"] = data;

	// create auditing report when first hypo message is generated for this
	// hypo
	if (m_bHypoGenerated == false) {
		m_hapsAudit.dtFirstHypoMessage = glass3::util::Date::now();
		glass3::util::Logger::log(
				"info",
				"CHypo::generateHypoMessage Auditing info for: " + m_sID + " "
						+ std::to_string(m_dLatitude) + " "
						+ std::to_string(m_dLongitude) + " "
						+ std::to_string(m_dDepth) + " "
						+ std::to_string(m_tOrigin) + " "
						+ std::to_string(m_hapsAudit.dtOrigin) + " "
						+ std::to_string(m_hapsAudit.dtNucleated) + " "
						+ std::to_string(m_hapsAudit.dtNucleationPickInsertion)
						+ " " + std::to_string(m_hapsAudit.dtLastBigMove) + " "
						+ std::to_string(m_hapsAudit.nMaxPhasesBeforeMove) + " "
						+ std::to_string(m_hapsAudit.dMaxStackBeforeMove) + " "
						+ std::to_string(m_hapsAudit.nMaxPhasesSinceMove) + " "
						+ std::to_string(m_hapsAudit.dMaxStackSinceMove) + " "
						+ std::to_string(m_hapsAudit.dtFirstEventMessage) + " "
						+ std::to_string(m_hapsAudit.dLatPrev) + " "
						+ std::to_string(m_hapsAudit.dLonPrev) + " "
						+ std::to_string(m_hapsAudit.dDepthPrev) + " "
						+ std::to_string(m_iProcessCount) + " "
						+ std::to_string(m_dBayesValue) + " "
						+ std::to_string(m_vPickData.size()) + " "
						+ std::to_string(m_dInitialBayesValue) + " "
						+ std::to_string(m_dMinDistance) + " "
						+ std::to_string(m_dMedianDistance) + " "
						+ std::to_string(m_dGap) + " "
						+ std::to_string(m_dDistanceSD) + " "
						+ std::to_string(m_dAssociationDistanceCutoff) + " "
						+ std::to_string(m_dWebResolution) + "  ABC");
	}

	m_bHypoGenerated = true;

	// done
	return (hypo);
}

// ---------------------------------------------------incrementTotalProcessCount
int CHypo::incrementTotalProcessCount() {
	m_iTotalProcessCount++;

	return (m_iTotalProcessCount);
}

// ---------------------------------------------------------initialize
bool CHypo::initialize(double lat, double lon, double z, double time,
						std::string pid, std::string web, double bayes,
						double thresh, int cut,
						std::shared_ptr<traveltime::CTravelTime> firstTrav,
						std::shared_ptr<traveltime::CTravelTime> secondTrav,
						std::shared_ptr<traveltime::CTTT> ttt,
						double resolution, double aziTap, double maxDep) {
	// lock mutex for this scope
	std::lock_guard < std::recursive_mutex > guard(m_HypoMutex);

	clear();

	setLatitude(lat);
	setLongitude(lon);
	setDepth(z);
	setTOrigin(time);
	setTSort(time);
	m_sID = pid;
	m_sWebName = web;
	m_dBayesValue = bayes;
	m_dInitialBayesValue = bayes;
	m_dAzimuthTaper = aziTap;
	m_dMaxDepth = maxDep;
	m_dNucleationStackThreshold = thresh;
	m_iNucleationDataThreshold = cut;
	m_dWebResolution = resolution;
	if (m_dWebResolution == 0.0) {
		m_dWebResolution = 100.0;
	}

	// init the performance timing audit struct to 0's
	memset(&m_hapsAudit, 0, sizeof(m_hapsAudit));
	m_hapsAudit.dtOrigin = time;
	m_hapsAudit.dtCreated = glass3::util::Date::now();
	m_hapsAudit.dLatPrev = lat;
	m_hapsAudit.dLonPrev = lon;
	m_hapsAudit.dDepthPrev = z;

	// make local copies of the travel times so that we don't
	// have cross-thread contention for them between hypos
	if (firstTrav != NULL) {
		m_pNucleationTravelTime1 = std::make_shared < traveltime::CTravelTime
				> (traveltime::CTravelTime(*firstTrav));
	}

	if (secondTrav != NULL) {
		m_pNucleationTravelTime2 = std::make_shared < traveltime::CTravelTime
				> (traveltime::CTravelTime(*secondTrav));
	}

	if (ttt != NULL) {
		m_pTravelTimeTables = std::make_shared < traveltime::CTTT
				> (traveltime::CTTT(*ttt));
	}
	setTCreate(glass3::util::Date::now());

	return (true);
}

// --------------------------------------------------------isLockedForProcessing
bool CHypo::isLockedForProcessing() {
	if (m_ProcessingMutex.try_lock() == false) {
		return (true);
	}

	m_ProcessingMutex.unlock();
	return (false);
}

// ---------------------------------------------------------localize
double CHypo::localize() {
	// lock mutex for this scope
	std::lock_guard < std::recursive_mutex > guard(m_HypoMutex);

	// check to see if this is a valid hypo, a hypo must always have an id
	if (m_sID == "") {
		return (m_dBayesValue);
	}

	// Localize this hypo
	char sLog[glass3::util::Logger::k_nMaxLogEntrySize];

	// glass3::util::Logger::log("debug", "CHypo::localize. " + m_sID);

	// if hypo is fixed, just return current bayesian value
	// NOTE: What implication does this have for "seed hypos" like twitter
	// detections
	if (m_bFixed) {
		calculateStatistics();
		return (m_dBayesValue);
	}

	// get the number of picks
	int npick = m_vPickData.size();

	// based on the number of picks, call relocate
	// if there are already a large number of picks, only do it
	// so often...

	// create taper using the number of picks to define the
	// search distance in localize. Smaller search with more picks
	// the search radius starts at 1/2 the node resolution and
	// shrinks to 1/8 the node radius as picks are added
	// note that the locator can extend past the input search radius
	// these values were chosen by testing specific events
	// k_dLocationTaperConstant is a taper constant to ensure that the taper
	// starts at 0 and maxes at one when the m_vPickData size exceeds 30 picks
	glass3::util::Taper taper(
			-k_dLocationTaperConstant, -k_dLocationTaperConstant,
			-k_dLocationTaperConstant,
			k_dLocationMaxTaperThreshold + k_dLocationTaperConstant);
	double searchR = (m_dWebResolution / k_dSearchRadiusResolutionFactor
			+ taper.calculateValue(npick) * k_dSearchRadiusTaperFactor
					* m_dWebResolution) / k_dSearchRadiusFactor;

	// This should be the default
	if (CGlass::getMinimizeTTLocator() == false) {
		if (npick < k_iLocationNPickThresholdMedium) {
			annealingLocateBayes(k_iLocationNumIterationsLarge, searchR,
									k_dLocationMinDistanceStepSize,
									searchR / k_dLocationSearchRadiusToTime,
									k_dLocationMinTimeStepSize);
		} else if (npick < k_iLocationNPickThresholdLarge
				&& (npick % k_iLocationNPicksToSkipMedium) == 0) {
			annealingLocateBayes(k_iLocationNumIterationsMedium, searchR,
									k_dLocationMinDistanceStepSize,
									searchR / k_dLocationSearchRadiusToTime,
									k_dLocationMinTimeStepSize);
		} else if ((npick % k_iLocationNPicksToSkipLarge) == 0) {
			annealingLocateBayes(k_iLocationNumIterationsSmall, searchR,
									k_dLocationMinDistanceStepSize,
									searchR / k_dLocationSearchRadiusToTime,
									k_dLocationMinTimeStepSize);
		} else {
			// calculate bayes for resolve even if we didn't localize
			m_dBayesValue = calculateBayes(m_dLatitude, m_dLongitude, m_dDepth,
											m_tOrigin, false);

			snprintf(sLog, sizeof(sLog),
						"CHypo::localize: Skipping localize with %d picks",
						npick);
			glass3::util::Logger::log(sLog);
		}
	} else {
		if (npick < k_iLocationNPickThresholdSmall) {
			annealingLocateResidual(k_iLocationNumIterationsLarge, searchR,
									k_dLocationMinDistanceStepSize,
									searchR / k_dLocationSearchRadiusToTime,
									k_dLocationMinTimeStepSize);
		} else if (npick < k_iLocationNPickThresholdMedium
				&& (npick % k_iLocationNPicksToSkipSmall) == 0) {
			annealingLocateResidual(k_iLocationNumIterationsMedium, searchR,
									k_dLocationMinDistanceStepSize,
									searchR / k_dLocationSearchRadiusToTime,
									k_dLocationMinTimeStepSize);
		} else if (npick < k_iLocationNPickThresholdLarge
				&& (npick % k_iLocationNPicksToSkipMedium) == 0) {
			annealingLocateResidual(k_iLocationNumIterationsMedium, searchR,
									k_dLocationMinDistanceStepSize,
									searchR / k_dLocationSearchRadiusToTime,
									k_dLocationMinTimeStepSize);
		} else if ((npick % k_iLocationNPicksToSkipLarge) == 0) {
			annealingLocateResidual(k_iLocationNumIterationsSmall, searchR,
									k_dLocationMinDistanceStepSize,
									searchR / k_dLocationSearchRadiusToTime,
									k_dLocationMinTimeStepSize);
		} else {
			// calculate bayes for resolve even if we didn't localize
			m_dBayesValue = calculateBayes(m_dLatitude, m_dLongitude, m_dDepth,
											m_tOrigin, false);

			snprintf(sLog, sizeof(sLog),
						"CHypo::localize: Skipping localize with %d picks",
						npick);
			glass3::util::Logger::log(sLog);
		}
	}

	// log
	glass3::util::Date dt = glass3::util::Date(m_tOrigin);
	snprintf(sLog, sizeof(sLog),
				"CHypo::localize: HYP %s %s%9.4f%10.4f%6.1f %d", m_sID.c_str(),
				dt.dateTime().c_str(), getLatitude(), getLongitude(),
				getDepth(), npick);
	glass3::util::Logger::log(sLog);

	// compute current stats after location
	calculateStatistics();

	// return the final maximum bayesian fit
	return (m_dBayesValue);
}

// ---------------------------------------------------------pruneData
bool CHypo::pruneData(CHypoList* parentThread) {
	// lock mutex for this scope
	std::lock_guard < std::recursive_mutex > guard(m_HypoMutex);

	// check to see if this is a valid hypo, a hypo must always have an id
	if (m_sID == "") {
		return (false);
	}

	// glass3::util::Logger::log("debug", "CHypo::prune. " + m_sID);

	// set up local vector to track picks to remove
	std::vector < std::shared_ptr < CPick >> vremove;

	// get the standard deviation allowed for pruning
	double sdprune = CGlass::getPruningSDCutoff();
	char sLog[glass3::util::Logger::k_nMaxLogEntrySize];

	// for each pick in this hypo
	for (auto pck : m_vPickData) {
		if (parentThread != NULL) {
			parentThread->setThreadHealth();
		}

		// check to see if it can still be associated
		if (!canAssociate(pck, 1.0, sdprune)) {
			// pick no longer associates, add to remove list
			vremove.push_back(pck);

			snprintf(
					sLog, sizeof(sLog), "CHypo::prune: ASSOC-CUL %s %s",
					glass3::util::Date::encodeDateTime(pck->getTPick()).c_str(),
					pck->getSite()->getSCNL().c_str());
			glass3::util::Logger::log(sLog);

			// on to the next pick
			continue;
		}

		// Trim whiskers
		// compute distance in degrees between site and hypo
		double siteDistance = calculateDistanceToPick(pck);

		// check if site is beyond distance limit
		if (siteDistance > m_dAssociationDistanceCutoff) {
			snprintf(
					sLog, sizeof(sLog), "CHypo::prune: DIST-CUL %s %s (%.2f > %.2f)",
					glass3::util::Date::encodeDateTime(pck->getTPick()).c_str(),
					pck->getSite()->getSCNL().c_str(), siteDistance,
					getAssociationDistanceCutoff());
			glass3::util::Logger::log(sLog);

			// add pick to remove list
			vremove.push_back(pck);

			// on to the next pick
			continue;
		}
	}

	int pruneCount = 0;
	for (auto pck : vremove) {
		pruneCount++;
		pck->removeHypoReference(getID());
		removePickReference(pck);
	}

	glass3::util::Logger::log(
			"debug",
			"CHypo::prune " + m_sID + + " removed " + std::to_string(pruneCount)
			+ " picks.");

	// set up local vector to track correlations to remove
	std::vector < std::shared_ptr < CCorrelation >> vcremove;

	// get the correlation windows
	double tWindow = CGlass::getCorrelationMatchingTimeWindow();
	double xWindow = CGlass::getCorrelationMatchingDistanceWindow();

	// for each correlation in this hypo
	for (auto cor : m_vCorrelationData) {
		if (parentThread != NULL) {
			parentThread->setThreadHealth();
		}

		// check to see if it can still be associated
		if (!canAssociate(cor, tWindow, xWindow)) {
			// correlation no longer associates, add to remove list
			vcremove.push_back(cor);

			snprintf(
					sLog,
					sizeof(sLog),
					"CHypo::prune: Corr-CUL %s %s",
					glass3::util::Date::encodeDateTime(cor->getTCorrelation())
							.c_str(),
					cor->getSite()->getSCNL().c_str());
			glass3::util::Logger::log(sLog);

			// on to the next correlation
			continue;
		}
	}

	pruneCount = 0;
	for (auto cor : vcremove) {
		pruneCount++;
		cor->clearHypoReference();
		removeCorrelationReference(cor);
	}

	// glass3::util::Logger::log(
	// "debug",
	// "CHypo::prune " + m_sID + + " removed " + std::to_string(pruneCount)
	// + " correlations.");

	// if we didn't find any data to remove, just
	// return
	if ((vremove.size() < 1) && (vcremove.size() < 1)) {
		return (false);
	}

	// we've removed at least one data
	return (true);
}

// ---------------------------------------------------removeCorrelationReference
void CHypo::removeCorrelationReference(std::shared_ptr<CCorrelation> corr) {
	// null check
	if (corr == NULL) {
		glass3::util::Logger::log("warning",
									"CHypo::remCorrelation: NULL correlation.");
		return;
	}

	// lock mutex for this scope
	std::lock_guard < std::recursive_mutex > guard(m_HypoMutex);

	// get the correlation id
	std::string pid = corr->getID();

	// for each correlation in the vector
	for (int i = 0; i < m_vCorrelationData.size(); i++) {
		// get the current correlation
		auto correlation = m_vCorrelationData[i];

		// is this correlation a match?
		if (correlation->getID() == pid) {
			// remove correlation from vector
			m_vCorrelationData.erase(m_vCorrelationData.cbegin() + i);
			return;
		}
	}
}

// ---------------------------------------------------------removePickReference
void CHypo::removePickReference(std::shared_ptr<CPick> pck) {
	// null check
	if (pck == NULL) {
		glass3::util::Logger::log("warning", "CHypo::remPick: NULL pck.");
		return;
	}

	// lock mutex for this scope
	std::lock_guard < std::recursive_mutex > guard(m_HypoMutex);

	// get the pick id
	std::string pid = pck->getID();

	// for each pick in the vector
	for (int i = 0; i < m_vPickData.size(); i++) {
		// get the current pick
		auto pick = m_vPickData[i];

		// is this pick a match?
		if (pick->getID() == pid) {
			// remove pick from vector
			m_vPickData.erase(m_vPickData.cbegin() + i);

			return;
		}
	}
}

// ---------------------------------------------------------reportCheck
bool CHypo::reportCheck() {
	// lock mutex for this scope
	std::lock_guard < std::recursive_mutex > guard(m_HypoMutex);

	// check to see if this is a valid hypo, a hypo must always have an id
	if (m_sID == "") {
		return (false);
	}

	char sLog[glass3::util::Logger::k_nMaxLogEntrySize * 2];
	char sHypo[glass3::util::Logger::k_nMaxLogEntrySize];

	glass3::util::Date dt = glass3::util::Date(m_tOrigin);
	snprintf(sHypo, sizeof(sHypo), "%s %s%9.4f%10.4f%6.1f %d", m_sID.c_str(),
				dt.dateTime().c_str(), getLatitude(), getLongitude(),
				getDepth(), static_cast<int>(m_vPickData.size()));

	int nReportCut = CGlass::getReportingDataThreshold();

	// check data count
	if ((m_vPickData.size() + m_vCorrelationData.size()) < nReportCut) {
		// there isn't
		snprintf(sLog, sizeof(sLog),
					"CHypo::reportCheck: Below data count threshold "
					"((%d + %d) < %d) Hypo: %s",
					static_cast<int>(m_vPickData.size()),
					static_cast<int>(m_vCorrelationData.size()), nReportCut,
					sHypo);
		glass3::util::Logger::log(sLog);

		// this hypo cannot be reported
		return (false);
	}

	// rms check? other checks?

	// baysian threshold check
	double dReportThresh = CGlass::getReportingStackThreshold();
	if (m_dBayesValue < dReportThresh) {
		// failure
		snprintf(
				sLog, sizeof(sLog),
				"CHypo::reportCheck: Below bayesian threshold (%.1f < %.1f) "
				"Hypo: %s",
				getBayesValue(), dReportThresh, sHypo);
		glass3::util::Logger::log(sLog);

		// this hypo cannot be reported
		return (false);
	}

	// Hypo can be reported
	return (true);
}

// ---------------------------------------------------------resolveData
bool CHypo::resolveData(std::shared_ptr<CHypo> hyp, bool allowStealing,
		CHypoList* parentThread) {
	// lock the hypo since we're iterating through it's lists
	std::lock_guard < std::recursive_mutex > hypoGuard(m_HypoMutex);

	// nullchecks
	if (CGlass::getHypoList() == NULL) {
		return (false);
	}

	// check to see if this is a valid hypo, a hypo must always have an id
	if (m_sID == "") {
		return (false);
	}

	// glass3::util::Logger::log("debug", "CHypo::resolve. " + m_sID);

	bool bAssoc = false;
	char sLog[glass3::util::Logger::k_nMaxLogEntrySize];

	// handle picks
	// for each pick in this hypo
	int nPck = m_vPickData.size();

	int addedCount = 0;
	int keptCount = 0;
	int removeCount = 0;

	// NOTE: Why are we moving backwards through the list?
	for (int iPck = nPck - 1; iPck >= 0; iPck--) {
		if (parentThread != NULL) {
			parentThread->setThreadHealth();
		}

		// get the pick
		std::shared_ptr<CPick> pck = m_vPickData[iPck];

		// get the pick's hypo pointer
		std::shared_ptr<CHypo> pickHyp = pck->getHypoReference();

		// if this pick isn't linked to a hypo
		if (pickHyp == NULL) {
			addedCount++;

			// link to this hypo and move on
			pck->addHypoReference(hyp);

			continue;
		}

		std::string sOtherPid = pickHyp->getID();

		// if this pick is linked to this hypo
		if (sOtherPid == m_sID) {
			// nothing else to do
			continue;
		}

		// get the current pick's affinity to the provided hypo
		double aff1 = calculateAffinity(pck);

		// get the current pick's affinity to the hypo it's linked to
		double aff2 = pickHyp->calculateAffinity(pck);

		snprintf(sLog, sizeof(sLog),
					"CHypo::resolve: SCV COMPARE %s %s %s %s (%.2f, %.2f)",
					m_sID.c_str(), sOtherPid.c_str(),
					glass3::util::Date::encodeDateTime(pck->getTPick()).c_str(),
					pck->getSite()->getSCNL().c_str(), aff1, aff2);
		glass3::util::Logger::log(sLog);

		// check which affinity is better
		if (aff1 > aff2) {
			if (allowStealing == true) {
				// this pick has a higher affinity with this hypo than it's
				// currently linked hypo
				// remove the pick from it's original hypo
				pickHyp->removePickReference(pck);

				// link pick to the this hypo
				pck->addHypoReference(hyp, true);

				// add provided hypo to the processing queue
				// NOTE: this puts provided hypo before original hypo in FIFO,
				// we want this hypo to keep this pick, rather than the original
				// just stealing it back, which is why we do it here
				// NOTE: why add it at all? we're gonna locate before we finish
				// is it to see if we can get more next time
				CGlass::getHypoList()->appendToHypoProcessingQueue(hyp);

				// add the original hypo the pick was linked to the processing
				// queue
				CGlass::getHypoList()->appendToHypoProcessingQueue(pickHyp);

				// we've made a change to the hypo (grabbed a pick)
				bAssoc = true;
				keptCount++;
			}

		} else {
			// this pick has higher affinity with the original hypo
			// remove pick from provided hypo
			removePickReference(pck);

			// we've made a change to the hypo (got rid of a pick)
			bAssoc = true;
			removeCount++;
		}
	}

	glass3::util::Logger::log(
			"debug",
			"CHypo::resolve " + m_sID + " added:" + std::to_string(addedCount)
					+ " kept:" + std::to_string(keptCount)
					+ " removed:" + std::to_string(removeCount));

	// handle correlations
	// for each correlation in this hypo
	int nCorr = m_vCorrelationData.size();

	// NOTE: Why are we moving backwards through the list?
	for (int iCorr = nCorr - 1; iCorr >= 0; iCorr--) {
		// get the correlation
		auto corr = m_vCorrelationData[iCorr];

		// get the correlation's hypo pointer
		std::shared_ptr<CHypo> corrHyp = corr->getHypoReference();

		// if this correlation isn't linked to a hypo
		if (corrHyp == NULL) {
			// link to this hypo and move on
			corr->addHypoReference(hyp);
			continue;
		}

		std::string sOtherPid = corrHyp->getID();

		// if this corr is linked to this hypo
		if (sOtherPid == m_sID) {
			// nothing else to do
			continue;
		}

		// get the current correlation's affinity to the provided hypo
		double aff1 = calculateAffinity(corr);
		double aff2 = corrHyp->calculateAffinity(corr);

		snprintf(
				sLog,
				sizeof(sLog),
				"CHypo::resolve: C SCV COMPARE %s %s %s %s (%.2f, %.2f )",
				m_sID.c_str(),
				sOtherPid.c_str(),
				glass3::util::Date::encodeDateTime(corr->getTCorrelation())
						.c_str(),
				corr->getSite()->getSCNL().c_str(), aff1, aff2);
		glass3::util::Logger::log(sLog);

		// check which affinity is better
		if (aff1 > aff2) {
			if (allowStealing == true) {
				// this correlation has a higher affinity with the provided hypo
				// remove the correlation from it's original hypo
				corrHyp->removeCorrelationReference(corr);

				// link pick to the provided hypo
				corr->addHypoReference(hyp, true);

				// add provided hypo to the processing queue
				// NOTE: this puts provided hypo before original hypo in FIFO,
				// we want this hypo to keep this pick, rather than the original
				// just stealing it back, which is why we do it here
				CGlass::getHypoList()->appendToHypoProcessingQueue(hyp);

				// add the original hypo the pick was linked to the processing queue
				CGlass::getHypoList()->appendToHypoProcessingQueue(corrHyp);

				// we've made a change to the hypo (grabbed a pick)
				bAssoc = true;
			}
		} else {
			// this pick has higher affinity with the original hypo
			// remove pick from provided hypo
			removeCorrelationReference(corr);

			// we've made a change to the hypo (got rid of a pick)
			bAssoc = true;
		}
	}

	return (bAssoc);
}

// ---------------------------------------------------------calculateStatistics
void CHypo::calculateStatistics() {
	// lock mutex for this scope
	std::lock_guard < std::recursive_mutex > guard(m_HypoMutex);

	// Calculate the statistical distribution of distance
	// histogram for culling purposes. The actual values are
	// reflected to give 0 mean, which is, of course, completely
	// wierd. =). Consequently, the mean is not of interest.
	// don't bother with calculations if
	// there are no picks
	if (m_vPickData.size() < 1) {
		m_dDistanceSD = 0.0;
		m_dMedianDistance = 0.0;
		m_dMinDistance = 0.0;
		m_dAssociationDistanceCutoff = 0.0;
		m_iTeleseismicPhaseCount = 0;
		m_dGap = 360.0;
		return;
	}

	// set up a geographic object for this hypo
	glass3::util::Geo geo;
	geo.setGeographic(m_dLatitude, m_dLongitude,
						glass3::util::Geo::k_EarthRadiusKm - m_dDepth);

	// create and populate vectors containing the
	// pick distances and azimuths
	std::vector<double> dis;
	std::vector<double> azm;
	int telePhaseCount = 0;
	for (auto pick : m_vPickData) {
		// get the site
		std::shared_ptr<CSite> site = pick->getSite();

		// compute the distance delta
		double delta = calculateDistanceToPick(pick);

		// count phases past teleseismic distance
		if (delta >= CGlass::getTeleseismicDistanceLimit()) {
			telePhaseCount++;
		}

		// add to distance vactor
		dis.push_back(delta);

		// compute the azimuth
		double azimuth = geo.azimuth(&site->getGeo())
				/ glass3::util::GlassMath::k_DegreesToRadians;

		// add to azimuth vector
		azm.push_back(azimuth);
	}

	m_iTeleseismicPhaseCount = telePhaseCount;

	// Calculate distance standard deviation. Note that the denominator is N
	// and not N-1, since a mean of 0 is pre-ordained.
	// The skewness is also 0, since the distribution
	// is exactly symmetric
	double sum = 0.0;
	int ndis = dis.size();
	for (int i = 0; i < ndis; i++) {
		double d = dis[i];
		sum += d * d;
	}
	double var = sum / ndis;
	m_dDistanceSD = sqrt(var);

	// sort the distances
	sort(dis.begin(), dis.end());

	// get minimum distance
	m_dMinDistance = dis.front();

	// get median distance
	m_dMedianDistance = dis[ndis / 2];

	// compute distance cutoff
	// NOTE: Harley want's both the original .4 and 4.0 values to be
	// configurable via glass_init.
	// In the long term, he wants to replace this with a more statistics
	// based algorithm
	int icut = static_cast<int>((CGlass::getDistanceCutoffRatio() * ndis));
	m_dAssociationDistanceCutoff = CGlass::getDistanceCutoffFactor()
			* dis[icut];

	// make sure our calculated dCut is not below the minimum allowed
	if (m_dAssociationDistanceCutoff < CGlass::getMinDistanceCutoff()) {
		m_dAssociationDistanceCutoff = CGlass::getMinDistanceCutoff();
	}

	// sort the azimuths
	sort(azm.begin(), azm.end());

	// add the first (smallest) azimuth to the end by adding 360
	azm.push_back(azm.front() + 360.0);

	// compute gap
	m_dGap = calculateGap(m_dLatitude, m_dLongitude, m_dDepth);

	/* glass3::util::Logger::log("debug",
		"CHypo::calculateStatistics: ID: " + getID()
		+ "; m_dDistanceSD: " + std::to_string(m_dDistanceSD)
		+ "; m_dMedianDistance: " + std::to_string(m_dMedianDistance)
		+ "; m_dMinDistance: " + std::to_string(m_dMinDistance)
		+ "; m_dAssociationDistanceCutoff: "
		+ std::to_string(m_dAssociationDistanceCutoff)
		+ "; m_iTeleseismicPhaseCount: "
		+ std::to_string(m_iTeleseismicPhaseCount)
		+ "; m_dGap: " + std::to_string(m_dGap));
	*/
}

// ---------------------------------------------------------trap
void CHypo::trap() {
	// lock mutex for this scope
	std::lock_guard < std::recursive_mutex > guard(m_HypoMutex);

	char sLog[glass3::util::Logger::k_nMaxLogEntrySize];

	// for each pick in this hypocenter
	for (const auto &q : m_vPickData) {
		if (q == NULL) {
			continue;
		}

		// get the pick's hypo pointer
		std::shared_ptr<CHypo> hyp = q->getHypoReference();

		// check pointer
		if (hyp == NULL) {
			// bad hypo pointer
			snprintf(sLog, sizeof(sLog),
						"CHypo::trap: sPid %s Pick %s has no back link to hypo",
						m_sID.c_str(), q->getID().c_str());
			glass3::util::Logger::log("warning", sLog);

			continue;
		}

		// check sPid
		if (hyp->getID() != m_sID) {
			// sPid is for a different hypo
			snprintf(
					sLog, sizeof(sLog),
					"CHypo::trap: sPid %s Pick: %s linked to another hypo: %s",
					m_sID.c_str(), q->getID().c_str(), hyp->getID().c_str());
			glass3::util::Logger::log("warning", sLog);
		}
	}
}

// ---------------------------------------------------------setProcessCount
int CHypo::setProcessCount(int newCycle) {
	m_iProcessCount = newCycle;

	return (m_iProcessCount);
}

// ---------------------------------------------------setNucleationDataThreshold
void CHypo::setNucleationDataThreshold(int cut) {
	m_iNucleationDataThreshold = cut;
}

// --------------------------------------------------setCorrelationAdded
void CHypo::setCorrelationAdded(bool corrAdded) {
	m_bCorrelationAdded = corrAdded;
}

// --------------------------------------------------setFixed
void CHypo::setFixed(bool fixed) {
	m_bFixed = fixed;
}

// --------------------------------------------------setLatitude
void CHypo::setLatitude(double lat) {
	m_dLatitude = lat;
}

// --------------------------------------------------setLongitude
void CHypo::setLongitude(double lon) {
	// longitude wrap check
	if (lon > 180.0) {
		// lon is greater than 180
		m_dLongitude = lon - 360.0;
	} else if (lon < -180.0) {
		// lon is less than -180
		m_dLongitude = lon + 360.0;
	} else {
		m_dLongitude = lon;
	}
}

// --------------------------------------------------setDepth
void CHypo::setDepth(double z) {
	m_dDepth = z;
}

// --------------------------------------------------setTOrigin
void CHypo::setTOrigin(double newTOrg) {
	m_tOrigin = newTOrg;
}

// --------------------------------------------------setTSort
void CHypo::setTSort(double newTSort) {
	m_tSort = std::floor(newTSort);
}

// ---------------------------------------------------------setTCreate
void CHypo::setTCreate(double newTCreate) {
	m_tCreate = newTCreate;
}

// --------------------------------------------------setNucleationStackThreshold
void CHypo::setNucleationStackThreshold(double thresh) {
	m_dNucleationStackThreshold = thresh;
}

// --------------------------------------------------setNucleationAuditingInfo
void CHypo::setNucleationAuditingInfo(double tNucleation,
										double tNucleationKeyPickInsertion) {
	// lock mutex for this scope
	std::lock_guard < std::recursive_mutex > guard(m_HypoMutex);

	if (!m_hapsAudit.dtNucleated) {
		m_hapsAudit.dtNucleated = tNucleation;
		m_hapsAudit.dtNucleationPickInsertion = tNucleationKeyPickInsertion;
	}
}

// ----------------------------------------------getHypoAuditingPerformanceInfo
const HypoAuditingPerformanceStruct * CHypo::getHypoAuditingPerformanceInfo() {
	// lock mutex for this scope
	std::lock_guard < std::recursive_mutex > guard(m_HypoMutex);

	return (&m_hapsAudit);
}
}  // namespace glasscore

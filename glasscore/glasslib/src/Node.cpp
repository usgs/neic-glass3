#include "Node.h"
#include <json.h>
#include <logger.h>
#include <glassmath.h>
#include <geo.h>
#include <memory>
#include <string>
#include <utility>
#include <tuple>
#include <limits>
#include <mutex>
#include <algorithm>
#include <vector>
#include <cmath>
#include <set>
#include "Glass.h"
#include "Web.h"
#include "Trigger.h"
#include "Site.h"
#include "Pick.h"
#include "PickList.h"

namespace glasscore {

// constants
constexpr double CNode::k_dTravelTimePickSelectionWindow;
constexpr double CNode::k_dDepthShellResolutionKm;
constexpr double CNode::k_dGridPointVsResolutionRatio;
constexpr double CNode::k_residualDistanceAllowanceFactor;

// site Link sorting function
// Compares site links using travel times
bool sortSiteLink(const SiteLink &lhs, const SiteLink &rhs) {
	double travelTime1 = std::get < LINK_TT1 > (lhs);
	if (travelTime1 < 0) {
		travelTime1 = std::get < LINK_TT2 > (lhs);
	}

	double travelTime2 = std::get < LINK_TT1 > (rhs);
	if (travelTime2 < 0) {
		travelTime2 = std::get < LINK_TT2 > (rhs);
	}

	// compare
	if (travelTime1 < travelTime2) {
		return (true);
	}

	// travelTime2 > travelTime1
	return (false);
}

// ---------------------------------------------------------CNode
CNode::CNode() {
	clear();
}

// ---------------------------------------------------------CNode
CNode::CNode(std::string name, double lat, double lon, double z,
				double resolution, double maxDepth, bool aseismic) {
	if (!initialize(name, lat, lon, z, resolution, maxDepth, aseismic)) {
		clear();
	}
}

// ---------------------------------------------------------~CNode
CNode::~CNode() {
}

// ---------------------------------------------------------clear
void CNode::clear() {
	std::lock_guard < std::recursive_mutex > nodeGuard(m_NodeMutex);

	clearSiteLinks();

	m_sName = "UNDEFINED";
	m_pWeb = NULL;
	m_dLatitude = 0;
	m_dLongitude = 0;
	m_dDepth = 0;
	m_dResolution = 0;
	m_dMaxDepth = 0;
	m_bEnabled = false;
	m_bAseismic = false;
	m_SourceSet.clear();
}

// ---------------------------------------------------------clearSiteLinks
void CNode::clearSiteLinks() {
	// lock mutex for this scope
	std::lock_guard < std::mutex > guard(m_SiteLinkListMutex);

	m_dMaxSiteDistance = 0;

	if (m_vSiteLinkList.size() == 0) {
		return;
	}

	// remove any links that sites have TO this node
	for (auto &link : m_vSiteLinkList) {
		std::shared_ptr<CSite> aSite = std::get < LINK_PTR > (link);
		aSite->removeNode(getID());
	}

	// remove all the links from this node to sites
	m_vSiteLinkList.clear();
}

// ---------------------------------------------------------initialize
bool CNode::initialize(std::string name, double lat, double lon, double z,
						double resolution, double maxDepth, bool aseismic) {
	std::lock_guard < std::recursive_mutex > nodeGuard(m_NodeMutex);

	clear();

	m_sName = name;
	m_dLatitude = lat;
	m_dLongitude = lon;
	m_dDepth = z;
	m_dResolution = resolution;
	m_dMaxDepth = maxDepth;
	m_bEnabled = true;
	m_bAseismic = aseismic;

	return (true);
}

// ---------------------------------------------------------linkSite
bool CNode::linkSite(std::shared_ptr<CSite> site, std::shared_ptr<CNode> node,
						double distDeg, double travelTime1, std::string phase1,
						double travelTime2, std::string phase2) {
	// nullchecks
	// check site
	if (site == NULL) {
		glass3::util::Logger::log("error",
									"CNode::linkSite: NULL site pointer.");
		return (false);
	}
	// check node
	if (node == NULL) {
		glass3::util::Logger::log("error",
									"CNode::linkSite: NULL node pointer.");
		return (false);
	}

	// lock mutex for this scope
	std::lock_guard < std::mutex > guard(m_SiteLinkListMutex);

	// Link node to site using traveltime
	// NOTE: No validation on travel times or distance
	SiteLink link = std::make_tuple(site, travelTime1, phase1, travelTime2,
									phase2, distDeg);
	m_vSiteLinkList.push_back(link);

	// link site to node, again using the traveltime
	// NOTE: this used to be site->addNode(shared_ptr<CNode>(this), tt);
	// but that caused problems when deleting site-node links.
	site->addNode(node, distDeg, travelTime1, phase1, travelTime2, phase2);

	if (distDeg > m_dMaxSiteDistance) {
		m_dMaxSiteDistance = distDeg;
	}

	// successfully linked site
	return (true);
}

// ---------------------------------------------------------unlinkSite
bool CNode::unlinkSite(std::shared_ptr<CSite> site) {
	// nullchecks
	// check site
	if (site == NULL) {
		glass3::util::Logger::log("error",
									"CNode::unlinkSite: NULL site pointer.");
		return (false);
	}

	// lock while searching / modifing vSite
	m_SiteLinkListMutex.lock();

	// search through each site linked to this node
	bool found = false;
	SiteLink foundLink;
	std::shared_ptr<CSite> foundSite;
	for (const auto &link : m_vSiteLinkList) {
		// get the site
		std::shared_ptr<CSite> currentSite = std::get < LINK_PTR > (link);

		// if the new station would be before
		// the current station
		if (currentSite == site) {
			found = true;
			foundLink = link;
			foundSite = currentSite;

			// done
			break;
		}
	}

	if (found == true) {
		// find site iterator to remove
		auto it = std::find(m_vSiteLinkList.begin(), m_vSiteLinkList.end(),
							foundLink);
		if (it != m_vSiteLinkList.end()) {
			// remove site
			// unlink site from node
			m_vSiteLinkList.erase(it);

			// recompute furthest site distance
			m_dMaxSiteDistance = 0;
			for (const auto &link : m_vSiteLinkList) {
				// get the distance
				double distDeg = std::get < LINK_DIST > (link);

				if (distDeg > m_dMaxSiteDistance) {
					m_dMaxSiteDistance = distDeg;
				}
			}

			// done modifying vSite
			m_SiteLinkListMutex.unlock();

			// unlink node from site
			// done after unlock to avoid node-site deadlocks
			foundSite->removeNode(getID());

			return (true);
		}
	}

	// unlock before returning
	m_SiteLinkListMutex.unlock();
	return (false);
}

// ---------------------------------------------------------unlinkLastSite
bool CNode::unlinkLastSite() {
	// get the last site in the list
	std::shared_ptr<CSite> lastSite = getLastSite();

	if (lastSite == NULL) {
		return (false);
	}

	// unlink node from last site
	// done before lock guard to prevent
	// deadlock between node and site list mutexes.
	lastSite->removeNode(getID());

	// lock mutex for this scope
	std::lock_guard < std::mutex > guard(m_SiteLinkListMutex);

	// unlink last site from node
	m_vSiteLinkList.pop_back();

	// recompute furthest site distance
	m_dMaxSiteDistance = 0;
	for (const auto &link : m_vSiteLinkList) {
		// get the distance
		double distDeg = std::get < LINK_DIST > (link);

		if (distDeg > m_dMaxSiteDistance) {
			m_dMaxSiteDistance = distDeg;
		}
	}

	return (true);
}

// ---------------------------------------------------------nucleate
std::shared_ptr<CTrigger> CNode::nucleate(double tOrigin,
		CPickList* parentThread) {
	std::lock_guard < std::recursive_mutex > nodeGuard(m_NodeMutex);
	// don't nucleate if this node is disabled
	if (m_bEnabled == false) {
		return (NULL);
	}

	// nullchecks
	// check web
	if (m_pWeb == NULL) {
		glass3::util::Logger::log("error",
									"CNode::nucleate: NULL web pointer.");
		return (NULL);
	}

	// get the cut and threshold from our
	// parent web
	int nCut = m_pWeb->getNucleationDataCountThreshold();
	double dThresh = m_pWeb->getNucleationStackThreshold();

	// use aseismic flag to decide whether to use stricter thresholds
	if (m_bAseismic == true) {
		nCut = m_pWeb->getASeismicNucleationDataCountThreshold();
		dThresh = m_pWeb->getASeismicNucleationStackThreshold();
	}

	double dAzimuthRange = CGlass::getBeamMatchingAzimuthWindow();
	// commented out because slowness matching of beams is not yet implemented
	// but is scheduled to be soon
	// double dDistanceRange = CGlass::getBeamMatchingDistanceWindow();

	// init overall significance sum and node site count
	// to 0
	double dSum = 0.0;
	int nCount = 0;

	std::vector < std::shared_ptr < CPick >> vPick;

	// lock mutex for this scope (iterating through the site links)
	std::lock_guard < std::mutex > guard(m_SiteLinkListMutex);

	bool haltNucleation = false;

	// search through each site linked to this node
	for (const auto &link : m_vSiteLinkList) {
		// halt nucleation if the node has been disabled
		if (m_bEnabled == false) {
			haltNucleation = true;
			break;
		}

		if (parentThread != NULL) {
			parentThread->setThreadHealth();
		}

		// init sigbest
		double dSigBest_phase1 = -1.0;
		double dSigBest_phase2 = -1.0;

		// the best nucleating picks
		std::shared_ptr<CPick> pickBest_phase1;
		std::shared_ptr<CPick> pickBest_phase2;

		// get shared pointer to site
		std::shared_ptr<CSite> site = std::get < LINK_PTR > (link);

		// Ignore if station out of service
		if (!site->getUse()) {
			continue;
		}
		if (!site->getEnable()) {
			continue;
		}

		// get traveltime(s) to site
		double travelTime1 = std::get < LINK_TT1 > (link);
		std::string phase1 = std::get < LINK_PHS1 > (link);
		double travelTime2 = std::get < LINK_TT2 > (link);
		std::string phase2 = std::get < LINK_PHS2 > (link);
		double distDeg = std::get < LINK_DIST > (link);

		// the minimum and maximum time windows for picks
		double min = 0.0;
		double max = 0.0;

		// the exclusion window within min and max that we don't want
		// picks from
		double dtExcludeBegin = 0.0;
		double dtExcludeEnd = 0.0;

		// use traveltimes to compute min and max
		if ((travelTime1 >= 0) && (travelTime2 >= 0)) {
			// both travel times are valid
			if (travelTime1 <= travelTime2) {
				// TT1 smaller/shorter/faster
				min = tOrigin + travelTime1
						- (k_dTravelTimePickSelectionWindow / 2);
				max = tOrigin + travelTime2
						+ (k_dTravelTimePickSelectionWindow / 2);
			} else {
				// TT2 smaller/shorter/faster
				min = tOrigin + travelTime2
						- (k_dTravelTimePickSelectionWindow / 2);
				max = tOrigin + travelTime1
						+ (k_dTravelTimePickSelectionWindow / 2);
			}
		} else if (travelTime1 >= 0) {
			// Only TT1 valid
			min = tOrigin + travelTime1
					- (k_dTravelTimePickSelectionWindow / 2);
			max = tOrigin + travelTime1
					+ (k_dTravelTimePickSelectionWindow / 2);
		} else if (travelTime2 >= 0) {
			// Only TT2 valid
			min = tOrigin + travelTime2
					- (k_dTravelTimePickSelectionWindow / 2);
			max = tOrigin + travelTime2
					+ (k_dTravelTimePickSelectionWindow / 2);
		} else {
			// no valid TTs
			glass3::util::Logger::log(
					"error", "CNode::nucleate: Bad Node Traveltimes while generating pick"
					" SearchRange.");
			continue;
		}

		// use min and max to compute exclusion window
		if ((max - min) > k_dTravelTimePickSelectionWindow) {
			// we have two different TTs and there's a window of picks
			// in between the two TTs we don't want
			dtExcludeBegin = min + (k_dTravelTimePickSelectionWindow / 2) * 2.0;
			dtExcludeEnd = max - (k_dTravelTimePickSelectionWindow / 2) * 2.0;
			if (dtExcludeEnd < dtExcludeBegin) {
				// we effectively don't have a window because our two windows
				// overlap.
				dtExcludeBegin = 0.0;
			}
		}

		// lock site pick list while we're extracting our picks
		site->getPickMutex().lock();

		// compute bounds iterator
		auto lower = site->getLower(min);

		for (auto it = lower; (it != site->getEnd()); ++it) {
			// halt nucleation if the node has been disabled
			if (m_bEnabled == false) {
				haltNucleation = true;
				break;
			}

			auto pick = *it;

			bool phase1set = false;
			bool phase2set = false;

			if (pick == NULL) {
				continue;
			}

			// skip this pick if it's not in the set of allowed sources
			std::string pickSource = pick->getSource();
			if ((m_SourceSet.empty() == false) &&
				(pickSource != "") &&
				(m_SourceSet.find(pickSource) == m_SourceSet.end())) {
				// we have a set of allowed sources
				// and we have a valid pick source
				// and the source is NOT found in allowed sources
				// so skip this pick
				continue;
			}

			// check to see if the pick is currently associated to a hypo
			/*
			std::shared_ptr<CHypo> pHypo = pick->getHypoReference();
			if (pHypo != NULL) {
				// compute ratio to threshold
				double adBayesRatio = (pHypo->getBayesValue())
						/ (pHypo->getNucleationStackThreshold());

				// check to see if the ratio is high enough to not bother
				// nucleating with
				// NOTE: Hardcoded ratio threshold
				if (adBayesRatio > 2.0) {
					continue;
				}
			}
			*/

			// get the pick's arrival time
			double tPick = pick->getTPick();

			if (tPick > max) {
				break;  // picks are in time order and we're past our window
			}

			// skip this pick if it's in our exclude window(if we have one)
			// between our two TTs
			if (dtExcludeBegin && (tPick > dtExcludeBegin)
					&& (tPick <= dtExcludeEnd)) {
				continue;
			}

			// get the picks back azimuth
			double backAzimuth = pick->getBackAzimuth();

			// compute observed travel time from the pick time and
			// the provided origin time
			double tObs = tPick - tOrigin;

			// check backazimuth if present
			if (backAzimuth > 0) {
				// set up a geo for distance calculations
				glass3::util::Geo nodeGeo;
				nodeGeo.setGeographic(
						m_dLatitude, m_dLongitude,
						glass3::util::Geo::k_EarthRadiusKm - m_dDepth);

				// compute azimuth from the site to the node
				double siteAzimuth = pick->getSite()->getGeo().azimuth(
						&nodeGeo);

				// check to see if pick's backazimuth is within the
				// valid range
				if (glass3::util::GlassMath::angleDifference(backAzimuth,
																siteAzimuth)
						> dAzimuthRange) {
					// it is not, do not nucleate
					continue;
				}
			}

			// check slowness if present
			// Need modify travel time libraries to support getting distance
			// from slowness, and it's of limited value compared to the back
			// azimuth check
			/*if (pick->dSlowness > 0) {
			 // compute distance from the site to the node
			 double siteDistance = pick->pSite->getGeo().delta(&nodeGeo);
			 // compute observed distance from slowness (1/velocity)
			 // and tObs (distance = velocity * time)
			 double obsDistance = (1 / pick->dSlowness) * tObs;
			 // check to see if the observed distance is within the
			 // valid range
			 if ((obsDistance < (siteDistance - dDistanceRange))
			 || (obsDistance > (siteDistance + dDistanceRange))) {
			 // it is not, do not nucleate
			 continue;
			 }
			 }
			 */

			// check pick classification
			// are we configured to check pick phase classification
			if (CGlass::getPickPhaseClassificationThreshold() > 0) {
				// check to see if the phase classification is valid and above
				// our threshold
				if ((std::isnan(pick->getClassifiedPhaseProbability()) != true)
						&& (pick->getClassifiedPhaseProbability()
								> CGlass::getPickPhaseClassificationThreshold())) {
					// check to see if the phase is classified as one of our
					// nucleation phases
					if (pick->getClassifiedPhase() == phase1) {
						// match, we only consider traveltime1, disable
						// traveltime2
						phase1set = true;
					} else if (pick->getClassifiedPhase() == phase2) {
						// match, we only consider traveltime2, disable
						// traveltime1
						phase2set = true;
					}
					// otherwise there is no match and it's business as usual
				}
			}

			// check azimuth classification
			if (CGlass::getPickAzimuthClassificationThreshold() > 0) {
				if ((std::isnan(pick->getClassifiedAzimuthProbability()) != true)
						&& (pick->getClassifiedAzimuthProbability()
								> CGlass::getPickAzimuthClassificationThreshold())) {
					// set up a geo for azimuth calculations
					glass3::util::Geo nodeGeo;
					nodeGeo.setGeographic(
							m_dLatitude, m_dLongitude,
							glass3::util::Geo::k_EarthRadiusKm - m_dDepth);

					// compute azimuth from the azimuth node to site
					double siteAzimuth = nodeGeo.azimuth(
							&(pick->getSite()->getGeo()))
							* glass3::util::GlassMath::k_RadiansToDegrees;

					// check to see if pick's backazimuth is within the
					// valid range
					if (glass3::util::GlassMath::angleDifference(
							pick->getClassifiedAzimuth(), siteAzimuth)
							> CGlass::getPickAzimuthClassificationUncertainty()) {
						// it is not, do not nucleate
						continue;
					}
				}
			}

			// check distance classification
			if (CGlass::getPickDistanceClassificationThreshold() > 0) {
				if ((std::isnan(pick->getClassifiedDistanceProbability())
						!= true)
						&& (pick->getClassifiedDistanceProbability()
								> CGlass::getPickDistanceClassificationThreshold())) {
					// set up a geo for distance calculations
					glass3::util::Geo nodeGeo;
					nodeGeo.setGeographic(
							m_dLatitude, m_dLongitude,
							glass3::util::Geo::k_EarthRadiusKm - m_dDepth);

					// compute distance from the site to the node
					double siteDistance = pick->getSite()->getGeo().delta(
							&nodeGeo)
							* glass3::util::GlassMath::k_RadiansToDegrees;

					// check to see if pick's distance is within the
					// valid range
					if (siteDistance
							< CGlass::getDistanceClassLowerBound(
									pick->getClassifiedDistance())
							|| siteDistance
									> CGlass::getDistanceClassUpperBound(
											pick->getClassifiedDistance())) {
						// it is not, do not nucleate
						continue;
					}
				}
			}

			// get the best significance from the observed time and link
			double dSig1 = getSignificance(tObs, travelTime1, distDeg);
			double dSig2 = getSignificance(tObs, travelTime2, distDeg);

			if(phase1set) {
				dSig2 = -1.;
			}
			if(phase2set) {
				dSig1 = -1.;
			}
			if (dSig1 >= dSig2 && dSig1 > dSigBest_phase1) {
				dSigBest_phase1 = dSig1;
				pickBest_phase1 = pick;
			}
			if (dSig2 > dSig1 && dSig2 > dSigBest_phase2) {
				dSigBest_phase2 = dSig2;
				pickBest_phase2 = pick;
			}
		}  // ---- end search through each pick at this site ----

		site->getPickMutex().unlock();

		// signal that we're still here
		if (parentThread != NULL) {
			parentThread->setThreadHealth();
		}

		// break out of the loop if nucleation has been halted
		if (haltNucleation == true) {
			break;
		}

		// check to see if the pick with the highest significance at this site
		// should be added to the overall sum from this site
		// NOTE: This significance threshold is hard coded.
		if ((dSigBest_phase1 >= 0.1) && (pickBest_phase1 != NULL)) {
			// count this site
			nCount++;

			// add the best pick significance to the node
			// significance sum
			dSum += dSigBest_phase1;

			// add the pick to the pick vector
			vPick.push_back(pickBest_phase1);
		}
		if ((dSigBest_phase2 >= 0.1) && (pickBest_phase2 != NULL)) {
			// count this site
			nCount++;
			// add the best pick significance to the node
			// significance sum
			dSum += dSigBest_phase2;

			// add the pick to the pick vector
			vPick.push_back(pickBest_phase2);
		}
	}  // ---- end search through each site this node is linked to ----

	// signal that we're still here
	if (parentThread != NULL) {
		parentThread->setThreadHealth();
	}

	// if we were halted, the node did not nucleate an event, return null
	if (haltNucleation == true) {
		return (NULL);
	}

	// make sure the number of significant picks
	// exceeds the nucleation threshold
	if (nCount < nCut) {
		// the node did not nucleate an event
		return (NULL);
	}

	// make sure the total node significance exceeds the
	// significance threshold
	if (dSum < dThresh) {
		// the node did not nucleate an event
		return (NULL);
	}

	// create trigger
	std::shared_ptr<CTrigger> trigger(
			new CTrigger(m_dLatitude, m_dLongitude, m_dDepth, tOrigin,
							m_dResolution, m_dMaxDepth, dSum, nCount,
							m_bAseismic, vPick, m_pWeb));

	// the node nucleated an event
	return (trigger);
}

// ---------------------------------------------------------getSignificance
double CNode::getSignificance(double tObservedTT, double travelTime,
								double distDeg) {
	// use observed travel time, travel times to site
	double tRes = -1;
	if (travelTime > 0) {
		// calculate time residual
		tRes = std::abs(tObservedTT - travelTime);
	}

	// compute significances using residuals and web resolution
	// should trigger be a looser cutoff than location cutoff
	//
	// The significance is defined in a way that allows for picks to still be
	// significant even if an event is not directly on a node. This is done in
	// the form of a residual allowance which calculates the maximum off grid
	// distance assuming the nodes form a cuboid and multiply but compute
	// slowness at that region, then multiplies by a factor (2) for slop.
	double dSig = 0;
	if (tRes > 0) {
		dSig =
				glass3::util::GlassMath::sig(
						std::max(
								0.0,
								(tRes
										- (travelTime / distDeg)
												* (std::sqrt(
														3.
																* (m_dResolution
																		* m_dResolution)
																+ (k_dDepthShellResolutionKm
																		* k_dDepthShellResolutionKm))
														* .5)
												* glass3::util::Geo::k_KmToDegrees
												* k_residualDistanceAllowanceFactor)),
						CGlass::k_dNucleationSecondsPerSigma);
	}

	return (dSig);
}

// ---------------------------------------------------------getSite
std::shared_ptr<CSite> CNode::getSite(std::string siteID) {
	if (siteID == "") {
		return (NULL);
	}

	// lock mutex for this scope
	std::lock_guard < std::mutex > guard(m_SiteLinkListMutex);

	// NOTE: could be made more efficient (faster)
	// if we had a std::map
	// for all sites
	for (const auto &link : m_vSiteLinkList) {
		// get the site
		auto aSite = std::get < LINK_PTR > (link);

		if (aSite->getSCNL() == siteID) {
			// found
			return (aSite);
		}
	}

	// not found
	return (NULL);
}

// ---------------------------------------------------------getLastSite
std::shared_ptr<CSite> CNode::getLastSite() {
	if (getSiteLinksCount() == 0) {
		return (NULL);
	}

	// lock mutex for this scope
	std::lock_guard < std::mutex > guard(m_SiteLinkListMutex);

	SiteLink lastLink = m_vSiteLinkList[m_vSiteLinkList.size() - 1];
	std::shared_ptr<CSite> lastSite = std::get < LINK_PTR > (lastLink);

	// found
	return (lastSite);
}

// ---------------------------------------------------------sortSiteLinks
void CNode::sortSiteLinks() {
	// lock mutex for this scope
	std::lock_guard < std::mutex > guard(m_SiteLinkListMutex);

	// sort sites
	sort(m_vSiteLinkList.begin(), m_vSiteLinkList.end(), sortSiteLink);
}

// ---------------------------------------------------------getSitesString
std::string CNode::getSitesString() {
	// lock mutex for this scope
	std::lock_guard < std::mutex > guard(m_SiteLinkListMutex);
	std::string siteString = "";

	// write to station file
	for (const auto &link : m_vSiteLinkList) {
		// get the site
		std::shared_ptr<CSite> currentSite = std::get < LINK_PTR > (link);
		double lat, lon, r;

		currentSite->getGeo().getGeographic(&lat, &lon, &r);

		siteString += getID() + "," + currentSite->getSCNL() + ","
				+ std::to_string(lat) + ";" + std::to_string(lon) + ";"
				+ std::to_string(r) + "\n";
	}

	return (siteString);
}

// ---------------------------------------------------------getSiteLinksCount
int CNode::getSiteLinksCount() const {
	// lock mutex for this scope
	std::lock_guard < std::mutex > guard(m_SiteLinkListMutex);
	return (m_vSiteLinkList.size());
}

// ---------------------------------------------------------getEnabled
bool CNode::getEnabled() const {
	return (m_bEnabled);
}

// ---------------------------------------------------------setEnabled
void CNode::setEnabled(bool enabled) {
	m_bEnabled = enabled;
}

// ---------------------------------------------------------getLatitude
double CNode::getLatitude() const {
	return (m_dLatitude);
}

// ---------------------------------------------------------getLongitude
double CNode::getLongitude() const {
	return (m_dLongitude);
}

// ---------------------------------------------------------getResolution
double CNode::getResolution() const {
	return (m_dResolution);
}

// ---------------------------------------------------------getDepth
double CNode::getDepth() const {
	return (m_dDepth);
}

// ---------------------------------------------------------getMaxDepth
double CNode::getMaxDepth() const {
	return (m_dMaxDepth);
}

// ---------------------------------------------------------getAseismic
bool CNode::getAseismic() const {
	return (m_bAseismic);
}

// ---------------------------------------------------------getGeo
glass3::util::Geo CNode::getGeo() const {
	glass3::util::Geo geoNode;
	geoNode.setGeographic(m_dLatitude, m_dLongitude,
							glass3::util::Geo::k_EarthRadiusKm - m_dDepth);
	return (geoNode);
}

// ---------------------------------------------------------getWeb
CWeb * CNode::getWeb() const {
	std::lock_guard < std::recursive_mutex > nodeGuard(m_NodeMutex);
	return (m_pWeb);
}

// ---------------------------------------------------------setWeb
void CNode::setWeb(CWeb* web) {
	std::lock_guard < std::recursive_mutex > nodeGuard(m_NodeMutex);
	m_pWeb = web;
}

// ---------------------------------------------------------getName
const std::string& CNode::getName() const {
	return (m_sName);
}

// ---------------------------------------------------------getID
std::string CNode::getID() const {
	return (std::string(
			m_sName + "." + std::to_string(getLatitude()) + "."
					+ std::to_string(getLongitude()) + "."
					+ std::to_string(getDepth())));
}

// -------------------------------------------------getMaxSiteDistance
double CNode::getMaxSiteDistance() const {
	std::lock_guard < std::mutex > guard(m_SiteLinkListMutex);
	return(m_dMaxSiteDistance);
}

// -------------------------------------------------------addSource
void CNode::addSource(std::string source) {
	std::lock_guard < std::recursive_mutex > nodeGuard(m_NodeMutex);
	m_SourceSet.insert(source);
}
}  // namespace glasscore
